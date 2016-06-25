/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard implementation. (ALSA Implementation)
 ******************************************************************************/

#include <OpenSK/opensk.h>
#include <OpenSK/dev/assert.h>
#include <OpenSK/dev/hosts.h>
#include <OpenSK/dev/macros.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alsa/asoundlib.h>

#define SK_ALSA_VIRTUAL_DEVICE_CARD_ID -1
#define SK_ALSA_MAX_IDENTIFIER_SIZE 256
#define SK_ALSA_MAX_HINT_NAME_SIZE 256
#define SK_ALSA_MAX_HINT_DESC_SIZE 256
#define SK_ALSA_MAX_HINT_IOID_SIZE 64

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef union SkPfnFunctionsIMPL {
  snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
  snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
} SkPfnFunctionsIMPL;

typedef struct SkLimitsContainerIMPL {
  SkComponentLimits             capture;
  SkComponentLimits             playback;
} SkLimitsContainerIMPL;

#define host_cast(h) ((SkHostApi_ALSA*)h)
typedef struct SkHostApi_ALSA {
  SkHostApi_T                   parent;
} SkHostApi_ALSA;

#define dev_cast(d) ((SkDevice_ALSA*)d)
typedef struct SkDevice_ALSA {
  SkDevice_T                    parent;
  int                           card;
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];
} SkDevice_ALSA;

#define comp_cast(c) ((SkComponent_ALSA*)c)
typedef struct SkComponent_ALSA {
  SkComponent_T                 parent;
  unsigned int                  device;
  char                          identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];
} SkComponent_ALSA;

#define stream_cast(s) ((SkStream_ALSA*)s)
typedef struct SkStream_ALSA {
  SkStream_T                    parent;
  snd_pcm_t*                    handle;
  SkPfnFunctionsIMPL            pfn;
} SkStream_ALSA;

////////////////////////////////////////////////////////////////////////////////
// Conversion Functions
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
      SKERROR("Should never not know the direction of a range.\n");
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
      SKERROR("Should never consume a non-PCM stream type.\n");
      return SND_PCM_STREAM_PLAYBACK;
  }
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
    case SK_FORMAT_INVALID:
      return SND_PCM_FORMAT_UNKNOWN;

    // This should have been handled
    case SK_FORMAT_ANY:
      SKERROR("Should never consume the SK_FORMAT_ANY flag.\n");
      return SND_PCM_FORMAT_UNKNOWN;

    // If it's any of the dynamic formats, we should resolve the actual type.
    case SK_FORMAT_S16:
    case SK_FORMAT_U16:
    case SK_FORMAT_S24:
    case SK_FORMAT_U24:
    case SK_FORMAT_S32:
    case SK_FORMAT_U32:
    case SK_FORMAT_FLOAT:
    case SK_FORMAT_FLOAT64:
      return toLocalPcmFormatType(skGetFormatStatic(type));
  }
}

static int
toLocalPcmAccessMode(SkAccessMode mode) {
  switch (mode) {
    case SK_ACCESS_MODE_BLOCK:
      return 0;
    case SK_ACCESS_MODE_NONBLOCK:
      return SND_PCM_NONBLOCK;
  }
}

static snd_pcm_access_t
toLocalPcmAccessType(SkAccessType type) {
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
    case SK_ACCESS_TYPE_ANY:
      SKERROR("Should never receive the access type SK_ACCESS_TYPE_ANY.\n");
      return SND_PCM_ACCESS_LAST;
  }
}

