/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard implementation. (ALSA Implementation)
 ******************************************************************************/

#include <OpenSK/opensk.h>
#include <OpenSK/dev/hosts.h>
#include <OpenSK/dev/macros.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

#define SK_ALSA_MAX_IDENTIFIER_NAME_SIZE 32
#define SK_ALSA_MAX_HINT_NAME_SIZE 256
#define SK_ALSA_MAX_HINT_DESC_SIZE 256
#define SK_ALSA_MAX_HINT_IOID_SIZE 64

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkLimitsContainerIMPL {
  SkComponentLimits             capture;
  SkComponentLimits             playback;
} SkLimitsContainerIMPL;

#define host_cast(h) ((SkHostApi_ALSA*)h)
typedef struct SkHostApi_ALSA {
  SkHostApi_T                   parent;
} SkHostApi_ALSA;

#define pdev_cast(d) ((SkPhysicalDevice_ALSA*)d)
typedef struct SkPhysicalDevice_ALSA {
  SkPhysicalDevice_T            parent;
  int                           card;
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_NAME_SIZE];
} SkPhysicalDevice_ALSA;

#define pcomp_cast(c) ((SkPhysicalComponent_ALSA*)c)
typedef struct SkPhysicalComponent_ALSA {
  SkPhysicalComponent_T         parent;
  unsigned int                  device;
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_NAME_SIZE];
} SkPhysicalComponent_ALSA;

#define vdev_cast(d) ((SkVirtualDevice_ALSA*)d)
typedef struct SkVirtualDevice_ALSA {
  SkVirtualDevice_T             parent;
} SkVirtualDevice_ALSA;

