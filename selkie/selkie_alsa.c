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

#define SK_ALSA_MAX_IDENTIFIER_NAME_SIZE 32
#define SK_ALSA_MAX_HINT_NAME_SIZE 256

////////////////////////////////////////////////////////////////////////////////
// Implementation Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkLimitsContainer_IMPL {
  SkComponentLimits             capture;
  SkComponentLimits             playback;
} SkLimitsContainer_IMPL;

typedef struct SkExtensionData_device_polling_SEA {
  SkBool32                      initialized;
  struct udev*                  pUdev;
  struct udev_monitor*          pUdevMonitor;
} SkExtensionData_device_polling_SEA;

typedef struct SkPhysicalComponent_T {
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_NAME_SIZE];
  int                           device;
  struct SkPhysicalComponent_T* next;
  SkComponentProperties         properties;
  SkLimitsContainer_IMPL        limits;
  SkPhysicalDevice              physicalDevice;
} SkPhysicalComponent_T;

typedef struct SkPhysicalDevice_T {
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_NAME_SIZE];
  SkInstance                    instance;
  struct SkPhysicalDevice_T*    next;
  struct SkPhysicalComponent_T* pComponents;
  SkDeviceProperties            properties;
  int                           card;
} SkPhysicalDevice_T;

typedef struct SkVirtualComponent_T {
  SkComponentProperties         properties;
  struct SkVirtualComponent_T*  next;
  SkVirtualDevice               virtualDevice;
} SkVirtualComponent_T;

typedef struct SkVirtualDevice_T {
  char                          name[SK_ALSA_MAX_HINT_NAME_SIZE];
  SkInstance                    instance;
  struct SkVirtualDevice_T*     next;
  struct SkVirtualComponent_T*  pComponents;
  SkDeviceProperties            properties;
} SkVirtualDevice_T;

