/*******************************************************************************
 * selkie - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * SelKie standard implementation. (ALSA Implementation)
 ******************************************************************************/

#include "selkie.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alsa/asoundlib.h>
#include <libudev.h>

////////////////////////////////////////////////////////////////////////////////
// Implementation Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkExtensionData_device_polling_SEA {
  SkBool32                      initialized;
  struct udev*                  pUdev;
  struct udev_monitor*          pUdevMonitor;
} SkExtensionData_device_polling_SEA;

typedef struct SkPhysicalDevice_T {
  char                          identifier[32];
  SkInstance                    instance;
  struct SkPhysicalDevice_T*    next;
  int                           card;
  snd_ctl_card_info_t*          pCardInfo;
} SkPhysicalDevice_T;

typedef struct SkInstance_T {
  const SkAllocationCallbacks*  pAllocator;
  SkPhysicalDevice_T*           pPhysicalDevices;
  SkExtensionData_device_polling_SEA extension_device_polling_SEA;
} SkInstance_T;

void* SkDefaultAllocationFunction(
  void*               pUserData,
  size_t              size,
  size_t              alignment,
  SkAllocationScope   scope
) {
  (void)scope;
  (void)pUserData;
  return aligned_alloc(alignment, size);
}

void* SkDefaultFreeFunction(
  void*               pUserData,
  void*               memory
) {
  (void)pUserData;
  free(memory);
}

static SkAllocationCallbacks SkDefaultAllocationCallbacks = {
  NULL,
  &SkDefaultAllocationFunction,
  &SkDefaultFreeFunction
};

typedef SkResult (*PFN_skConstructExtension)(SkInstance instance);

typedef struct SkExtensionPropertiesInternal {
  char extensionName[SK_MAX_EXTENSION_NAME_SIZE];
  uint32_t specVersion;
  PFN_skConstructExtension constructExtension;
} SkExtensionPropertiesInternal;

////////////////////////////////////////////////////////////////////////////////
// Implementation Functions (Internal)
////////////////////////////////////////////////////////////////////////////////
SkResult skConstructExtension_device_polling_SEA(SkInstance instance) {
  SkExtensionData_device_polling_SEA *extension = &instance->extension_device_polling_SEA;
  SkAllocationCallbacks const* pAllocator = instance->pAllocator;
  if (extension->initialized) return SK_SUCCESS;

  // Set up for monitoring device change events
  extension->pUdev = udev_new();
  if (!extension->pUdev) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  extension->pUdevMonitor = udev_monitor_new_from_netlink(extension->pUdev, "udev");
  if (!extension->pUdevMonitor) {
    udev_unref(extension->pUdev);
    return SK_ERROR_INITIALIZATION_FAILED;
  }
  udev_monitor_filter_add_match_subsystem_devtype(extension->pUdevMonitor, "sound", NULL);
  udev_monitor_enable_receiving(extension->pUdevMonitor);
  //int fd = udev_monitor_get_fd(extension->pUdevMonitor);

  return SK_SUCCESS;
}

static SkExtensionPropertiesInternal SkSupportedExtensions[] = {
  { "SK_SEA_device_polling", 1, &skConstructExtension_device_polling_SEA }
};
#define SK_EXTENSION_PROPERTIES_INTERNAL_COUNT sizeof(SkSupportedExtensions) / sizeof(SkExtensionProperties)