#define vcomp_cast(c) ((SkVirtualComponent_ALSA*)c)
typedef struct SkVirtualComponent_ALSA {
  SkVirtualComponent_T          parent;
  unsigned int                  device;
  char                          name[SK_ALSA_MAX_HINT_NAME_SIZE];
} SkVirtualComponent_ALSA;

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////
static void
assignRangeAndDirection(SkRangedValue *range, uint32_t value, int dir) {
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

static snd_pcm_stream_t
toLocalPcmStreamType(SkStreamType type) {
  switch (type) {
    case SK_STREAM_TYPE_PCM_CAPTURE:
      return SND_PCM_STREAM_CAPTURE;
    case SK_STREAM_TYPE_PCM_PLAYBACK:
      return SND_PCM_STREAM_PLAYBACK;
    default:
      break;
  }
  // Default to PCM Playback
  return SND_PCM_STREAM_PLAYBACK;
}

static snd_pcm_format_t
toLocalPcmFormatType(SkFormat type) {
  switch (type) {
    case SK_FORMAT_S8:
      return SND_PCM_FORMAT_S8;
    case SK_FORMAT_U8:
      return SND_PCM_FORMAT_U8;
    case SK_FORMAT_S16_LE:
      return SND_PCM_FORMAT_S16_LE;
    case SK_FORMAT_S16_BE:
      return SND_PCM_FORMAT_S16_BE;
    case SK_FORMAT_U16_LE:
      return SND_PCM_FORMAT_U16_LE;
    case SK_FORMAT_U16_BE:
      return SND_PCM_FORMAT_U16_BE;
    case SK_FORMAT_S24_LE:
      return SND_PCM_FORMAT_S24_LE;
    case SK_FORMAT_S24_BE:
      return SND_PCM_FORMAT_S24_BE;
    case SK_FORMAT_U24_LE:
      return SND_PCM_FORMAT_U24_LE;
    case SK_FORMAT_U24_BE:
      return SND_PCM_FORMAT_U24_BE;
    case SK_FORMAT_S32_LE:
      return SND_PCM_FORMAT_S32_LE;
    case SK_FORMAT_S32_BE:
      return SND_PCM_FORMAT_S32_BE;
    case SK_FORMAT_U32_LE:
      return SND_PCM_FORMAT_U32_LE;
    case SK_FORMAT_U32_BE:
      return SND_PCM_FORMAT_U32_BE;
    case SK_FORMAT_FLOAT_LE:
      return SND_PCM_FORMAT_FLOAT_LE;
    case SK_FORMAT_FLOAT_BE:
      return SND_PCM_FORMAT_FLOAT_BE;
    case SK_FORMAT_FLOAT64_LE:
      return SND_PCM_FORMAT_FLOAT64_LE;
    case SK_FORMAT_FLOAT64_BE:
      return SND_PCM_FORMAT_FLOAT64_BE;
    case SK_FORMAT_UNKNOWN:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

static SkResult
skPopulateComponentLimitsIMPL(snd_pcm_t *handle, SkComponentLimits *limits) {
  int iValue;
  unsigned int uiValue;
  snd_pcm_uframes_t uframesValue;

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);
  if (snd_pcm_hw_params_any(handle, params) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  SkFormat skFormat;
  snd_pcm_format_mask_t *formatMask;
  snd_pcm_format_mask_alloca(&formatMask);
  snd_pcm_hw_params_get_format_mask(params, formatMask);
  memset(limits->supportedFormats, 0, sizeof(limits->supportedFormats));
  for (skFormat = SK_FORMAT_STATIC_BEGIN; skFormat <= SK_FORMAT_END; ++skFormat) {
    if (snd_pcm_format_mask_test(formatMask, toLocalPcmFormatType(skGetFormatStatic(skFormat))) == 0) {
      limits->supportedFormats[skFormat] = SK_TRUE;
    }
  }
  limits->supportedFormats[SK_FORMAT_ANY] = SK_TRUE;

  if (snd_pcm_hw_params_get_buffer_size_max(params, &uframesValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_size_min(params, &uframesValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minFrameSize = uframesValue;

  if (snd_pcm_hw_params_get_buffer_time_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxBufferTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_buffer_time_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minBufferTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_channels_max(params, &uiValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->maxChannels = uiValue;

  if (snd_pcm_hw_params_get_channels_min(params, &uiValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  limits->minChannels = uiValue;

  if (snd_pcm_hw_params_get_period_size_max(params, &uframesValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriodSize, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_size_min(params, &uframesValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriodSize, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_time_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriodTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_period_time_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriodTime, uiValue, iValue);

  if (snd_pcm_hw_params_get_periods_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxPeriods, uiValue, iValue);

  if (snd_pcm_hw_params_get_periods_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minPeriods, uiValue, iValue);

  if (snd_pcm_hw_params_get_rate_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->maxRate, uiValue, iValue);

  if (snd_pcm_hw_params_get_rate_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  assignRangeAndDirection(&limits->minRate, uiValue, iValue);

  return SK_SUCCESS;
}

static SkPhysicalDevice
skGetPhysicalDeviceByCardIMPL(SkHostApi hostApi, int card) {
  SkPhysicalDevice device = hostApi->physicalDevices;
  while (device) {
    if (pdev_cast(device)->card == card) break;
    device = device->pNext;
  }
  return device;
}

static SkPhysicalComponent
skGetPhysicalComponentByDeviceIMPL(SkPhysicalDevice physicalDevice, int device) {
  SkPhysicalComponent component = physicalDevice->physicalComponents;
  while (component) {
    if (pcomp_cast(component)->device == device) break;
    component = component->pNext;
  }
  return component;
}

static SkResult
skConstructPhysicalComponentIMPL(snd_ctl_t *ctlHandle, unsigned int device, snd_pcm_stream_t type, SkPhysicalDevice physicalDevice, const SkAllocationCallbacks* pAllocator) {
  snd_pcm_t *handle;
  snd_pcm_info_t *info;
  SkComponentProperties *pProperties;
  SkPhysicalComponent *parentComponent;
  SkPhysicalComponent component = skGetPhysicalComponentByDeviceIMPL(physicalDevice, device);

  if (!component) {
    component = SKALLOC(sizeof(SkPhysicalComponent_ALSA), SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!component) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill in the required public information
    memset(component, 0, sizeof(SkPhysicalComponent_ALSA));
    component->objectType = SK_OBJECT_TYPE_PHYSICAL_COMPONENT;
    component->physicalDevice = physicalDevice;
    skGenerateObjectIdentifier(component->properties.identifier, (SkObject)component, (uint32_t)device);

    // Fill in the ALSA internal information
    sprintf(pcomp_cast(component)->identifier, "hw:%d,%u", pdev_cast(physicalDevice)->card, device);
    pcomp_cast(component)->device = device;

    // Put the component on the end to preserve component order
    parentComponent = &physicalDevice->physicalComponents;
    while (*parentComponent) {
      parentComponent = &(*parentComponent)->pNext;
    }
    (*parentComponent) = component;
  }

  pProperties = &component->properties;
  snd_pcm_info_alloca(&info);
  snd_pcm_info_set_device(info, device);
  snd_pcm_info_set_subdevice(info, 0);
  snd_pcm_info_set_stream(info, type);
  if (snd_ctl_pcm_info(ctlHandle, info) != 0) {
    // Note: This means that the stream type is not supported.
    return SK_SUCCESS;
  }
  strncpy(pProperties->componentName, snd_pcm_info_get_name(info), SK_MAX_COMPONENT_NAME_SIZE);

  if (snd_pcm_open(&handle, pcomp_cast(component)->identifier, type, 0) != 0) {
    // Note: This means that this device is not available.
    return SK_SUCCESS;
  }

  switch (type) {
    case SND_PCM_STREAM_PLAYBACK:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PCM_PLAYBACK;
      break;
    case SND_PCM_STREAM_CAPTURE:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PCM_CAPTURE;
      break;
  }

  snd_pcm_close(handle);

  return SK_SUCCESS;
}

static SkResult
skPopulatePhysicalDeviceComponentIMPL(snd_pcm_stream_t type, SkPhysicalDevice physicalDevice, const SkAllocationCallbacks* pAllocator) {
  SkResult result;
  SkResult opResult;
  snd_ctl_t *handle;
  snd_ctl_card_info_t *info;
  SkDeviceProperties *pProperties;

  if (snd_ctl_open(&handle, pdev_cast(physicalDevice)->identifier, type) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }

  // Populate device properties
  pProperties = &physicalDevice->properties;
  snd_ctl_card_info_alloca(&info);
  if (snd_ctl_card_info(handle, info)) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  strncpy(pProperties->deviceName, snd_ctl_card_info_get_name(info), SK_MAX_DEVICE_NAME_SIZE);
  strncpy(pProperties->driverName, snd_ctl_card_info_get_driver(info), SK_MAX_DRIVER_NAME_SIZE);
  strncpy(pProperties->mixerName, snd_ctl_card_info_get_mixername(info), SK_MAX_MIXER_NAME_SIZE);

  // handle pcm streams
  result = SK_SUCCESS;
  int device = -1;
  for (;;) {
    if (snd_ctl_pcm_next_device(handle, &device) != 0) {
      snd_ctl_close(handle);
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    if (device <= -1) break;

    opResult = skConstructPhysicalComponentIMPL(handle, (unsigned int)device, type, physicalDevice, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }
  snd_ctl_close(handle);

  return result;
}

static SkResult
skUpdatePhysicalDeviceIMPL(SkPhysicalDevice physicalDevice, const SkAllocationCallbacks* pAllocator) {
  SkResult result = SK_SUCCESS;
  SkResult opResult;
  opResult = skPopulatePhysicalDeviceComponentIMPL(SND_PCM_STREAM_PLAYBACK, physicalDevice, pAllocator);
  if (opResult != SK_SUCCESS) {
    result = SK_INCOMPLETE;
  }
  opResult = skPopulatePhysicalDeviceComponentIMPL(SND_PCM_STREAM_CAPTURE, physicalDevice, pAllocator);
  if (opResult != SK_SUCCESS) {
    result = SK_INCOMPLETE;
  }
  return result;
}

static SkResult
skConstructPhysicalDeviceIMPL(int card, SkHostApi hostApi, const SkAllocationCallbacks* pAllocator) {
  SkPhysicalDevice physicalDevice = skGetPhysicalDeviceByCardIMPL(hostApi, card);

  if (!physicalDevice) {
    physicalDevice = SKALLOC(sizeof(SkPhysicalDevice_ALSA), SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!physicalDevice) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill in public device information
    memset(physicalDevice, 0, sizeof(SkPhysicalDevice_ALSA));
    physicalDevice->objectType = SK_OBJECT_TYPE_PHYSICAL_DEVICE;
    skGenerateObjectIdentifier(physicalDevice->properties.identifier, (SkObject)physicalDevice, (uint32_t)card);
    physicalDevice->hostApi = hostApi;
    physicalDevice->pNext = hostApi->physicalDevices;
    hostApi->physicalDevices = physicalDevice;

    // Fill initial meta-information
    sprintf(pdev_cast(physicalDevice)->identifier, "hw:%d", card);
    pdev_cast(physicalDevice)->card = card;
  }

  return skUpdatePhysicalDeviceIMPL(physicalDevice, pAllocator);
}

static SkVirtualDevice
skGetVirtualDeviceByNameIMPL(SkHostApi hostApi, char const *name) {
  SkVirtualDevice device = hostApi->virtualDevices;
  while (device) {
    if (strcmp(device->properties.deviceName, name) == 0) return device;
    device = device->pNext;
  }
  return NULL;
}

static SkVirtualComponent
skGetVirtualComponentByNameIMPL(SkVirtualDevice virtualDevice, char const *name) {
  SkVirtualComponent component = virtualDevice->virtualComponents;
  while (component) {
    if (strcmp(vcomp_cast(component)->name, name) == 0) return component;
    component = component->pNext;
  }
  return NULL;
}

static SkResult
skPopulateVirtualDeviceComponentIMPL(void *hint, snd_pcm_stream_t type, SkVirtualDevice virtualDevice, const SkAllocationCallbacks* pAllocator) {
  // Get information from the device
  char *name, *desc;
  name = snd_device_name_get_hint(hint, "NAME");
  desc = snd_device_name_get_hint(hint, "DESC");
  SkVirtualComponent component = skGetVirtualComponentByNameIMPL(virtualDevice, name);

  if (!component) {
    // Count existing components
    uint32_t count = 0;
    component = virtualDevice->virtualComponents;
    while (component) {
      ++count;
      component = component->pNext;
    }

    component = SKALLOC(sizeof(SkVirtualComponent_ALSA), SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!component) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill in the initial meta-information
    memset(component, 0, sizeof(SkVirtualComponent_ALSA));
    component->objectType = SK_OBJECT_TYPE_VIRTUAL_COMPONENT;
    component->virtualDevice = virtualDevice;
    skGenerateObjectIdentifier(component->properties.identifier, (SkObject)component, count);
    strncpy(vcomp_cast(component)->name, name, SK_MAX_COMPONENT_NAME_SIZE);
    strncpy(component->properties.componentName, name, SK_MAX_COMPONENT_NAME_SIZE);
    strncpy(component->properties.componentDescription, desc, SK_MAX_COMPONENT_DESC_SIZE);

    // Put the component on the end to preserve component order
    SkVirtualComponent *parentComponent = &virtualDevice->virtualComponents;
    while (*parentComponent) {
      parentComponent = &(*parentComponent)->pNext;
    }
    (*parentComponent) = component;
  }

  // Note: Virtual devices don't support hardware limits.
  switch (type) {
    case SND_PCM_STREAM_PLAYBACK:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PCM_PLAYBACK;
      break;
    case SND_PCM_STREAM_CAPTURE:
      component->properties.supportedStreams |= SK_STREAM_TYPE_PCM_CAPTURE;
      break;
  }

  return SK_SUCCESS;
}

static SkResult
skUpdateVirtualDeviceIMPL(void *hint, SkVirtualDevice virtualDevice, const SkAllocationCallbacks* pAllocator) {
  // Get information from the device
  char *ioid;
  ioid = snd_device_name_get_hint(hint, "IOID");

  // Check stream information
  // Note: In ALSA, a NULL IOID means it supports both input and output.
  SkResult result = SK_SUCCESS;
  SkResult opResult;
  if (!ioid || strcmp(ioid, "Output") == 0) {
    opResult = skPopulateVirtualDeviceComponentIMPL(hint, SND_PCM_STREAM_PLAYBACK, virtualDevice, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }
  if (!ioid || strcmp(ioid, "Input") == 0) {
    opResult = skPopulateVirtualDeviceComponentIMPL(hint, SND_PCM_STREAM_CAPTURE, virtualDevice, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }
  return result;
}

static SkResult
skConstructVirtualDeviceIMPL(SkHostApi hostApi, void *hint, const SkAllocationCallbacks* pAllocator) {
  // Get information from the device
  char *name;
  name = snd_device_name_get_hint(hint, "NAME");

  // Note: In ALSA, we form Virtual Devices from the first
  //       part of the stream name. (Everything before ':')
  char deviceName[SK_MAX_DEVICE_NAME_SIZE];
  char *colonPtr = deviceName;
  strncpy(deviceName, name, SK_MAX_DEVICE_NAME_SIZE);
  while (*colonPtr && *colonPtr != ':') ++colonPtr;
  *colonPtr = '\0';

  // Construct the device if it doesn't exist
  SkVirtualDevice virtualDevice = skGetVirtualDeviceByNameIMPL(hostApi, deviceName);
  if (!virtualDevice) {
    // Count existing devices
    uint32_t count = 0;
    virtualDevice = hostApi->virtualDevices;
    while (virtualDevice) {
      ++count;
      virtualDevice = virtualDevice->pNext;
    }

    virtualDevice = SKALLOC(sizeof(SkVirtualDevice_ALSA), SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
    if (!virtualDevice) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Fill initial meta-information
    // Note: Since virtual devices don't have specific drivers/mixers,
    //       these values should almost always be set to 'N/A'.
    memset(virtualDevice, 0, sizeof(SkVirtualDevice_ALSA));
    virtualDevice->objectType = SK_OBJECT_TYPE_VIRTUAL_DEVICE;
    virtualDevice->hostApi = hostApi;
    skGenerateObjectIdentifier(virtualDevice->properties.identifier, (SkObject)virtualDevice, count);
    strcpy(virtualDevice->properties.deviceName, deviceName);
    strcpy(virtualDevice->properties.driverName, "N/A");
    strcpy(virtualDevice->properties.mixerName, "N/A");

    // Put the device on the end to preserve component order
    SkVirtualDevice *prevDevice = &hostApi->virtualDevices;
    while (*prevDevice) {
      prevDevice = &(*prevDevice)->pNext;
    }
    (*prevDevice) = virtualDevice;
  }

  return skUpdateVirtualDeviceIMPL(hint, virtualDevice, pAllocator);
}

////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////
void skHostApiFree_ALSA(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkObject device;
  SkObject nextDevice;
  SkObject component;
  SkObject nextComponent;

  // Free Physical Devices and Components
  device = (SkObject)hostApi->physicalDevices;
  while (device) {
    nextDevice = (SkObject)((SkPhysicalDevice)device)->pNext;
    component = (SkObject)((SkPhysicalDevice)device)->physicalComponents;
    while (component) {
      nextComponent = (SkObject)((SkPhysicalComponent)component)->pNext;
      SKFREE(component);
      component = nextComponent;
    }
    SKFREE(device);
    device = nextDevice;
  }

  // Free Virtual Devices and Components
  device = (SkObject)hostApi->virtualDevices;
  while (device) {
    nextDevice = (SkObject)((SkVirtualDevice)device)->pNext;
    component = (SkObject)((SkVirtualDevice)device)->virtualComponents;
    while (component) {
      nextComponent = (SkObject)((SkVirtualComponent)component)->pNext;
      SKFREE(component);
      component = nextComponent;
    }
    SKFREE(device);
    device = nextDevice;
  }

  // Free HostApi
  SKFREE(hostApi);
}

SkResult skRefreshPhysicalDevices_ALSA(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkResult opResult;
  uint32_t componentCount;
  uint32_t deviceCount = 0;
  uint32_t totalComponentCount = 0;
  SkPhysicalDevice physicalDevice;
  SkPhysicalComponent physicalComponent;
  SkResult result = SK_SUCCESS;
  int card = -1;

  for(;;) {
    // Attempt to open the next card
    if (snd_card_next(&card) != 0) {
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    if (card == -1) break; // Last card

    // Attempt to construct the new card handle
    opResult = skConstructPhysicalDeviceIMPL(card, hostApi, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }

  // Refresh the Host API's properties structure
  physicalDevice = hostApi->physicalDevices;
  while (physicalDevice) {
    ++deviceCount;
    componentCount = 0;
    physicalComponent = physicalDevice->physicalComponents;
    while (physicalComponent) {
      ++componentCount;
      physicalComponent = physicalComponent->pNext;
    }
    physicalDevice->properties.componentCount = componentCount;
    totalComponentCount += componentCount;
    physicalDevice = physicalDevice->pNext;
  }
  hostApi->properties.physicalDevices = deviceCount;
  hostApi->properties.physicalComponents = totalComponentCount;

  return result;
}

SkResult skRefreshVirtualDevices_ALSA(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkResult opResult;
  uint32_t componentCount;
  uint32_t deviceCount = 0;
  uint32_t totalComponentCount = 0;
  SkVirtualDevice virtualDevice;
  SkVirtualComponent virtualComponent;
  SkResult result = SK_SUCCESS;
  void **hints, **hint;

  // Refresh the list of virtual devices
  {
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    hint = hints;

    while (*hint) {
      opResult = skConstructVirtualDeviceIMPL(hostApi, *hint, pAllocator);
      if (opResult != SK_SUCCESS) {
        result = opResult;
      }
      ++hint;
    }
  }

  // Refresh the Host API's properties structure
  virtualDevice = hostApi->virtualDevices;
  while (virtualDevice) {
    ++deviceCount;
    componentCount = 0;
    virtualComponent = virtualDevice->virtualComponents;
    while (virtualComponent) {
      ++componentCount;
      virtualComponent = virtualComponent->pNext;
    }
    virtualDevice->properties.componentCount = componentCount;
    totalComponentCount += componentCount;
    virtualDevice = virtualDevice->pNext;
  }
  hostApi->properties.virtualDevices = deviceCount;
  hostApi->properties.virtualComponents = totalComponentCount;

  return result;
}

SkResult skRefreshPhysicalComponents_ALSA(
  SkPhysicalDevice                  physicalDevice,
  const SkAllocationCallbacks*      pAllocator
) {
  return skUpdatePhysicalDeviceIMPL(physicalDevice, pAllocator);
}

SkResult skRefreshVirtualComponents_ALSA(
  SkVirtualDevice                   virtualDevice,
  const SkAllocationCallbacks*      pAllocator
) {
  // Note: because virtual devices for ALSA is based on hints, we should just update all virtual devices...
  return skRefreshVirtualDevices_ALSA(virtualDevice->hostApi, pAllocator);
}

SkResult skGetPhysicalComponentLimits_ALSA(
    SkPhysicalComponent               physicalComponent,
    SkStreamType                      streamType,
    SkComponentLimits*                pLimits
) {
  SkResult result;
  snd_pcm_t *handle;

  if (snd_pcm_open(&handle, pcomp_cast(physicalComponent)->identifier, toLocalPcmStreamType(streamType), 0) != 0) {
    // Note: This means that this device is not available.
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  result = skPopulateComponentLimitsIMPL(handle, pLimits);
  snd_pcm_close(handle);
  return result;
}

SkResult skGetVirtualComponentLimits_ALSA(
    SkVirtualComponent                virtualComponent,
    SkStreamType                      streamType,
    SkComponentLimits*                pLimits
) {
  SkResult result;
  snd_pcm_t *handle;

  if (snd_pcm_open(&handle, vcomp_cast(virtualComponent)->name, toLocalPcmStreamType(streamType), 0) != 0) {
    // Note: This means that this device is not available.
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  result = skPopulateComponentLimitsIMPL(handle, pLimits);
  snd_pcm_close(handle);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Public API (Initializer)
////////////////////////////////////////////////////////////////////////////////
SkResult skHostApiInit_ALSA(
  SkHostApi*                        pHostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkHostApi hostApi;

  // Initialize the Host API's data
  hostApi = SKALLOC(sizeof(SkHostApi_ALSA), SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
  if (!hostApi) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the Host API's properties
  memset(hostApi, 0, sizeof(SkHostApi_ALSA));
  hostApi->objectType = SK_OBJECT_TYPE_HOST_API;
  strcpy(hostApi->properties.identifier, "alsa");
  strcpy(hostApi->properties.hostName, "ALSA");
  strcpy(hostApi->properties.hostNameFull, "Advanced Linux Sound Architecture (ALSA)");
  strcpy(hostApi->properties.description, "User space library (alsa-lib) to simplify application programming and provide higher level functionality.");
  hostApi->properties.capabilities = SK_HOST_CAPABILITIES_PCM | SK_HOST_CAPABILITIES_SEQUENCER;

  // Initialize the Host API's implementation
  hostApi->impl.SkHostApiFree = &skHostApiFree_ALSA;
  hostApi->impl.SkRefreshPhysicalDevices = &skRefreshPhysicalDevices_ALSA;
  hostApi->impl.SkRefreshVirtualDevices = &skRefreshVirtualDevices_ALSA;
  hostApi->impl.SkRefreshPhysicalComponents = &skRefreshPhysicalComponents_ALSA;
  hostApi->impl.SkRefreshVirtualComponents = &skRefreshVirtualComponents_ALSA;
  hostApi->impl.SkGetPhysicalComponentLimits = &skGetPhysicalComponentLimits_ALSA;
  hostApi->impl.SkGetVirtualComponentLimits = &skGetVirtualComponentLimits_ALSA;

  *pHostApi = hostApi;
  return SK_SUCCESS;
}

/*

////////////////////////////////////////////////////////////////////////////////
// Implementation Types
////////////////////////////////////////////////////////////////////////////////
typedef union SkPfnFunctions {
  snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
} SkPfnFunctions;

typedef struct SkExtensionData_device_polling_SEA {
  SkBool32                      initialized;
  struct udev*                  pUdev;
  struct udev_monitor*          pUdevMonitor;
} SkExtensionData_device_polling_SEA;

typedef struct SkStream_T {
  SkStream                      pNext;
  snd_pcm_t*                    handle;
  SkInstance                    instance;
  SkStreamRequestInfo           requestInfo;
  SkStreamInfo                  streamInfo;
  SkPfnFunctions                pfn;
} SkStream_T;

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
  SkStream                      pStreams;
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
static union { uint32_t u32; unsigned char u8[4]; } intEndianCheck = { (uint32_t)0x01234567 };
static union { float f32; unsigned char u8[4]; } floatEndianCheck = { (float)0x01234567 };
#define SK_INT_IS_BIGENDIAN() intEndianCheck.u8[3] == 0x01
#define SK_FLOAT_IS_BIGENDIAN() floatEndianCheck.u8[3] == 0x01

static int toLocalPcmAccessMode(SkAccessMode mode) {
  switch (mode) {
    case SK_ACCESS_MODE_BLOCK:
      return 0;
    case SK_ACCESS_MODE_NONBLOCK:
      return SND_PCM_NONBLOCK;
  }
}

static snd_pcm_access_t toLocalPcmAccessType(SkAccessType type) {
  switch (type) {
    case SK_ACCESS_TYPE_INTERLEAVED:
      return SND_PCM_ACCESS_RW_INTERLEAVED;
    case SK_ACCESS_TYPE_NONINTERLEAVED:
      return SND_PCM_ACCESS_RW_NONINTERLEAVED;
    case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
      return SND_PCM_ACCESS_MMAP_INTERLEAVED;
    case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
      return SND_PCM_ACCESS_MMAP_NONINTERLEAVED;
    case SK_ACCESS_TYPE_MMAP_COMPLEX:
      return SND_PCM_ACCESS_MMAP_COMPLEX;
  }
}

static SkAccessType toSkAccessType(snd_pcm_access_t type) {
  switch (type) {
    case SND_PCM_ACCESS_RW_INTERLEAVED:
      return SK_ACCESS_TYPE_INTERLEAVED;
    case SND_PCM_ACCESS_RW_NONINTERLEAVED:
      return SK_ACCESS_TYPE_NONINTERLEAVED;
    case SND_PCM_ACCESS_MMAP_INTERLEAVED:
      return SK_ACCESS_TYPE_MMAP_INTERLEAVED;
    case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
      return SK_ACCESS_TYPE_MMAP_NONINTERLEAVED;
    case SND_PCM_ACCESS_MMAP_COMPLEX:
      return SK_ACCESS_TYPE_MMAP_COMPLEX;
  }
}


static SkFormat toSkFormat(snd_pcm_format_t type) {
  switch (type) {
    case SND_PCM_FORMAT_S8:
      return SK_FORMAT_S8;
    case SND_PCM_FORMAT_U8:
      return SK_FORMAT_U8;
    case SND_PCM_FORMAT_S16_LE:
      return SK_FORMAT_S16_LE;
    case SND_PCM_FORMAT_S16_BE:
      return SK_FORMAT_S16_BE;
    case SND_PCM_FORMAT_U16_LE:
      return SK_FORMAT_U16_LE;
    case SND_PCM_FORMAT_U16_BE:
      return SK_FORMAT_U16_BE;
    case SND_PCM_FORMAT_S24_LE:
      return SK_FORMAT_S24_LE;
    case SND_PCM_FORMAT_S24_BE:
      return SK_FORMAT_S24_BE;
    case SND_PCM_FORMAT_U24_LE:
      return SK_FORMAT_U24_LE;
    case SND_PCM_FORMAT_U24_BE:
      return SK_FORMAT_U24_BE;
    case SND_PCM_FORMAT_S32_LE:
      return SK_FORMAT_S32_LE;
    case SND_PCM_FORMAT_S32_BE:
      return SK_FORMAT_S32_BE;
    case SND_PCM_FORMAT_U32_LE:
      return SK_FORMAT_U32_LE;
    case SND_PCM_FORMAT_U32_BE:
      return SK_FORMAT_U32_BE;
    case SND_PCM_FORMAT_FLOAT_LE:
      return SK_FORMAT_FLOAT_LE;
    case SND_PCM_FORMAT_FLOAT_BE:
      return SK_FORMAT_FLOAT_BE;
    case SND_PCM_FORMAT_FLOAT64_LE:
      return SK_FORMAT_FLOAT64_LE;
    case SND_PCM_FORMAT_FLOAT64_BE:
      return SK_FORMAT_FLOAT64_BE;
    default:
      return SK_FORMAT_UNKNOWN;
  }
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

static void skDestructPhysicalDevice(SkPhysicalDevice device) {
  device->instance->pAllocator->pfnFree(
    device->instance->pAllocator->pUserData,
    device
  );
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
  instance->pStreams = NULL;
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

  // Destroy Physical Devices
  SkPhysicalDevice currPhysicalDevice = instance->pPhysicalDevices;
  SkPhysicalDevice nextPhysicalDevice;
  while (currPhysicalDevice) {
    nextPhysicalDevice = currPhysicalDevice->next;
    instance->pAllocator->pfnFree(instance->pAllocator->pUserData, currPhysicalDevice);
    currPhysicalDevice = nextPhysicalDevice;
  }

  // Destroy Virtual Devices
  SkVirtualDevice currVirtualDevice = instance->pVirtualDevices;
  SkVirtualDevice nextVirtualDevice;
  while (currVirtualDevice) {
    nextVirtualDevice = currVirtualDevice->next;
    instance->pAllocator->pfnFree(instance->pAllocator->pUserData, currVirtualDevice);
    currVirtualDevice = nextVirtualDevice;
  }

  // Destroy Streams
  SkStream currStream = instance->pStreams;
  SkStream nextStream;
  while (currStream) {
    nextStream = currStream->pNext;
    snd_pcm_close(currStream->handle);
    instance->pAllocator->pfnFree(instance->pAllocator->pUserData, currStream);
    currStream = nextStream;
  }

  instance->pAllocator->pfnFree(instance->pAllocator->pUserData, instance);
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

#define PCM_CHECK(call) if (call < 0) { return SK_ERROR_STREAM_REQUEST_UNSUPPORTED; }
static SkResult skRequestStream(
  snd_pcm_t*                        handle,
  SkInstance                        instance,
  SkStreamRequestInfo*              pStreamRequestInfo,
  SkStream*                         pStream
) {
  int iValue;
  unsigned int uiValue;
  snd_pcm_uframes_t ufValue;
  snd_pcm_access_t aValue;
  snd_pcm_format_t fValue;
  SkStreamInfo streamInfo;

  snd_pcm_hw_params_t *hwParams;
  snd_pcm_hw_params_alloca(&hwParams);
  PCM_CHECK(snd_pcm_hw_params_any(handle, hwParams))

  // Access Type (Default = First)
  if (pStreamRequestInfo->accessType != SK_ACCESS_TYPE_ANY) {
    snd_pcm_access_t access = toLocalPcmAccessType(pStreamRequestInfo->accessType);
    PCM_CHECK(snd_pcm_hw_params_set_access(handle, hwParams, access))
  }

  // Format (Default = First)
  if (pStreamRequestInfo->formatType != SK_FORMAT_ANY) {
    snd_pcm_format_t format = toLocalPcmFormatType(pStreamRequestInfo->formatType);
    PCM_CHECK(snd_pcm_hw_params_set_format(handle, hwParams, format))
  }

  // Channels (Default = Minimum)
  if (pStreamRequestInfo->channels != 0) {
    uiValue = pStreamRequestInfo->channels;
    PCM_CHECK(snd_pcm_hw_params_set_channels_near(handle, hwParams, &uiValue))
  }

  // Rate (Default = Minimum)
  if (pStreamRequestInfo->rate != 0) {
    uiValue = pStreamRequestInfo->rate; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_rate_near(handle, hwParams, &uiValue, &iValue))
  }

  // Period (Default = Minimum)
  if (pStreamRequestInfo->periods != 0) {
    uiValue = pStreamRequestInfo->periods; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_periods_near(handle, hwParams, &uiValue, &iValue))
  }

  // Period Time (Default = Minimum)
  if (pStreamRequestInfo->periodSize > 0) {
    ufValue = pStreamRequestInfo->periodSize; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_period_size_near(handle, hwParams, &ufValue, &iValue))
  }
  else if (pStreamRequestInfo->periodTime > 0) {
    uiValue = pStreamRequestInfo->periodTime; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_period_time_near(handle, hwParams, &uiValue, &iValue))
  }

  // Buffer Size (Default = Maximum)
  if (pStreamRequestInfo->bufferSize > 0) {
    ufValue = pStreamRequestInfo->bufferSize; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_buffer_size_near(handle, hwParams, &ufValue))
  }
  else if (pStreamRequestInfo->bufferTime > 0) {
    uiValue = pStreamRequestInfo->bufferSize; iValue = 0;
    PCM_CHECK(snd_pcm_hw_params_set_buffer_time_near(handle, hwParams, &uiValue, &iValue))
  }

  int err = snd_pcm_hw_params(handle, hwParams);
  if (err < 0) {
    snd_pcm_close(handle);
    return SK_ERROR_STREAM_REQUEST_FAILED;
  }

  // Gather the configuration space information
  streamInfo.streamType = pStreamRequestInfo->streamType;
  streamInfo.accessMode = pStreamRequestInfo->accessMode;
  PCM_CHECK(snd_pcm_hw_params_get_access(hwParams, &aValue))
  streamInfo.accessType = toSkAccessType(aValue);
  PCM_CHECK(snd_pcm_hw_params_get_format(hwParams, &fValue))
  streamInfo.formatType = toSkFormat(fValue);
  streamInfo.formatBits = (uint32_t)snd_pcm_format_physical_width(fValue);
  PCM_CHECK(snd_pcm_hw_params_get_periods(hwParams, &uiValue, &iValue))
  assignRangeAndDirection(&streamInfo.periods, uiValue, iValue);
  PCM_CHECK(snd_pcm_hw_params_get_channels(hwParams, &uiValue))
  streamInfo.channels = uiValue;
  PCM_CHECK(snd_pcm_hw_params_get_period_size(hwParams, &ufValue, &iValue))
  streamInfo.periodSamples = ufValue;
  PCM_CHECK(snd_pcm_hw_params_get_period_time(hwParams, &uiValue, &iValue))
  assignRangeAndDirection(&streamInfo.periodTime, uiValue, iValue);
  PCM_CHECK(snd_pcm_hw_params_get_buffer_size(hwParams, &ufValue))
  streamInfo.bufferSamples = ufValue;
  PCM_CHECK(snd_pcm_hw_params_get_buffer_time(hwParams, &uiValue, &iValue))
  assignRangeAndDirection(&streamInfo.bufferTime, uiValue, iValue);
  PCM_CHECK(snd_pcm_hw_params_get_rate(hwParams, &uiValue, &iValue))
  assignRangeAndDirection(&streamInfo.rate, uiValue, iValue);
  streamInfo.sampleBits = streamInfo.channels * streamInfo.formatBits;
  streamInfo.bufferBits = streamInfo.bufferSamples * streamInfo.sampleBits;
  streamInfo.sampleTime.value = streamInfo.periodTime.value / streamInfo.periodSamples;
  streamInfo.sampleTime.direction = streamInfo.periodTime.direction;
  streamInfo.periodBits = streamInfo.sampleBits * streamInfo.periodSamples;
  // latency = periodsize * periods / (rate * bytes_per_frame)
  streamInfo.latency = 8 * streamInfo.periodSamples * streamInfo.periods.value / (streamInfo.rate.value * streamInfo.sampleBits);

  // Success! Create the stream -
  SkStream finalStream =
  (SkStream)instance->pAllocator->pfnAllocation(
    instance->pAllocator->pUserData,
    sizeof(SkStream_T),
    0,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  finalStream->instance = instance;
  finalStream->streamInfo = streamInfo;
  finalStream->pNext = instance->pStreams;
  finalStream->handle = handle;
  instance->pStreams = finalStream;

  switch (streamInfo.streamType) {
    case SK_STREAM_TYPE_CAPTURE:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          finalStream->pfn.readi_func = &snd_pcm_readi;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          finalStream->pfn.readn_func = &snd_pcm_readn;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          finalStream->pfn.readi_func = &snd_pcm_mmap_readi;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          finalStream->pfn.readn_func = &snd_pcm_mmap_readn;
          break;
        default:
          break;
      }
      break;
    case SK_STREAM_TYPE_PLAYBACK:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          finalStream->pfn.writei_func = &snd_pcm_writei;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          finalStream->pfn.writen_func = &snd_pcm_writen;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          finalStream->pfn.writei_func = &snd_pcm_mmap_writei;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          finalStream->pfn.writen_func = &snd_pcm_mmap_writen;
          break;
        default:
          break;
      }
      break;
  }

  *pStream = finalStream;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestPhysicalStream(
  SkPhysicalComponent               physicalComponent,
  SkStreamRequestInfo*              pStreamRequestInfo,
  SkStream*                         pStream
) {
  int err;
  snd_pcm_t *handle;
  int mode = toLocalPcmAccessMode(pStreamRequestInfo->accessMode);
  snd_pcm_stream_t stream = toLocalPcmStreamType(pStreamRequestInfo->streamType);
  if ((err = snd_pcm_open(&handle, physicalComponent->identifier, stream, mode)) != 0) {
    if (err == -EBUSY) return SK_ERROR_STREAM_BUSY;
    return SK_ERROR_STREAM_REQUEST_FAILED;
  }
  SkResult result = skRequestStream(handle, physicalComponent->physicalDevice->instance, pStreamRequestInfo, pStream);
  if (result != SK_SUCCESS) {
    snd_pcm_close(handle);
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestVirtualStream(
  SkVirtualComponent                virtualComponent,
  SkStreamRequestInfo*              pStreamRequestInfo,
  SkStream*                         pStream
) {
  int err;
  snd_pcm_t *handle;
  int mode = toLocalPcmAccessMode(pStreamRequestInfo->accessMode);
  snd_pcm_stream_t stream = toLocalPcmStreamType(pStreamRequestInfo->streamType);
  if ((err = snd_pcm_open(&handle, virtualComponent->properties.componentName, stream, mode)) != 0) {
    if (err == -EBUSY) return SK_ERROR_STREAM_BUSY;
    return SK_ERROR_STREAM_REQUEST_FAILED;
  }
  SkResult result = skRequestStream(handle, virtualComponent->virtualDevice->instance, pStreamRequestInfo, pStream);
  if (result != SK_SUCCESS) {
    snd_pcm_close(handle);
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestDefaultStream(
  SkInstance                        instance,
  SkStreamRequestInfo*              pStreamRequestInfo,
  SkStream*                         pStream
) {
  int err;
  snd_pcm_t *handle;
  int mode = toLocalPcmAccessMode(pStreamRequestInfo->accessMode);
  snd_pcm_stream_t stream = toLocalPcmStreamType(pStreamRequestInfo->streamType);
  if ((err = snd_pcm_open(&handle, "default", stream, mode)) != 0) {
    if (err == -EBUSY) return SK_ERROR_STREAM_BUSY;
    return SK_ERROR_STREAM_REQUEST_FAILED;
  }
  SkResult result = skRequestStream(handle, instance, pStreamRequestInfo, pStream);
  if (result != SK_SUCCESS) {
    snd_pcm_close(handle);
  }
  return result;
}
#undef PCM_CHECK

SKAPI_ATTR SkResult SKAPI_CALL skGetStreamInfo(
  SkStream                          stream,
  SkStreamInfo*                     pStreamInfo
) {
  *pStreamInfo = stream->streamInfo;
}

static snd_pcm_sframes_t xrun_recovery(snd_pcm_t *handle, snd_pcm_sframes_t err)
{
  // An underrun occurred, open the stream back up and try again
  if (err == -EPIPE) {
    err = snd_pcm_prepare(handle);
    if (err < 0) {
      return SK_ERROR_FAILED_STREAM_WRITE;
    }
    return SK_ERROR_XRUN;
  }
  //
  else if (err == -ESTRPIPE) {
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
        printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
    }
    return 0;
  }
  else if (err == -EBADFD) {
    printf("Not the correct state?\n");
  }
  else {
    printf("We do not know what happened.\n");
  }
  return err;
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteInterleaved(
  SkStream                          stream,
  void const*                       pBuffer,
  uint32_t                          framesCount
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream->pfn.writei_func(stream->handle, pBuffer, framesCount)) < 0) {
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteNoninterleaved(
  SkStream                          stream,
  void const*const*                 pBuffer,
  uint32_t                          framesCount
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream->pfn.writen_func(stream->handle, (void**)pBuffer, framesCount)) < 0) {
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

SKAPI_ATTR void SKAPI_CALL skDestroyStream(
    SkStream                          stream,
    SkBool32                          drain
) {
  // The device could be suspended - should we handle this?
  if (drain == SK_TRUE) {
    (void)snd_pcm_drain(stream->handle);
  }
  else {
    (void)snd_pcm_drop(stream->handle);
  }

  // Fix the linked list
  SkStream *prev = &stream->instance->pStreams;
  while ((*prev) != stream) {
    prev = &(*prev)->pNext;
  }
  (*prev) = stream->pNext;

  stream->instance->pAllocator->pfnFree(
    stream->instance->pAllocator->pUserData,
    stream
  );
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

*/