static SkAccessType
toSkAccessType(snd_pcm_access_t type) {
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

static SkFormat
toSkFormat(snd_pcm_format_t type) {
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
      return SK_FORMAT_INVALID;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////
static SkDevice
skGetDeviceByCardIMPL(
  SkDevice                            pDevices,
  int                                 card
) {
  while (pDevices) {
    if (dev_cast(pDevices)->card == card) break;
    pDevices = pDevices->pNext;
  }
  return pDevices;
}

static SkDevice
skGetDeviceByIdentifierIMPL(
  SkDevice                            pDevices,
  char const*                         identifier
) {
  while (pDevices) {
    if (strcmp(dev_cast(pDevices)->identifier, identifier) == 0)
      return pDevices;
    pDevices = pDevices->pNext;
  }
  return NULL;
}

static SkComponent
skGetComponentByDeviceIMPL(
  SkComponent                         pComponents,
  int                                 device
) {
  while (pComponents) {
    if (comp_cast(pComponents)->device == device) break;
    pComponents = pComponents->pNext;
  }
  return pComponents;
}

static SkComponent
skGetComponentByIdentifierIMPL(
  SkComponent                         pComponents,
  char const*                         identifier
) {
  while (pComponents) {
    if (strcmp(comp_cast(pComponents)->identifier, identifier) == 0)
      return pComponents;
    pComponents = pComponents->pNext;
  }
  return NULL;
}

// TODO: Unsure if SkComponentLimits is a useful object...
static SkResult
skPopulateComponentLimitsIMPL(
  snd_pcm_t*                          handle,
  SkComponentLimits*                  limits
) {
  int error;
  int iValue;
  unsigned int uiValue;
  snd_pcm_uframes_t uframesValue;
  snd_pcm_hw_params_t *params;

  // Allocate hardware parameters
  error = snd_pcm_hw_params_malloc(&params);
  if (error < 0) {
    SKWARN("%s\n", snd_strerror(error));
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initial configuration
  snd_pcm_hw_params_any(handle, params);

  SkFormat skFormat;
  snd_pcm_format_mask_t *formatMask;
  snd_pcm_format_mask_alloca(&formatMask);
  snd_pcm_hw_params_get_format_mask(params, formatMask);
  memset(limits->supportedFormats, 0, sizeof(limits->supportedFormats));
  for (skFormat = SK_FORMAT_STATIC_BEGIN; skFormat <= SK_FORMAT_END; ++skFormat) {
    SkFormat staticFormat = skGetFormatStatic(skFormat);
    snd_pcm_format_t format = toLocalPcmFormatType(staticFormat);
    if (snd_pcm_format_mask_test(formatMask, format) == 0) {
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

static SkResult
skResolveComponentIMPL(
  SkDevice                            device,
  int                                 deviceId,
  char const*                         identifier,
  const SkAllocationCallbacks*        pAllocator,
  SkComponent*                        pComponent
) {
  SkResult result;
  SkComponent component;
  SKASSERT(identifier != NULL, "Identifier cannot be null!\n");

  // Attempt to find the component if it already exists
  if (deviceId >= 0) {
    component = skGetComponentByDeviceIMPL(device->pComponents, (unsigned int)deviceId);
  }
  else {
    component = skGetComponentByIdentifierIMPL(device->pComponents, identifier);
  }

  // Failed to find the component, so we must construct it
  if (!component) {
    result = skConstructComponent(device, sizeof(SkComponent_ALSA), pAllocator, &component);
    if (result != SK_SUCCESS) {
      return result;
    }

    // Fill in the ALSA internal information
    comp_cast(component)->device = (unsigned int)deviceId;
    strcpy(comp_cast(component)->identifier, identifier);
  }

  *pComponent = component;
  return SK_SUCCESS;
}

static SkResult
skResolveDeviceIMPL(
  SkHostApi                           hostApi,
  int                                 cardId,
  char const*                         identifier,
  const SkAllocationCallbacks*        pAllocator,
  SkDevice*                           pDevice
) {
  SkResult result;
  SkDevice device;

  // Attempt to find the component if it already exists
  if (cardId >= 0) {
    device = skGetDeviceByCardIMPL(hostApi->pDevices, (unsigned int)cardId);
  }
  else {
    device = skGetDeviceByIdentifierIMPL(hostApi->pDevices, identifier);
  }

  // Failed to find the device, so we must construct it
  if (!device) {
    result = skConstructDevice(hostApi, sizeof(SkDevice_ALSA), (cardId >= 0) ? SK_TRUE : SK_FALSE, pAllocator, &device);
    if (result != SK_SUCCESS) {
      return result;
    }

    // Fill in the ALSA internal information
    dev_cast(device)->card = cardId;
    strcpy(dev_cast(device)->identifier, identifier);
  }
  *pDevice = device;

  return SK_SUCCESS;
}

static SkResult
skUpdatePhysicalComponentIMPL(
  snd_ctl_t*                          ctlHandle,
  SkDevice                            device,
  int                                 deviceId,
  const SkAllocationCallbacks*        pAllocator
) {
  int error;
  SkResult result;
  snd_pcm_info_t *info;
  SkComponent component;
  SkStreamType streamType;
  char identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];

  // Resolve or construct the component
  sprintf(identifier, "hw:%d,%d", dev_cast(device)->card, deviceId);
  result = skResolveComponentIMPL(device, deviceId, identifier, pAllocator, &component);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Get information about the name of the device
  snd_pcm_info_malloc(&info);
  snd_pcm_info_set_device(info, (unsigned int)deviceId);
  snd_pcm_info_set_subdevice(info, 0);
  for (streamType = SK_STREAM_TYPE_PCM_BEGIN; streamType != SK_STREAM_TYPE_PCM_END; streamType <<= 1) {
    snd_pcm_info_set_stream(info, toLocalPcmStreamType(streamType));
    error = snd_ctl_pcm_info(ctlHandle, info);
    switch (error) {
      case 0:
      case -EBUSY: // Stream exists but is busy
        strncpy(component->properties.componentName, snd_pcm_info_get_name(info), SK_MAX_COMPONENT_NAME_SIZE);
        component->properties.supportedStreams |= streamType;
        break;
      case -ENOENT:// Stream type isn't supported
        break;
      default:
        SKWARNIF(error != 0, "ALSA Error: %s\n", snd_strerror(error));
        break;
    }
  }
  component->properties.isPhysical = device->properties.isPhysical;
  snd_pcm_info_free(info);

  return SK_SUCCESS;
}

static SkResult
skUpdatePhysicalDeviceIMPL(
  SkHostApi                           hostApi,
  unsigned int                        cardId,
  const SkAllocationCallbacks*        pAllocator
) {
  int error;
  SkResult result;
  SkResult opResult;
  snd_ctl_t* ctlHandle;
  snd_ctl_card_info_t *info;
  SkDevice device;
  int deviceId;
  char identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];

  // Resolve or construct the device
  sprintf(identifier, "hw:%u", cardId);
  result = skResolveDeviceIMPL(hostApi, cardId, identifier, pAllocator, &device);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Attempt to acquire the card
  error = snd_ctl_open(&ctlHandle, identifier, 0);
  if (error != 0) {
    SKERROR("ALSA: %s\n", snd_strerror(error));
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Iterate through all of the components on the device
  deviceId = -1;
  for (;;) {
    if (snd_ctl_pcm_next_device(ctlHandle, &deviceId) != 0) {
      snd_ctl_close(ctlHandle);
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    if (deviceId <= -1) break;
    opResult = skUpdatePhysicalComponentIMPL(ctlHandle, device, deviceId, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }

  // Apply metadata to the card itself
  snd_ctl_card_info_malloc(&info);
  error = snd_ctl_card_info(ctlHandle, info);
  if (error != 0) {
    SKERROR("ALSA: %s\n", snd_strerror(error));
    // Ignore, doesn't matter because it's only metadata.
  }
  else {
    strncpy(device->properties.deviceName, snd_ctl_card_info_get_name(info), SK_MAX_DEVICE_NAME_SIZE);
    strncpy(device->properties.driverName, snd_ctl_card_info_get_driver(info), SK_MAX_DRIVER_NAME_SIZE);
    strncpy(device->properties.mixerName, snd_ctl_card_info_get_mixername(info), SK_MAX_MIXER_NAME_SIZE);
  }
  snd_ctl_card_info_free(info);

  // Close card
  snd_ctl_close(ctlHandle);
  return result;
}

static SkResult
skUpdateVirtualDeviceIMPL(
  SkHostApi                           hostApi,
  void*                               hint,
  const SkAllocationCallbacks*        pAllocator
) {
  char *ioid;
  char *colonPtr;
  char *componentName;
  SkResult result;
  SkDevice device;
  SkComponent component;
  SkStreamType streamType;
  char deviceName[SK_MAX_DEVICE_NAME_SIZE];

  // Note: In ALSA, we form Virtual Devices from the first
  //       part of the stream name. (Everything before ':')
  colonPtr = deviceName;
  componentName = snd_device_name_get_hint(hint, "NAME");
  strncpy(deviceName, componentName, SK_MAX_DEVICE_NAME_SIZE);
  while (*colonPtr && *colonPtr != ':') ++colonPtr;
  if (*colonPtr == ':') {
    *colonPtr = '\0';
    componentName = colonPtr + 1;
  }

  // Resolve or construct the device
  result = skResolveDeviceIMPL(hostApi, SK_ALSA_VIRTUAL_DEVICE_CARD_ID, deviceName, pAllocator, &device);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Update virtual device properties
  strcpy(device->properties.deviceName, deviceName);
  strcpy(device->properties.driverName, "N/A");
  strcpy(device->properties.mixerName, "N/A");

  // Resolve or construct the component
  result = skResolveComponentIMPL(device, SK_ALSA_VIRTUAL_DEVICE_CARD_ID, componentName, pAllocator, &component);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Update virtual component properties
  strncpy(component->properties.componentName, componentName, SK_MAX_COMPONENT_NAME_SIZE);
  component->properties.isPhysical = device->properties.isPhysical;

  // Find out which PCM streams this virtual component supports
  streamType = SK_STREAM_TYPE_NONE;
  ioid = snd_device_name_get_hint(hint, "IOID");
  if (!ioid || strcmp(ioid, "Output") == 0) streamType |= SK_STREAM_TYPE_PCM_PLAYBACK;
  if (!ioid || strcmp(ioid, "Input")  == 0) streamType |= SK_STREAM_TYPE_PCM_CAPTURE;
  component->properties.supportedStreams = streamType;

  return SK_SUCCESS;
}


static snd_pcm_sframes_t
xrun_recovery(snd_pcm_t *handle, snd_pcm_sframes_t err)
{
  // An underrun occurred, open the stream back up and try again
  if (err == -EPIPE) {
    SKWARN("An xrun has occurred.\n");
    err = snd_pcm_prepare(handle);
    if (err < 0) {
      return SK_ERROR_FAILED_STREAM_WRITE;
    }
    return SK_ERROR_XRUN;
  }
  //
  else if (err == -ESTRPIPE) {
    SKWARN("Device is not yet ready, try again later.\n");
    while ((err = snd_pcm_resume(handle)) == -EAGAIN)
      sleep(1);
    if (err < 0) {
      err = snd_pcm_prepare(handle);
      if (err < 0)
        SKERROR("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
    }
    return 0;
  }
  else if (err == -EBADFD) {
    SKERROR("Not the correct state?\n");
  }
  else {
    SKERROR("We do not know what happened.\n");
  }
  return err;
}

////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////
void skHostApiFree_ALSA(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkStream stream;
  SkStream nextStream;
  SkDevice device;
  SkDevice nextDevice;
  SkComponent component;
  SkComponent nextComponent;

  // Free Devices, Components, and Streams
  device = hostApi->pDevices;
  while (device) {
    nextDevice = device->pNext;
    component = device->pComponents;
    while (component) {
      nextComponent = component->pNext;
      stream = component->pStreams;
      while (stream) {
        nextStream = stream->pNext;
        SKFREE(stream);
        stream = nextStream;
      }
      SKFREE(component);
      component = nextComponent;
    }
    SKFREE(device);
    device = nextDevice;
  }

  // Free HostApi
  SKFREE(hostApi);
}

static SkResult
skScanDevices_ALSA(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  int card;
  void** hint;
  void** hints;
  SkResult result;
  SkResult opResult;

  result = SK_SUCCESS;

  // Physical Devices
  card = -1;
  for(;;) {
    // Attempt to open the next card
    if (snd_card_next(&card) != 0) {
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    if (card == -1) break; // Last card

    // Attempt to construct the new card handle
    opResult = skUpdatePhysicalDeviceIMPL(hostApi, (unsigned int)card, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }

  // Virtual Devices
  {
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
      return SK_ERROR_FAILED_QUERYING_DEVICE;
    }
    hint = hints;

    while (*hint) {
      opResult = skUpdateVirtualDeviceIMPL(hostApi, *hint, pAllocator);
      if (opResult != SK_SUCCESS) {
        result = opResult;
      }
      ++hint;
    }
  }

    return result;
}

static SkResult
skGetComponentLimits_ALSA(
  SkComponent                       component,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  SkResult result;
  snd_pcm_t *handle;

  if (snd_pcm_open(&handle, comp_cast(component)->identifier, toLocalPcmStreamType(streamType), 0) != 0) {
    return SK_ERROR_FAILED_QUERYING_DEVICE;
  }
  result = skPopulateComponentLimitsIMPL(handle, pLimits);
  snd_pcm_close(handle);
  return result;
}

#define PCM_CHECK(call) if ((error = call) < 0) { SKWARNIF(error != -EINVAL, "%s\n", snd_strerror(error)); continue; }
static SkResult
skRequestPcmStreamIMPL(
  SkComponent                       component,
  SkPcmStreamRequest*               pStreamRequest,
  SkStream*                         pStream,
  const SkAllocationCallbacks*      pAllocator
) {
  int error;
  int iValue;
  int accessMode;
  unsigned int uiValue;
  snd_pcm_uframes_t ufValue;
  snd_pcm_access_t aValue;
  snd_pcm_format_t fValue;
  snd_pcm_stream_t streamType;
  snd_pcm_t* pcmHandle;
  snd_pcm_hw_params_t *hwParams;
  SkPcmStreamInfo streamInfo;
  SkResult result;
  SkStream stream;
  SkFormat formatType;
  SkFormat formatIterator;
  SkRangedValue rangedValue;

  // Translate to local PCM definitions
  accessMode = toLocalPcmAccessMode(pStreamRequest->accessMode);
  streamType = toLocalPcmStreamType(pStreamRequest->streamType);

  // Open the PCM stream
  // Note: Will not open if the device is busy.
  error = snd_pcm_open(&pcmHandle, comp_cast(component)->identifier, streamType, accessMode);
  if (error < 0) {
    SKWARN("%s\n", snd_strerror(error));
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Allocate hardware parameters
  error = snd_pcm_hw_params_malloc(&hwParams);
  if (error < 0) {
    snd_pcm_close(pcmHandle);
    SKWARN("%s\n", snd_strerror(error));
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Enumerate format types until one of the formats works
  // Note: If the user passed a non-dynamic format, this will only loop once.
  formatIterator = SK_FORMAT_INVALID;
  while (skEnumerateFormats(pStreamRequest->formatType, &formatIterator, &formatType)) {
    // Initial configuration
    PCM_CHECK(snd_pcm_hw_params_any(pcmHandle, hwParams));

    // Format (Default = First)
    if (formatType != SK_FORMAT_ANY) {
      snd_pcm_format_t format = toLocalPcmFormatType(formatType);
      PCM_CHECK(snd_pcm_hw_params_set_format(pcmHandle, hwParams, format));
    }

    // Access Type (Default = First)
    if (pStreamRequest->accessType != SK_ACCESS_TYPE_ANY) {
      snd_pcm_access_t access = toLocalPcmAccessType(pStreamRequest->accessType);
      PCM_CHECK(snd_pcm_hw_params_set_access(pcmHandle, hwParams, access));
    }

    // Channels (Default = Minimum)
    if (pStreamRequest->channels != 0) {
      uiValue = pStreamRequest->channels;
      PCM_CHECK(snd_pcm_hw_params_set_channels_near(pcmHandle, hwParams, &uiValue));
    }

    // Rate (Default = Minimum)
    if (pStreamRequest->sampleRate != 0) {
      uiValue = pStreamRequest->sampleRate; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_rate_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Period (Default = Minimum)
    if (pStreamRequest->periods != 0) {
      uiValue = pStreamRequest->periods; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_periods_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Period Time (Default = Minimum)
    if (pStreamRequest->periodSize > 0) {
      ufValue = pStreamRequest->periodSize; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_period_size_near(pcmHandle, hwParams, &ufValue, &iValue));
    }
    else if (pStreamRequest->periodTime > 0) {
      uiValue = pStreamRequest->periodTime; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_period_time_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Buffer Size (Default = Maximum)
    if (pStreamRequest->bufferSize > 0) {
      ufValue = pStreamRequest->bufferSize; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_buffer_size_near(pcmHandle, hwParams, &ufValue));
    }
    else if (pStreamRequest->bufferTime > 0) {
      uiValue = pStreamRequest->bufferSize; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_buffer_time_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Success!
    break;
  }

  // If we have an invalid format, we've failed to find a valid configuration
  if (formatType == SK_FORMAT_INVALID) {
    snd_pcm_close(pcmHandle);
    snd_pcm_hw_params_free(hwParams);
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Apply the hardware parameters to the PCM stream
  error = snd_pcm_hw_params(pcmHandle, hwParams);
  if (error < 0) {
    snd_pcm_close(pcmHandle);
    snd_pcm_hw_params_free(hwParams);
    SKWARN("%s\n", snd_strerror(error));
    return SK_ERROR_STREAM_REQUEST_FAILED;
  }

  // Gather the configuration space information
  do {
    streamInfo.streamType = pStreamRequest->streamType;
    streamInfo.accessMode = pStreamRequest->accessMode;
    PCM_CHECK(snd_pcm_hw_params_get_access(hwParams, &aValue));
    streamInfo.accessType = toSkAccessType(aValue);
    PCM_CHECK(snd_pcm_hw_params_get_format(hwParams, &fValue));
    streamInfo.formatType = toSkFormat(fValue);
    streamInfo.formatBits = (uint32_t) snd_pcm_format_physical_width(fValue);
    PCM_CHECK(snd_pcm_hw_params_get_periods(hwParams, &uiValue, &iValue));
    streamInfo.periods = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_channels(hwParams, &uiValue));
    streamInfo.channels = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_period_size(hwParams, &ufValue, &iValue));
    streamInfo.periodSamples = ufValue;
    PCM_CHECK(snd_pcm_hw_params_get_period_time(hwParams, &uiValue, &iValue));
    streamInfo.periodTime = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_buffer_size(hwParams, &ufValue));
    streamInfo.bufferSamples = ufValue;
    PCM_CHECK(snd_pcm_hw_params_get_buffer_time(hwParams, &uiValue, &iValue));
    streamInfo.bufferTime = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_rate(hwParams, &uiValue, &iValue));
    streamInfo.sampleRate = uiValue;
    streamInfo.sampleBits = streamInfo.channels * streamInfo.formatBits;
    streamInfo.bufferBits = streamInfo.bufferSamples * streamInfo.sampleBits;
    streamInfo.sampleTime = streamInfo.periodTime / streamInfo.periodSamples;
    streamInfo.periodBits = streamInfo.sampleBits * streamInfo.periodSamples;
    // latency = periodsize * periods / (rate * bytes_per_frame)
    streamInfo.latency = 8 * streamInfo.periodSamples * streamInfo.periods / (streamInfo.sampleRate * streamInfo.sampleBits);
  } while (SK_FALSE);
  snd_pcm_hw_params_free(hwParams);

  // Success! Create the stream -
  result = skConstructStream(component, sizeof(SkStream_ALSA), pAllocator, &stream);
  if (result != SK_SUCCESS) {
    snd_pcm_close(pcmHandle);
    return result;
  }
  stream->streamInfo.pcm = streamInfo;
  stream_cast(stream)->handle = pcmHandle;
  switch (streamInfo.streamType) {
    case SK_STREAM_TYPE_PCM_CAPTURE:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          stream_cast(stream)->pfn.readi_func = &snd_pcm_readi;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          stream_cast(stream)->pfn.readn_func = &snd_pcm_readn;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          stream_cast(stream)->pfn.readi_func = &snd_pcm_mmap_readi;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          stream_cast(stream)->pfn.readn_func = &snd_pcm_mmap_readn;
          break;
        default:
          break;
      }
      break;
    case SK_STREAM_TYPE_PCM_PLAYBACK:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          stream_cast(stream)->pfn.writei_func = &snd_pcm_writei;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          stream_cast(stream)->pfn.writen_func = &snd_pcm_writen;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          stream_cast(stream)->pfn.writei_func = &snd_pcm_mmap_writei;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          stream_cast(stream)->pfn.writen_func = &snd_pcm_mmap_writen;
          break;
        default:
          break;
      }
      break;
  }

  *pStream = stream;
  return SK_SUCCESS;
}

static SkResult skRequestStream_ALSA(
  SkComponent                       component,
  SkStreamRequest*                  pStreamRequest,
  SkStream*                         pStream,
  const SkAllocationCallbacks*      pAllocator
) {
  switch (pStreamRequest->streamType) {
    case SK_STREAM_TYPE_PCM_PLAYBACK:
    case SK_STREAM_TYPE_PCM_CAPTURE:
      return skRequestPcmStreamIMPL(component, &pStreamRequest->pcm, pStream, pAllocator);
    case SK_STREAM_TYPE_NONE:
      SKERROR("An SkStreamType other than SK_STREAM_TYPE_NONE must be provided in SkStreamRequest.\n");
      return SK_ERROR_INITIALIZATION_FAILED;
  }
}

static int64_t
skStreamWriteInterleaved_ALSA(
  SkStream                            stream,
  void const*                         pBuffer,
  uint32_t                            samples
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream_cast(stream)->pfn.writei_func(stream_cast(stream)->handle, pBuffer, samples)) < 0) {
      SKWARN("Write Error!\n");
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream_cast(stream)->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

static int64_t
skStreamWriteNoninterleaved_ALSA(
  SkStream                            stream,
  void const* const*                  pBuffer,
  uint32_t                            samples
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream_cast(stream)->pfn.writen_func(stream_cast(stream)->handle, (void**)pBuffer, samples)) < 0) {
      SKWARN("Write Error!\n");
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream_cast(stream)->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

static int64_t
skStreamReadInterleaved_ALSA(
  SkStream                            stream,
  void*                               pBuffer,
  uint32_t                            samples
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream_cast(stream)->pfn.readi_func(stream_cast(stream)->handle, pBuffer, samples)) < 0) {
      SKWARN("Read Error!\n");
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream_cast(stream)->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

static int64_t
skStreamReadNoninterleaved_ALSA(
  SkStream                            stream,
  void**                              pBuffer,
  uint32_t                            samples
) {
  snd_pcm_sframes_t err = 0;
  for (;;) {
    if ((err = stream_cast(stream)->pfn.readn_func(stream_cast(stream)->handle, pBuffer, samples)) < 0) {
      SKWARN("Read Error!\n");
      if (err == -EAGAIN) continue;
      if ((err = xrun_recovery(stream_cast(stream)->handle, err)) < 0) {
        return -SK_ERROR_FAILED_STREAM_WRITE;
      }
      continue;
    }
    break;
  }
  return err;
}

static void
skDestroyStream_ALSA(
  SkStream                          stream,
  SkBool32                          drain
) {
  SkStream *pStream;

  // Drain and close PCM stream
  if (drain) {
    snd_pcm_drain(stream_cast(stream)->handle);
  }
  snd_pcm_close(stream_cast(stream)->handle);

  // Remove stream and destroy
  pStream = &stream->component->pStreams;
  while (*pStream && *pStream != stream) {
    pStream = &(*pStream)->pNext;
  }
  if (*pStream) {
    *pStream = (*pStream)->pNext;
  }

  _SKFREE(stream->component->device->hostApi->instance->pAllocator, stream);
}

////////////////////////////////////////////////////////////////////////////////
// Public API (Initializer)
////////////////////////////////////////////////////////////////////////////////
SkResult skHostApiInit_ALSA(
  SkInstance                        instance,
  SkHostApi*                        pHostApi,
  const SkAllocationCallbacks*      pAllocator
) {
  SkResult result;
  SkHostApi hostApi;

  // Initialize the Host API's data
  result = skConstructHostApi(instance, sizeof(SkHostApi_ALSA), pAllocator, &hostApi);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Initialize the Host API's properties
  strcpy(hostApi->properties.identifier, "alsa");
  strcpy(hostApi->properties.hostName, "ALSA");
  strcpy(hostApi->properties.hostNameFull, "Advanced Linux Sound Architecture (ALSA)");
  strcpy(hostApi->properties.description, "User space library (alsa-lib) to simplify application programming and provide higher level functionality.");
  hostApi->properties.capabilities = SK_HOST_CAPABILITIES_PCM | SK_HOST_CAPABILITIES_SEQUENCER;

  // Initialize the Host API's implementation
  hostApi->impl.SkHostApiFree = &skHostApiFree_ALSA;
  hostApi->impl.SkScanDevices = &skScanDevices_ALSA;
  hostApi->impl.SkGetComponentLimits = &skGetComponentLimits_ALSA;
  hostApi->impl.SkRequestStream = &skRequestStream_ALSA;
  hostApi->impl.SkStreamWriteInterleaved = &skStreamWriteInterleaved_ALSA;
  hostApi->impl.SkStreamWriteNoninterleaved = &skStreamWriteNoninterleaved_ALSA;
  hostApi->impl.SkStreamReadInterleaved = &skStreamReadInterleaved_ALSA;
  hostApi->impl.SkStreamReadNoninterleaved = &skStreamReadNoninterleaved_ALSA;
  hostApi->impl.SkDestroyStream = &skDestroyStream_ALSA;

  *pHostApi = hostApi;
  return SK_SUCCESS;
}