typedef struct SkInstance_T {
  const SkAllocationCallbacks*  pAllocator;
  SkPhysicalDevice_T*           pPhysicalDevices;
  SkVirtualDevice_T*            pVirtualDevices;
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
snd_pcm_stream_t toLocalPcmStreamType(SkStreamType type) {
  switch (type) {
    case SK_STREAM_TYPE_CAPTURE:
      return SND_PCM_STREAM_CAPTURE;
    case SK_STREAM_TYPE_PLAYBACK:
      return SND_PCM_STREAM_PLAYBACK;
    default:
      break;
  }
  // Default to PCM Playback
  return SND_PCM_STREAM_PLAYBACK;
}

static SkResult skConstructExtension_device_polling_SEA(SkInstance instance) {
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

static SkPhysicalDevice skGetPhysicalDeviceByCard(SkInstance instance, int card) {
  SkPhysicalDevice device = instance->pPhysicalDevices;
  while (device) {
    if (device->card == card) return device;
    device = device->next;
  }
  return NULL;
}

static SkPhysicalComponent skGetPhysicalComponentByDevice(SkPhysicalDevice physicalDevice, int device) {
  SkPhysicalComponent component = physicalDevice->pComponents;
  while (component) {
    if (component->device == device) return component;
    component = component->next;
  }
  return NULL;
}

static SkVirtualDevice skGetVirtualDeviceByName(SkInstance instance, char const *name) {
  SkVirtualDevice device = instance->pVirtualDevices;
  while (device) {
    if (strcmp(device->name, name) == 0) return device;
    device = device->next;
  }
  return NULL;
}

static SkVirtualComponent skGetVirtualComponentByName(SkVirtualDevice virtualDevice, char const *name) {
  SkVirtualComponent component = virtualDevice->pComponents;
  while (component) {
    if (strcmp(component->properties.componentName, name) == 0) return component;
    component = component->next;
  }
  return NULL;
}

static void skDestructPhysicalDevice(SkPhysicalDevice device) {
  device->instance->pAllocator->pfnFree(
    device->instance->pAllocator->pUserData,
    device
  );
}

static void assignRangeAndDirection(SkRangedValue *range, uint32_t value, int dir) {
  range->value = value;
  switch (dir) {
    case -1:
      range->direction = SK_RANGE_DIRECTION_LESS;
      break;
    case  0:
      range->direction = SK_RANGE_DIRECTION_EQUAL;
      break;
    case  1:
      range->direction = SK_RANGE_DIRECTION_GREATER;
      break;
    default:
      range->direction = SK_RANGE_DIRECTION_UNKNOWN;
      break;
  }
}

static SkResult skPopulateComponentLimits(snd_pcm_t *handle, SkComponentLimits *limits) {
  int iValue;
  unsigned int uiValue;
  snd_pcm_uframes_t uframesValue;

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);
  if (snd_pcm_hw_params_any(handle, params) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  if (snd_pcm_hw_params_get_buffer_size_max(params, &uframesValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_size_min(params, &uframesValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_time_max(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxBufferTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_buffer_time_min(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minBufferTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_channels_max(params, &uiValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxChannels = uiValue;

  if (snd_pcm_hw_params_get_channels_min(params, &uiValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minChannels = uiValue;

  if (snd_pcm_hw_params_get_period_size_max(params, &uframesValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriodSize, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_size_min(params, &uframesValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriodSize, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_time_max(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriodTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_time_min(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriodTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_periods_max(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriods, uiValue, iValue);

  if (snd_pcm_hw_params_get_periods_min(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriods, uiValue, iValue);

  if (snd_pcm_hw_params_get_rate_max(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxRate, uiValue, iValue);

  if (snd_pcm_hw_params_get_rate_min(params, &uiValue, &iValue) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minRate, uiValue, iValue);

  return SK_SUCCESS;
}

static SkResult skConstructPhysicalComponent(SkPhysicalDevice physicalDevice, snd_ctl_t *ctlHandle, int device, snd_pcm_stream_t type) {
  SkPhysicalComponent component = skGetPhysicalComponentByDevice(physicalDevice, device);

  if (!component) {
    component =
    (SkPhysicalComponent)physicalDevice->instance->pAllocator->pfnAllocation(
      physicalDevice->instance->pAllocator->pUserData,
      sizeof(SkPhysicalComponent_T),
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
    component->physicalDevice = physicalDevice;

    // Put the component on the end to preserve component order
    SkPhysicalComponent *parentComponent = &physicalDevice->pComponents;
    while (*parentComponent) {
      parentComponent = &(*parentComponent)->next;
    }
    (*parentComponent) = component;
  }

  SkComponentProperties *pProperties = &component->properties;
  snd_pcm_info_t *info;
  snd_pcm_info_alloca(&info);
  snd_pcm_info_set_device(info, device);
  snd_pcm_info_set_subdevice(info, 0);
  snd_pcm_info_set_stream(info, type);
  if (snd_ctl_pcm_info(ctlHandle, info) != 0) {
    // Note: This means that the stream type is not supported.
    return SK_SUCCESS;
  }
  strncpy(pProperties->componentName, snd_pcm_info_get_name(info), SK_MAX_COMPONENT_NAME_SIZE);

  snd_pcm_t *handle;
  if (snd_pcm_open(&handle, component->identifier, type, 0) != 0) {
    // Note: This means that this device is not available.
    return SK_SUCCESS;
  }

  SkComponentLimits *limits;
  switch (type) {
    case SND_PCM_STREAM_PLAYBACK:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PLAYBACK;
      limits = &component->limits.playback;
      break;
    case SND_PCM_STREAM_CAPTURE:
      component->properties.supportedStreams |= SK_STREAM_TYPE_CAPTURE;
      limits = &component->limits.capture;
      break;
  }

  SkResult result = skPopulateComponentLimits(handle, limits);

  snd_pcm_close(handle);

  return result;
}

static SkResult skPopulatePhysicalDeviceComponent(SkPhysicalDevice physicalDevice, snd_pcm_stream_t type) {
  snd_ctl_t *handle;
  if (snd_ctl_open(&handle, physicalDevice->identifier, type) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  // Populate device properties
  SkDeviceProperties *pProperties = &physicalDevice->properties;
  snd_ctl_card_info_t *info;
  snd_ctl_card_info_alloca(&info);
  if (snd_ctl_card_info(handle, info)) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  strncpy(pProperties->deviceName, snd_ctl_card_info_get_name(info), SK_MAX_DEVICE_NAME_SIZE);
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

    SkResult tempResult = skConstructPhysicalComponent(physicalDevice, handle, device, type);
    if (tempResult != SK_SUCCESS) {
      result = tempResult;
    }
  }

  snd_ctl_close(handle);

  return result;
}

static SkResult skConstructPhysicalDevice(SkInstance instance, int card) {
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
  result = skPopulatePhysicalDeviceComponent(physicalDevice, SND_PCM_STREAM_PLAYBACK);
  if (result != SK_SUCCESS) {
    overallResult = SK_INCOMPLETE;
  }
  result = skPopulatePhysicalDeviceComponent(physicalDevice, SND_PCM_STREAM_CAPTURE);
  if (result != SK_SUCCESS) {
    overallResult = SK_INCOMPLETE;
  }

  return overallResult;
}

static SkResult skPopulateVirtualDeviceComponent(SkVirtualDevice virtualDevice, char const *name, snd_pcm_stream_t type) {
  SkVirtualComponent component = skGetVirtualComponentByName(virtualDevice, name);

  if (!component) {
    component =
    (SkVirtualComponent)virtualDevice->instance->pAllocator->pfnAllocation(
      virtualDevice->instance->pAllocator->pUserData,
      sizeof(SkVirtualComponent_T),
      0,
      SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
    );

    if (!component) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill in the initial meta-information
    component->properties.supportedStreams = 0;
    component->virtualDevice = virtualDevice;
    component->next = NULL;
    strncpy(component->properties.componentName, name, SK_MAX_COMPONENT_NAME_SIZE);

    // Put the component on the end to preserve component order
    SkVirtualComponent *parentComponent = &virtualDevice->pComponents;
    while (*parentComponent) {
      parentComponent = &(*parentComponent)->next;
    }
    (*parentComponent) = component;
  }

  // Note: Virtual devices don't support hardware limits.
  switch (type) {
    case SND_PCM_STREAM_PLAYBACK:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PLAYBACK;
      break;
    case SND_PCM_STREAM_CAPTURE:
      component->properties.supportedStreams |= SK_STREAM_TYPE_CAPTURE;
      break;
  }

  return SK_SUCCESS;
}

static SkResult skConstructVirtualDeviceFromName(SkInstance instance, char const *fullName, char const *deviceName, char const *ioid) {
  SkVirtualDevice virtualDevice = skGetVirtualDeviceByName(instance, deviceName);

  // Construct the device if it doesn't exist
  if (!virtualDevice) {
    virtualDevice =
        (SkVirtualDevice)instance->pAllocator->pfnAllocation(
            instance->pAllocator->pUserData,
            sizeof(SkVirtualDevice_T),
            0,
            SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
        );

    if (!virtualDevice) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill initial meta-information
    // Note: Since virtual devices don't have specific drivers/mixers,
    //       these values should almost always be set to 'N/A'.
    strcpy(virtualDevice->name, deviceName);
    strcpy(virtualDevice->properties.deviceName, deviceName);
    strcpy(virtualDevice->properties.driverName, "N/A");
    strcpy(virtualDevice->properties.mixerName, "N/A");
    virtualDevice->instance = instance;
    virtualDevice->pComponents = NULL;
    virtualDevice->next = NULL;

    // Put the device on the end to preserve component order
    SkVirtualDevice *prevDevice = &instance->pVirtualDevices;
    while (*prevDevice) {
      prevDevice = &(*prevDevice)->next;
    }
    (*prevDevice) = virtualDevice;
  }

  // Check stream information
  // Note: In ALSA, a NULL IOID means it supports both input and output.
  SkResult result;
  SkResult overallResult = SK_SUCCESS;
  if (!ioid || strcmp(ioid, "Output") == 0) {
    result = skPopulateVirtualDeviceComponent(virtualDevice, fullName, SND_PCM_STREAM_PLAYBACK);
    if (result != SK_SUCCESS) {
      overallResult = SK_INCOMPLETE;
    }
  }
  if (!ioid || strcmp(ioid, "Input") == 0) {
    result = skPopulateVirtualDeviceComponent(virtualDevice, fullName, SND_PCM_STREAM_CAPTURE);
    if (result != SK_SUCCESS) {
      overallResult = SK_INCOMPLETE;
    }
  }

  return overallResult;
}

static SkResult skConstructVirtualDevice(SkInstance instance, void *hint) {
  // Get information from the device
  char *name, *desc, *ioid;
  name = snd_device_name_get_hint(hint, "NAME");
  desc = snd_device_name_get_hint(hint, "DESC");
  ioid = snd_device_name_get_hint(hint, "IOID");

  // Note: In ALSA, we form Virtual Devices from the first
  //       part of the stream name. (Everything before ':')
  char *deviceName = strdup(name);
  char *colonPtr = deviceName;
  while (*colonPtr && *colonPtr != ':') ++colonPtr;
  *colonPtr = '\0';

  return skConstructVirtualDeviceFromName(instance, name, deviceName, ioid);
}

static SkExtensionPropertiesInternal SkSupportedExtensions[] = {
  //{ "SK_SEA_device_polling", 1, &skConstructExtension_device_polling_SEA }
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
  // Note: Construct default virtual device so that it's first in the list.
  *pInstance = instance;
  skConstructVirtualDeviceFromName(instance, "default", "default", NULL);

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
  SkDeviceProperties*                 pProperties
) {
  *pProperties = physicalDevice->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalComponents(
  SkPhysicalDevice                  physicalDevice,
  uint32_t*                         pPhysicalComponentCount,
  SkPhysicalComponent*              pPhysicalComponents
) {

  uint32_t count = 0;
  uint32_t max = (pPhysicalComponents) ? *pPhysicalComponentCount : UINT32_MAX;
  SkPhysicalComponent component = physicalDevice->pComponents;
  for (; count < max; ++count) {
    if (!component) break;
    if (pPhysicalComponents) {
      pPhysicalComponents[count] = component;
    }
    component = component->next;
  }
  *pPhysicalComponentCount = count;

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalComponentProperties(
  SkPhysicalComponent               physicalComponent,
  SkComponentProperties*            pProperties
) {
  *pProperties = physicalComponent->properties;
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalComponentLimits(
  SkPhysicalComponent               physicalComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  switch (streamType) {
    case SK_STREAM_TYPE_PLAYBACK:
      *pLimits = physicalComponent->limits.playback;
      break;
    case SK_STREAM_TYPE_CAPTURE:
      *pLimits = physicalComponent->limits.capture;
      break;
  }
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
    SkInstance                        instance,
    uint32_t*                         pVirtualDeviceCount,
    SkVirtualDevice*                  pVirtualDevices
) {
  SkResult result = SK_SUCCESS;
  uint32_t count = 0;

  // Refresh the list of virtual devices
  {
    void **hints, **hint;
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    hint = hints;

    while (*hint) {
      SkResult tempResult = skConstructVirtualDevice(instance, *hint);
      if (tempResult != SK_SUCCESS) {
        result = tempResult;
      }
      ++hint;
    }
  }

  // Only get counts
  SkVirtualDevice virtualDevices = instance->pVirtualDevices;
  if (!pVirtualDevices) {
    while (virtualDevices) {
      ++count;
      virtualDevices = virtualDevices->next;
    }
  }
    // Construct physical device handles
  else {
    while (virtualDevices) {
      pVirtualDevices[count] = virtualDevices;
      ++count;
      virtualDevices = virtualDevices->next;
    }
  }
  *pVirtualDeviceCount = count;

  return result;
}

SKAPI_ATTR void SKAPI_CALL skGetVirtualDeviceProperties(
  SkVirtualDevice                   virtualDevice,
  SkDeviceProperties*               pProperties
) {
  *pProperties = virtualDevice->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualComponents(
  SkVirtualDevice                   virtualDevice,
  uint32_t*                         pVirtualComponentCount,
  SkVirtualComponent*               pVirtualComponents
) {

  uint32_t count = 0;
  uint32_t max = (pVirtualComponents) ? *pVirtualComponentCount : UINT32_MAX;
  SkVirtualComponent component = virtualDevice->pComponents;
  for (; count < max; ++count) {
    if (!component) break;
    if (pVirtualComponents) {
      pVirtualComponents[count] = component;
    }
    component = component->next;
  }
  *pVirtualComponentCount = count;

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skGetVirtualComponentProperties(
  SkVirtualComponent                virtualComponent,
  SkComponentProperties*            pProperties
) {
  *pProperties = virtualComponent->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skResolvePhysicalComponent(
  SkVirtualComponent                virtualComponent,
  SkStreamType                      streamType,
  SkPhysicalComponent*              pPhysicalComponent
) {
  snd_pcm_t *handle;
  if (snd_pcm_open(&handle, virtualComponent->properties.componentName, toLocalPcmStreamType(streamType), 0) != 0) {
    return SK_ERROR_FAILED_RESOLVING_DEVICE;
  }

  snd_pcm_info_t *info;
  snd_pcm_info_alloca(&info);
  if (snd_pcm_info(handle, info) != 0) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_RESOLVING_DEVICE;
  }

  SkPhysicalDevice physicalDevice = skGetPhysicalDeviceByCard(virtualComponent->virtualDevice->instance, snd_pcm_info_get_card(info));
  if (!physicalDevice) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_RESOLVING_DEVICE;
  }
  SkPhysicalComponent physicalComponent = skGetPhysicalComponentByDevice(physicalDevice, snd_pcm_info_get_device(info));
  if (!physicalComponent) {
    snd_pcm_close(handle);
    return SK_ERROR_FAILED_RESOLVING_DEVICE;
  }

  *pPhysicalComponent = physicalComponent;
  snd_pcm_close(handle);
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skResolvePhysicalDevice(
  SkPhysicalComponent               physicalComponent,
  SkPhysicalDevice*                 pPhysicalDevice
) {
  *pPhysicalDevice = physicalComponent->physicalDevice;
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
  return SK_ERROR_NOT_IMPLEMENTED;
}
