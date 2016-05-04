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

typedef struct SkDeviceComponent_T {
  char                          identifier[32];
  int                           device;
  struct SkDeviceComponent_T*   next;
  SkDeviceComponentProperties   properties;
  SkDeviceComponentLimits       inputLimits;
  SkDeviceComponentLimits       outputLimits;
} SkDeviceComponent_T;

typedef struct SkPhysicalDevice_T {
  char                          identifier[32];
  SkInstance                    instance;
  struct SkPhysicalDevice_T*    next;
  struct SkDeviceComponent_T*   pComponents;
  SkPhysicalDeviceProperties    properties;
  int                           card;
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

SkPhysicalDevice skGetPhysicalDeviceByCard(SkInstance instance, int card) {
  SkPhysicalDevice device = instance->pPhysicalDevices;
  while (device) {
    if (device->card == card) return device;
    device = device->next;
  }
  return NULL;
}

SkDeviceComponent skGetDeviceComponentByDevice(SkPhysicalDevice physicalDevice, int device) {
  SkDeviceComponent component = physicalDevice->pComponents;
  while (component) {
    if (component->device == device) return component;
    component = component->next;
  }
  return NULL;
}

void skDestructPhysicalDevice(SkPhysicalDevice device) {
  device->instance->pAllocator->pfnFree(
    device->instance->pAllocator->pUserData,
    device
  );
}

SkResult skConstructDeviceComponent(SkPhysicalDevice physicalDevice, snd_ctl_t *ctlHandle, int device, snd_pcm_stream_t type) {
  SkDeviceComponent component = skGetDeviceComponentByDevice(physicalDevice, device);

  if (!component) {
    component =
    (SkDeviceComponent)physicalDevice->instance->pAllocator->pfnAllocation(
      physicalDevice->instance->pAllocator->pUserData,
      sizeof(SkDeviceComponent_T),
      0,
      SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
    );

    if (!component) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill in the initial meta-information
    sprintf(component->identifier, "hw:%d,%d", physicalDevice->card, device);
    component->properties.supportedStreams = 0;
    component->device = device;
    component->next = NULL;

    // Put the component on the end to preserve component order
    SkDeviceComponent *parentComponent = &physicalDevice->pComponents;
    while (*parentComponent) {
      parentComponent = &(*parentComponent)->next;
    }
    (*parentComponent) = component;
  }

  SkDeviceComponentProperties *pProperties = &component->properties;
  snd_pcm_info_t *info;
  snd_pcm_info_alloca(&info);
  snd_pcm_info_set_device(info, device);
  snd_pcm_info_set_subdevice(info, 0);
  snd_pcm_info_set_stream(info, type);
  if (snd_ctl_pcm_info(ctlHandle, info) != 0) {
    // Note: This means that the stream is not supported.
    return SK_SUCCESS;
  }
  strncpy(pProperties->componentName, snd_pcm_info_get_name(info), SK_MAX_DEVICE_COMPONENT_NAME_SIZE);

  snd_pcm_t *handle;
  if (snd_pcm_open(&handle, component->identifier, type, 0) != 0) {
    // Note: This means that this device is not available.
    return SK_SUCCESS;
  }

  SkDeviceComponentLimits *limits;
  switch (type) {
    case SND_PCM_STREAM_PLAYBACK:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PLAYBACK;
      limits = &component->outputLimits;
      break;
    case SND_PCM_STREAM_CAPTURE:
      component->properties.supportedStreams |= SK_STREAM_TYPE_CAPTURE;
      limits = &component->inputLimits;
      break;
  }

  int iValue;
  unsigned int uiValue;
  snd_pcm_uframes_t uframesValue;

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);
  if (snd_pcm_hw_params_any(handle, params) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  if (snd_pcm_hw_params_get_buffer_size_max(params, &uframesValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_size_min(params, &uframesValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_time_max(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxBufferTime.value = uiValue;
  switch (iValue) {
    case -1:
      limits->maxBufferTime.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->maxBufferTime.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->maxBufferTime.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_buffer_time_min(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minBufferTime.value = uiValue;
  switch (iValue) {
    case -1:
      limits->minBufferTime.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->minBufferTime.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->minBufferTime.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_channels_max(params, &uiValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxChannels = uiValue;

  if (snd_pcm_hw_params_get_channels_min(params, &uiValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minChannels = uiValue;

  if (snd_pcm_hw_params_get_period_size_max(params, &uframesValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxPeriodSize.value = uframesValue;
  switch (iValue) {
    case -1:
      limits->maxPeriodSize.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->maxPeriodSize.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->maxPeriodSize.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_period_size_min(params, &uframesValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minPeriodSize.value = uframesValue;
  switch (iValue) {
    case -1:
      limits->minPeriodSize.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->minPeriodSize.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->minPeriodSize.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_period_time_max(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxPeriodTime.value = uiValue;
  switch (iValue) {
    case -1:
      limits->maxPeriodTime.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->maxPeriodTime.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->maxPeriodTime.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_period_time_min(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minPeriodTime.value = uiValue;
  switch (iValue) {
    case -1:
      limits->minPeriodTime.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->minPeriodTime.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->minPeriodTime.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_periods_max(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxPeriods.value = uiValue;
  switch (iValue) {
    case -1:
      limits->maxPeriods.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->maxPeriods.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->maxPeriods.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_periods_min(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minPeriods.value = uiValue;
  switch (iValue) {
    case -1:
      limits->minPeriods.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->minPeriods.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->minPeriods.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_rate_max(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxRate.value = uiValue;
  switch (iValue) {
    case -1:
      limits->maxRate.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->maxRate.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->maxRate.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  if (snd_pcm_hw_params_get_rate_min(params, &uiValue, &iValue) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minRate.value = uiValue;
  switch (iValue) {
    case -1:
      limits->minRate.direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      limits->minRate.direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      limits->minRate.direction = SK_RANGE_DIRECTION_GREATER;
      break;
  }

  snd_pcm_close(handle);

}

SkResult skPopulateDeviceComponent(SkPhysicalDevice physicalDevice, snd_pcm_stream_t type) {
  snd_ctl_t *handle;
  if (snd_ctl_open(&handle, physicalDevice->identifier, type) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  // Populate device properties
  SkPhysicalDeviceProperties *pProperties = &physicalDevice->properties;
  snd_ctl_card_info_t *info;
  snd_ctl_card_info_alloca(&info);
  if (snd_ctl_card_info(handle, info)) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  pProperties->available = snd_card_load(physicalDevice->card) ? SK_TRUE : SK_FALSE;
  strncpy(pProperties->deviceName, snd_ctl_card_info_get_name(info), SK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
  strncpy(pProperties->driverName, snd_ctl_card_info_get_driver(info), SK_MAX_DRIVER_NAME_SIZE);
  strncpy(pProperties->mixerName, snd_ctl_card_info_get_mixername(info), SK_MAX_MIXER_NAME_SIZE);

  // handle pcm streams
  SkResult result = SK_SUCCESS;
  int device = -1;
  for (;;) {
    if (snd_ctl_pcm_next_device(handle, &device) != 0) {
      snd_ctl_close(handle);
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    if (device == -1) break;

    SkResult tempResult = skConstructDeviceComponent(physicalDevice, handle, device, type);
    if (tempResult != SK_SUCCESS) {
      result = tempResult;
    }
  }

  snd_ctl_close(handle);

  return result;
}

SkResult skConstructPhysicalDevice(SkInstance instance, int card) {
  SkPhysicalDevice physicalDevice = skGetPhysicalDeviceByCard(instance, card);

  if (!physicalDevice) {
    physicalDevice =
    (SkPhysicalDevice)instance->pAllocator->pfnAllocation(
      instance->pAllocator->pUserData,
      sizeof(SkPhysicalDevice_T),
      0,
      SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
    );

    if (!physicalDevice) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill initial meta-information
    sprintf(physicalDevice->identifier, "hw:%d", card);
    physicalDevice->instance = instance;
    physicalDevice->pComponents = NULL;
    physicalDevice->card = card;
    physicalDevice->next = instance->pPhysicalDevices;
    instance->pPhysicalDevices = physicalDevice;
  }

  // Check stream information
  SkResult result;
  SkResult overallResult = SK_SUCCESS;
  result = skPopulateDeviceComponent(physicalDevice, SND_PCM_STREAM_PLAYBACK);
  if (result != SK_SUCCESS) {
    overallResult = SK_INCOMPLETE;
  }
  result = skPopulateDeviceComponent(physicalDevice, SND_PCM_STREAM_CAPTURE);
  if (result != SK_SUCCESS) {
    overallResult = SK_INCOMPLETE;
  }

  return overallResult;
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
  SkResult result = SK_SUCCESS;
  int card = -1;
  uint32_t count = 0;

  // Only get counts
  if (!pPhysicalDevices) {
    for(;;) {
      // Attempt to grab the next device
      if (snd_card_next(&card) != 0) {
        return SK_ERROR_FAILED_QUERYING_DEVICE;
      }
      if (card == -1) break;
      ++count;
    }
  }
  // Construct physical device handles
  else {
    uint32_t max = *pPhysicalDeviceCount;
    while (count < max) {
      // Attempt to grab the next device
      if (snd_card_next(&card) != 0) {
        return SK_ERROR_FAILED_QUERYING_DEVICE;
      }
      if (card == -1) break; // Last card

      // Attempt to construct the new card handle
      SkResult tempResult = skConstructPhysicalDevice(instance, card);
      if (tempResult != SK_SUCCESS) {
        result = tempResult;
      }
      else {
        pPhysicalDevices[count] = skGetPhysicalDeviceByCard(instance, card);
        ++count;
      }
    }
  }
  *pPhysicalDeviceCount = count;

  return result;
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalDeviceProperties(
  SkPhysicalDevice                    physicalDevice,
  SkPhysicalDeviceProperties*         pProperties
) {
  *pProperties = physicalDevice->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDeviceComponents(
  SkPhysicalDevice                  physicalDevice,
  uint32_t*                         pDeviceComponentCount,
  SkDeviceComponent*                pDeviceComponents
) {

  uint32_t count = 0;
  uint32_t max = (pDeviceComponents) ? *pDeviceComponentCount : UINT32_MAX;
  SkDeviceComponent component = physicalDevice->pComponents;
  for (; count < max; ++count) {
    if (!component) break;
    if (pDeviceComponents) {
      pDeviceComponents[count] = component;
    }
    component = component->next;
  }
  *pDeviceComponentCount = count;

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skGetDeviceComponentProperties(
  SkDeviceComponent                 deviceComponent,
  SkDeviceComponentProperties*      pProperties
) {
  *pProperties = deviceComponent->properties;
}

SKAPI_ATTR void SKAPI_CALL skGetDeviceComponentLimits(
  SkDeviceComponent                 deviceComponent,
  SkStreamType                      streamType,
  SkDeviceComponentLimits*          pLimits
) {
  switch (streamType) {
    case SK_STREAM_TYPE_PLAYBACK:
      *pLimits = deviceComponent->outputLimits;
      break;
    case SK_STREAM_TYPE_CAPTURE:
      *pLimits = deviceComponent->inputLimits;
      break;
  }
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