////////////////////////////////////////////////////////////////////////////////
// Implementation Functions (Standard)
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skEnumerateInstanceExtensionProperties(
  void*                               pReserved,
  uint32_t*                           pExtensionCount,
  SkExtensionProperties*              pExtensionProperties
) {
  (void)pReserved;
  uint32_t max = SK_EXTENSION_PROPERTIES_INTERNAL_COUNT;
  if (!pExtensionProperties) {
    *pExtensionCount = max;
  }
  else {
    for (uint32_t idx = 0; idx < *pExtensionCount; ++idx) {
      if (idx >= max) {
        *pExtensionCount = max;
        return SK_SUCCESS;
      }
      strcpy(pExtensionProperties[idx].extensionName, SkSupportedExtensions[idx].extensionName);
      pExtensionProperties[idx].specVersion = SkSupportedExtensions[idx].specVersion;
    }
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
    const SkInstanceCreateInfo*         pCreateInfo,
    const SkAllocationCallbacks*        pAllocator,
    SkInstance*                         pInstance
) {
  if (!pAllocator) pAllocator = &SkDefaultAllocationCallbacks;

  SkInstance instance =
  (SkInstance)pAllocator->pfnAllocation(
    pAllocator->pUserData,
    sizeof(SkInstance_T),
    0,
    SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE
  );
  if (!instance) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Configure instance
  instance->pPhysicalDevices = NULL;
  instance->pAllocator = pAllocator;
  instance->extension_device_polling_SEA.initialized = SK_FALSE;

  // Enable extensions
  SkResult result;
  for (uint32_t idx = 0; idx < pCreateInfo->enabledExtensionCount; ++idx) {
    result = SK_ERROR_EXTENSION_NOT_PRESENT;

    // Attempt to see if the extension is supported
    for (uint32_t ddx = 0; ddx < SK_EXTENSION_PROPERTIES_INTERNAL_COUNT; ++ddx) {
      if (strcmp(pCreateInfo->ppEnabledExtensionNames[idx], SkSupportedExtensions[idx].extensionName) == 0) {
        result = SkSupportedExtensions[ddx].constructExtension(instance);
        break;
      }
    }

    // Check that the extension was initialized properly
    if (result != SK_SUCCESS) {
      skDestroyInstance(instance);
      return result;
    }

  }

  // Return instance
  *pInstance = instance;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyInstance(
    SkInstance                          instance
) {
  if (instance->extension_device_polling_SEA.initialized) {
    udev_unref(instance->extension_device_polling_SEA.pUdev);
  }
  SkPhysicalDevice currPhysicalDevice = instance->pPhysicalDevices;
  SkPhysicalDevice nextPhysicalDevice;
  while (nextPhysicalDevice) {
    nextPhysicalDevice = currPhysicalDevice->next;
    snd_ctl_card_info_free(currPhysicalDevice->pCardInfo);
    instance->pAllocator->pfnFree(instance->pAllocator->pUserData, currPhysicalDevice);
    currPhysicalDevice = nextPhysicalDevice;
  }
  instance->pAllocator->pfnFree(instance->pAllocator->pUserData, instance);
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkInstance                          instance,
  uint32_t*                           pPhysicalDeviceCount,
  SkPhysicalDevice*                   pPhysicalDevices
) {
  uint32_t count;
  SkResult result = SK_SUCCESS;
  SkPhysicalDevice  prevPhysicalDevice;
  SkPhysicalDevice_T** ppPhysicalDevice = &instance->pPhysicalDevices;
  uint32_t max = (pPhysicalDevices) ? *pPhysicalDeviceCount : UINT32_MAX;

  int card = -1;
  for (count = 0; count < max; ++count) {
    // Attempt to grab the next device
    if (snd_card_next(&card) != 0) {
      return SK_ERROR_FAILED_ENUMERATING_PHYSICAL_DEVICES;
    }
    if (card == -1) break; // Last device

    // Construct the physical device pointer
    if (!*ppPhysicalDevice) {
      prevPhysicalDevice =
      (SkPhysicalDevice)instance->pAllocator->pfnAllocation(
        instance->pAllocator->pUserData,
        sizeof(SkPhysicalDevice_T),
        0,
        SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
      );

      snd_ctl_card_info_malloc(&prevPhysicalDevice->pCardInfo);
      if (!prevPhysicalDevice->pCardInfo) {
        instance->pAllocator->pfnFree(instance->pAllocator->pUserData, prevPhysicalDevice);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }

      sprintf(prevPhysicalDevice->identifier, "hw:%d", card);
      prevPhysicalDevice->instance = instance;
      prevPhysicalDevice->next = NULL;
      prevPhysicalDevice->card = card;
      *ppPhysicalDevice = prevPhysicalDevice;
      ppPhysicalDevice = &prevPhysicalDevice->next;
    }
    else {
      prevPhysicalDevice = *ppPhysicalDevice;
      ppPhysicalDevice = &prevPhysicalDevice->next;
    }

    snd_ctl_t *handle;
    if (snd_ctl_open(&handle, prevPhysicalDevice->identifier, 0) != 0) {
      result = SK_INCOMPLETE;
      continue;
    }

    if (snd_ctl_card_info(handle, prevPhysicalDevice->pCardInfo) != 0) {
      snd_ctl_close(handle);
      result = SK_INCOMPLETE;
      continue;
    }

    snd_ctl_close(handle);
  }
  *pPhysicalDeviceCount = count;

  // Copy physical device handle into destination
  if (pPhysicalDevices) {
    ppPhysicalDevice = &instance->pPhysicalDevices;
    for (uint32_t idx = 0; idx < count; ++idx) {
      pPhysicalDevices[idx] = *ppPhysicalDevice;
      ppPhysicalDevice = &(*ppPhysicalDevice)->next;
    }
  }

  return result;
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalDeviceProperties(
  SkPhysicalDevice                    physicalDevice,
  SkPhysicalDeviceProperties*         pProperties
) {
  snd_ctl_card_info_t *info = physicalDevice->pCardInfo;
  pProperties->available = snd_card_load(physicalDevice->card) ? SK_TRUE : SK_FALSE;
  strncpy(pProperties->deviceName, snd_ctl_card_info_get_name(info), SK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
  strncpy(pProperties->driverName, snd_ctl_card_info_get_driver(info), SK_MAX_DRIVER_NAME_SIZE);
  strncpy(pProperties->mixerName, snd_ctl_card_info_get_mixername(info), SK_MAX_MIXER_NAME_SIZE);
}

////////////////////////////////////////////////////////////////////////////////
// Extension: SK_SEA_device_polling
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skRegisterDeviceCallbackSEA(
  SkInstance                        instance,
  SkDeviceEventTypeSEA              callbackEvents,
  PFN_skDeviceCallbackSEA           pfnEventCallback,
  void*                             pUserData
) {
  // Not currently implemented
}
