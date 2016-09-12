/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard implementation. (ALSA Implementation)
 ******************************************************************************/

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/dev/assert.h>
#include <OpenSK/dev/hosts.h>
#include <OpenSK/dev/macros.h>

// C99
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ALSA
#include <alsa/asoundlib.h>

// UDEV
#ifdef    PLUGIN_UDEV
#include <libudev.h>
#include <ctype.h>
#endif // PLUGIN_UDEV

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////

#define SK_ALSA_VIRTUAL_DEVICE_CARD_ID                                        -1
#define SK_ALSA_MAX_IDENTIFIER_SIZE                                          256

#define host_cast(h) ((SkHostApi_ALSA*)h)
typedef struct SkHostApi_ALSA {
  SkHostApi_T                           parent;
#ifdef    PLUGIN_UDEV
  struct udev*                          udev;
#endif // PLUGIN_UDEV
} SkHostApi_ALSA;

#define dev_cast(d) ((SkDevice_ALSA*)d)
typedef struct SkDevice_ALSA {
  SkDevice_T                            parent;
  int                                   card;
  char                                  identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];
} SkDevice_ALSA;

#define comp_cast(c) ((SkComponent_ALSA*)c)
typedef struct SkComponent_ALSA {
  SkComponent_T                         parent;
  int                                   device;
  char                                  identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];
} SkComponent_ALSA;

#define stream_cast(s) ((SkStream_ALSA*)s)
typedef struct SkStream_ALSA {
  SkStream_T                            parent;
  snd_pcm_t*                            handle;
} SkStream_ALSA;

////////////////////////////////////////////////////////////////////////////////
// Conversion Functions
////////////////////////////////////////////////////////////////////////////////

static snd_pcm_stream_t skToLocalPcmStreamIMPL(
  SkStreamType                          streamType
) {
  switch (streamType) {
    case SK_STREAM_TYPE_PCM_CAPTURE:
      return SND_PCM_STREAM_CAPTURE;
    case SK_STREAM_TYPE_PCM_PLAYBACK:
      return SND_PCM_STREAM_PLAYBACK;
    default:
      SKERROR("Should never consume a non-PCM stream type.\n");
      return SND_PCM_STREAM_PLAYBACK;
  }
}

static snd_pcm_format_t skToLocalPcmFormatIMPL(
  SkFormat                              format
) {
  switch (format) {
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
    case SK_FORMAT_FIRST:
    case SK_FORMAT_FIRST_DYNAMIC:
    case SK_FORMAT_FIRST_STATIC:
    case SK_FORMAT_LAST:
    case SK_FORMAT_LAST_DYNAMIC:
    case SK_FORMAT_LAST_STATIC:
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
      return skToLocalPcmFormatIMPL(skGetFormatStatic(format));
  }

  return SK_FORMAT_INVALID;
}

static SkFormat skFromLocalPcmFormatTypeIMPL(
  snd_pcm_format_t                      format
) {
  switch (format) {
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

static int skToLocalPcmAccessModeIMPL(
  SkAccessMode                          accessMode
) {
  switch (accessMode) {
    case SK_ACCESS_MODE_BLOCK:
      return 0;
    case SK_ACCESS_MODE_NONBLOCK:
      return SND_PCM_NONBLOCK;
  }

  return -1;
}

static snd_pcm_access_t skToLocalPcmAccessTypeIMPL(
  SkAccessType                          accessType
) {
  switch (accessType) {
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
  return -1;
}

static SkAccessType skFromLocalPcmAccessTypeIMPL(
  snd_pcm_access_t                      accessType
) {
  switch (accessType) {
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

  return -1;
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static int skDecodeHexCharacterIMPL(
  char                                  character
) {
  character = (char)tolower(character);
  if (character >= '0' && character <= '9') {
    return (character - '0');
  }
  else if (character >= 'a' && character <= 'f') {
    return (character - 'a' + 10);
  }
  return -1;
}

static int skDecodeCStrEscapeSequenceIMPL(
  char**                                pDestination,
  char const**                          pSource,
  size_t*                               pLength
) {
  int digits[2];
  switch (**pSource) {
    case '\\':
      **pDestination = '\\';
      ++*pDestination;
      --*pLength;
      break;
    case 'x':
    case 'X':
      ++*pSource;
      digits[0] = skDecodeHexCharacterIMPL(**pSource) * 16;
      if (digits[0] == -1) {
        return -1;
      }
      ++*pSource;
      digits[1] = skDecodeHexCharacterIMPL(**pSource);
      if (digits[1] == -1) {
        return -1;
      }
      **pDestination = (char)(digits[0] + digits[1]);
      ++*pDestination;
      --*pLength;
      break;
    default:
      return -1;
  }

  return 0;
}

static int skDecodeCStrIMPL(
  char*                                 pDestination,
  char const*                           pSource,
  size_t                                length
) {
  // Note: Always remove one length value for endline
  --length;
  while (*pSource && length) {
    switch (*pSource) {
      case '\\':
        ++pSource;
        if (skDecodeCStrEscapeSequenceIMPL(&pDestination, &pSource, &length) == -1) {
          return -1;
        }
        break;
      default:
        --length;
        *pDestination++ = *pSource++;
        break;
    }
  }
  *pDestination = '\0';
  return 0;
}

static int skXRunResoveryIMPL(
  snd_pcm_t*                            handle,
  snd_pcm_sframes_t                     err
) {
  switch (err) {
    // An underrun/overrun occurred, open the stream back up and try again
    case -EPIPE:
      err = snd_pcm_prepare(handle);
      SKWARNIF(err < 0, "Stream was unable to recover from xrun: %s\n", snd_strerror(err));
      return (err >= 0) ? -SK_ERROR_STREAM_XRUN : -SK_ERROR_STREAM_INVALID;
      // Device is not yet ready, try again later
    case -ESTRPIPE:
      err = snd_pcm_resume(handle);
      switch (err) {
        case -EAGAIN:
          return -SK_ERROR_STREAM_BUSY;
        default:
          SKWARN("Stream encountered an unexpected error after resuming: %s\n", snd_strerror(err));
        case -ENOSYS:
          err = snd_pcm_prepare(handle);
          SKWARNIF(err < 0, "Stream was unable to recover from xrun: %s\n", snd_strerror(err));
          return (err >= 0) ? -SK_ERROR_STREAM_NOT_READY : -SK_ERROR_STREAM_INVALID;
      }
    case -EAGAIN:
      return -SK_ERROR_STREAM_BUSY;
    case -EBADFD:
      return -SK_ERROR_STREAM_INVALID;
    case -ENOTTY:
    case -ENODEV:
      return -SK_ERROR_STREAM_LOST;
    default:
      SKWARN("Stream is in an unexpected state: (%d) %s\n", (int)err, snd_strerror(err));
      return -SK_ERROR_STREAM_INVALID;
  }
}

#define CHECK(op) if ((op) < 0) { return -SK_ERROR_UNKNOWN; }
static snd_pcm_sframes_t skUpdateStreamStateIMPL(
  SkStream_ALSA*                        stream
) {
  for (;;) {
    switch (snd_pcm_state(stream->handle)) {
      case SND_PCM_STATE_OPEN:
        CHECK(snd_pcm_prepare(stream->handle));
        return SK_SUCCESS;
      case SND_PCM_STATE_PREPARED:
        switch (stream->parent.streamInfo.pcm.streamType) {
          case SK_STREAM_TYPE_NONE:
            return SK_ERROR_STREAM_INVALID;
          case SK_STREAM_TYPE_PCM_PLAYBACK:
            return SK_SUCCESS;
          case SK_STREAM_TYPE_PCM_CAPTURE:
            CHECK(snd_pcm_start(stream->handle));
            break;
        }
        break;
      case SND_PCM_STATE_RUNNING:
        return SK_SUCCESS;
      case SND_PCM_STATE_XRUN:
        return skXRunResoveryIMPL(stream->handle, -EPIPE);
      case SND_PCM_STATE_SUSPENDED:
        return skXRunResoveryIMPL(stream->handle, -ESTRPIPE);
      default:
        return SK_SUCCESS;
    }
  }
}
#undef CHECK

static snd_pcm_sframes_t skGetRemainingFramesIMPL(
  SkStream_ALSA*                        stream
) {
  snd_pcm_sframes_t frames;

  for (;;) {

    // Update the internal state of the stream
    frames = skUpdateStreamStateIMPL(stream);
    if (frames < 0) {
      return frames;
    }

    // Handle polling operations if requested
    if (stream->parent.streamInfo.pcm.streamFlags & SK_STREAM_FLAGS_POLL_AVAILABLE) {

      // See how many frames are available
      // NOTE: On xrun, we might have to restart the stream
      frames = snd_pcm_avail(stream->handle);
      if (frames < 0) {
        return skXRunResoveryIMPL(stream->handle, frames);
      }

      // Handle waiting operation is requested
      // NOTE: On xrun, we might have to restart the stream
      if (stream->parent.streamInfo.pcm.streamFlags & SK_STREAM_FLAGS_WAIT_AVAILABLE) {
        if (!frames) {
          frames = snd_pcm_wait(stream->handle, -1);
          if (frames < 0) {
            return skXRunResoveryIMPL(stream->handle, frames);
          }
          continue;
        }
      }

      return frames;
    }

    // No polling, assume max availability
    return stream->parent.streamInfo.pcm.bufferSamples;
  }
}

#define skStreamOperationIMPL(fn,cast)                                        \
snd_pcm_sframes_t err = skGetRemainingFramesIMPL(stream_cast(stream));        \
if (err > 0) {                                                                \
  err = fn(stream_cast(stream)->handle, (cast)pBuffer, SKMIN(samples, err));  \
  if (err < 0) {                                                              \
    return skXRunResoveryIMPL(stream_cast(stream)->handle, err);              \
  }                                                                           \
}                                                                             \
return (int64_t)err

////////////////////////////////////////////////////////////////////////////////
// Device Management Functions
////////////////////////////////////////////////////////////////////////////////

static SkDevice skGetDeviceByCardIMPL(
  SkDevice                              pDevices,
  int                                   card
) {
  while (pDevices) {
    if (dev_cast(pDevices)->card == card) break;
    pDevices = pDevices->pNext;
  }
  return pDevices;
}

static SkDevice skGetDeviceByIdentifierIMPL(
  SkDevice                              pDevices,
  char const*                           identifier
) {
  while (pDevices) {
    if (strcmp(dev_cast(pDevices)->identifier, identifier) == 0)
      return pDevices;
    pDevices = pDevices->pNext;
  }
  return NULL;
}

static SkComponent skGetComponentByDeviceIMPL(
  SkComponent                           pComponents,
  int                                   device
) {
  while (pComponents) {
    if (comp_cast(pComponents)->device == device) break;
    pComponents = pComponents->pNext;
  }
  return pComponents;
}

static SkComponent skGetComponentByIdentifierIMPL(
  SkComponent                           pComponents,
  char const*                           identifier
) {
  while (pComponents) {
    if (strcmp(comp_cast(pComponents)->identifier, identifier) == 0)
      return pComponents;
    pComponents = pComponents->pNext;
  }
  return NULL;
}

static SkResult skPopulateComponentLimitsIMPL(
  snd_pcm_t*                            handle,
  SkComponentLimits*                    limits
) {
  int error;
  int iValue;
  unsigned int uiValue;
  SkFormat skFormat;
  snd_pcm_uframes_t uframesValue;
  snd_pcm_hw_params_t *params;
  snd_pcm_format_t localFormat;

  // Allocate hardware parameters
  error = snd_pcm_hw_params_malloc(&params);
  if (error < 0) {
    SKWARN("%s\n", snd_strerror(error));
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initial configuration
  snd_pcm_hw_params_any(handle, params);

  memset(limits->supportedFormats, 0, sizeof(limits->supportedFormats));
  for (skFormat = SK_FORMAT_STATIC_BEGIN; skFormat <= SK_FORMAT_STATIC_END; ++skFormat) {
    localFormat = skToLocalPcmFormatIMPL(skFormat);
    if (snd_pcm_hw_params_set_format(handle, params, localFormat) == 0) {
      limits->supportedFormats[skFormat] = SK_TRUE;
    }
    else {
      limits->supportedFormats[skFormat] = SK_FALSE;
    }
    snd_pcm_hw_params_any(handle, params);
  }
  limits->supportedFormats[SK_FORMAT_ANY] = SK_TRUE;

  if (snd_pcm_hw_params_get_buffer_size_max(params, &uframesValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->maxFrameSize = uframesValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_buffer_size_min(params, &uframesValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->minFrameSize = uframesValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_buffer_time_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  skTimePeriodSetQuantum(limits->maxBufferTime, uiValue, SK_TIME_UNITS_MICROSECONDS);
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_buffer_time_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  skTimePeriodSetQuantum(limits->minBufferTime, uiValue, SK_TIME_UNITS_MICROSECONDS);
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_channels_max(params, &uiValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->maxChannels = uiValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_channels_min(params, &uiValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->minChannels = uiValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_period_size_max(params, &uframesValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->maxPeriodSize = uframesValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_period_size_min(params, &uframesValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->minPeriodSize = uframesValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_period_time_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  skTimePeriodSetQuantum(limits->maxPeriodTime, uiValue, SK_TIME_UNITS_MICROSECONDS);
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_period_time_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  skTimePeriodSetQuantum(limits->minPeriodTime, uiValue, SK_TIME_UNITS_MICROSECONDS);
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_periods_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->maxPeriods = uiValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_periods_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->minPeriods = uiValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_rate_max(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->maxRate = uiValue;
  snd_pcm_hw_params_any(handle, params);

  if (snd_pcm_hw_params_get_rate_min(params, &uiValue, &iValue) < 0) {
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }
  limits->minRate = uiValue;
  snd_pcm_hw_params_any(handle, params);

  return SK_SUCCESS;
}

static SkResult skResolveComponentIMPL(
  SkDevice                              device,
  int                                   deviceId,
  char const*                           identifier,
  SkComponent*                          pComponent
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
    result = skCreateComponent(device, sizeof(SkComponent_ALSA), &component);
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

static SkResult skResolveDeviceIMPL(
  SkHostApi                             hostApi,
  int                                   cardId,
  char const*                           identifier,
  SkDevice*                             pDevice
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
    result = skCreateDevice(hostApi, sizeof(SkDevice_ALSA), (cardId >= 0) ? SK_TRUE : SK_FALSE, &device);
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

static SkResult skUpdatePhysicalComponentIMPL(
  snd_ctl_t*                            ctlHandle,
  SkDevice                              device,
  int                                   deviceId
) {
  int error;
  SkResult result;
  snd_pcm_info_t *info;
  SkComponent component;
  SkStreamType streamType;
  char identifier[SK_ALSA_MAX_IDENTIFIER_SIZE];

  // Resolve or construct the component
  sprintf(identifier, "hw:%d,%d", dev_cast(device)->card, deviceId);
  result = skResolveComponentIMPL(device, deviceId, identifier, &component);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Get information about the name of the device
  snd_pcm_info_malloc(&info);
  snd_pcm_info_set_device(info, (unsigned int)deviceId);
  snd_pcm_info_set_subdevice(info, 0);
  for (streamType = SK_STREAM_TYPE_PCM_BEGIN; streamType != SK_STREAM_TYPE_PCM_END; streamType <<= 1) {
    snd_pcm_info_set_stream(info, skToLocalPcmStreamIMPL(streamType));
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

static SkResult skUpdatePhysicalDeviceIMPL(
  SkHostApi                             hostApi,
  unsigned int                          cardId
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
  result = skResolveDeviceIMPL(hostApi, cardId, identifier, &device);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Attempt to acquire the card
  error = snd_ctl_open(&ctlHandle, identifier, 0);
  if (error != 0) {
    SKERROR("ALSA: %s\n", snd_strerror(error));
    return SK_ERROR_DEVICE_QUERY_FAILED;
  }

  // Iterate through all of the components on the device
  deviceId = -1;
  for (;;) {
    if (snd_ctl_pcm_next_device(ctlHandle, &deviceId) != 0) {
      snd_ctl_close(ctlHandle);
      return SK_ERROR_DEVICE_QUERY_FAILED;
    }
    if (deviceId <= -1) break;
    opResult = skUpdatePhysicalComponentIMPL(ctlHandle, device, deviceId);
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

#ifdef    PLUGIN_UDEV

/**
 * TODO: Unfortunately, after much research, I've found that libudev decides
 *       a lot of things in a fairly hacky way. For example, form factor is
 *       determined by a string compare with the product name. If the name
 *       contains a string like "[sS]peaker", the form factor is "speaker".
 *       The problem with this is not manny products say what they are in the
 *       product name. Not to mention this information _is_ available by the
 *       USB standard in a safer way. The underlying problem seems to be that
 *       sysfs doesn't make this information available, and libdev is built
 *       on sysfs information.
 *       For now, we accept this shortcoming - in the future we may have to
 *       construct more reliable code to extract this information.
 */
static SkResult skUpdatePhysicalUdevDeviceIMPL(
  SkHostApi                             hostApi,
  struct udev_device*                   uParent,
  struct udev_device*                   uDevice
) {
  SkBool32 flag;
  int cardNumber;
  char const* pCharConst;
  SkDevice pDevice;
  SkDeviceProperties *deviceProperties;

  // Find the audio subsystem:ALSA device pairing.
  pCharConst = udev_device_get_sysnum(uDevice);
  if (!pCharConst)
  {
    SKWARN("Found an audio subsystem device without a system number.");
    return SK_INCOMPLETE;
  }
  cardNumber = atoi(pCharConst);
  pDevice = skGetDeviceByCardIMPL(hostApi->pDevices, cardNumber);
  if (!pDevice)
  {
    SKWARN("Found an audio subsystem device without a corresponding ALSA device.");
    return SK_INCOMPLETE;
  }
  deviceProperties = &pDevice->properties;

  // Get the subsystem (transport type)
  pCharConst = udev_device_get_subsystem(uParent);
  if (!pCharConst) {
    pCharConst = udev_device_get_sysname(uParent);
    SKWARN("Undefined transport type for device: '%s'.\n", pCharConst);
    deviceProperties->transportType = SK_TRANSPORT_TYPE_UNDEFINED;
  }
  else if (strcmp(pCharConst, "usb") == 0) {
    deviceProperties->transportType = SK_TRANSPORT_TYPE_USB;
  }
  else if (strcmp(pCharConst, "pci") == 0) {
    deviceProperties->transportType = SK_TRANSPORT_TYPE_PCI;
  }
  // NOTE: ALSA doesn't readily support bluetooth, leaving this in in case it changes.
  else if (strcmp(pCharConst, "bluetooth") == 0) {
    deviceProperties->transportType = SK_TRANSPORT_TYPE_BLUETOOTH;
  }
  // TODO: Theoretical - Need someone to test this!
  else if (strcmp(pCharConst, "firewire") == 0) {
    deviceProperties->transportType = SK_TRANSPORT_TYPE_FIREWIRE;
  }
  // TODO: Theoretical - Need someone to test this!
  else if (strcmp(pCharConst, "thunderbolt") == 0) {
    deviceProperties->transportType = SK_TRANSPORT_TYPE_THUNDERBOLT;
  }
  else {
    SKWARN("Unknown transport type: '%s'.\n", pCharConst);
    deviceProperties->transportType = SK_TRANSPORT_TYPE_UNKNOWN;
  }

  // Get the form factor
  pCharConst = udev_device_get_property_value(uDevice, "SOUND_FORM_FACTOR");
  if (!pCharConst) {
    deviceProperties->formFactor = SK_FORM_FACTOR_UNDEFINED;
  }
  else if (strcmp(pCharConst, "internal") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_INTERNAL;
  }
  else if (strcmp(pCharConst, "microphone") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_MICROPHONE;
  }
  else if (strcmp(pCharConst, "speaker") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_SPEAKER;
  }
  else if (strcmp(pCharConst, "headphone") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_HEADPHONE;
  }
  else if (strcmp(pCharConst, "headset") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_HEADSET;
  }
  else if (strcmp(pCharConst, "webcam") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_WEBCAM;
  }
  else if (strcmp(pCharConst, "headset") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_HEADSET;
  }
  else if (strcmp(pCharConst, "handset") == 0) {
    deviceProperties->formFactor = SK_FORM_FACTOR_HANDSET;
  }
  else {
    SKWARN("Unknown form factor: '%s'.\n", pCharConst);
    deviceProperties->formFactor = SK_FORM_FACTOR_UNKNOWN;
  }

  // Get the Vendor name
  // Note: Flag denotes whether or not the string is encoded
  flag = SK_FALSE;
  pCharConst = udev_device_get_property_value(uDevice, "ID_VENDOR_FROM_DATABASE");
  if (!pCharConst) {
    flag = SK_TRUE;
    pCharConst = udev_device_get_property_value(uDevice, "ID_VENDOR_ENC");
  }
  if (!pCharConst) {
    flag = SK_FALSE;
    pCharConst = udev_device_get_property_value(uDevice, "ID_VENDOR");
  }
  if (pCharConst) {
    if (!flag || skDecodeCStrIMPL(deviceProperties->vendorName, pCharConst, SK_MAX_VENDOR_NAME_SIZE) == -1) {
      strncpy(deviceProperties->vendorName, pCharConst, SK_MAX_VENDOR_NAME_SIZE);
    }
  }

  // Get the Model name
  // Note: Flag denotes whether or not the string is encoded
  flag = SK_FALSE;
  pCharConst = udev_device_get_property_value(uDevice, "ID_MODEL_FROM_DATABASE");
  if (!pCharConst) {
    flag = SK_TRUE;
    pCharConst = udev_device_get_property_value(uDevice, "ID_MODEL_ENC");
  }
  if (!pCharConst) {
    flag = SK_FALSE;
    pCharConst = udev_device_get_property_value(uDevice, "ID_MODEL");
  }
  if (pCharConst) {
    if (!flag || skDecodeCStrIMPL(deviceProperties->modelName, pCharConst, SK_MAX_MODEL_NAME_SIZE) == -1) {
      strncpy(deviceProperties->modelName, pCharConst, SK_MAX_MODEL_NAME_SIZE);
    }
  }

  // Get the Serial name
  pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL");
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL_SHORT");
  }
  if (pCharConst) {
    strncpy(deviceProperties->serialName, pCharConst, SK_MAX_SERIAL_NAME_SIZE);
  }

  // Get Vendor ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_VENDOR_ID");
  if (pCharConst) {
    strncpy(deviceProperties->vendorUUID, pCharConst, SK_UUID_SIZE);
  }

  // Get Model ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_MODEL_ID");
  if (pCharConst) {
    strncpy(deviceProperties->modelUUID, pCharConst, SK_UUID_SIZE);
  }

  // Get Serial ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL");
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL_SHORT");
  }
  if (pCharConst) {
    strncpy(deviceProperties->serialUUID, pCharConst, SK_UUID_SIZE);
  }

  // Generate device ID
  // NOTE: This is one of: ID_FOR_SEAT, ID_PATH, ID_PATH_TAG, DEVPATH in that order
  //       Fallback is {vendorUUID}{modelUUID}{serialUUID} all appended together.
  pCharConst = udev_device_get_property_value(uDevice, "ID_FOR_SEAT");
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "ID_PATH");
  }
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "ID_PATH_TAG");
  }
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "DEVPATH");
  }
  if (pCharConst) {
    strncpy(deviceProperties->deviceUUID, pCharConst, SK_UUID_SIZE);
  }
  else {
    char *out = deviceProperties->deviceUUID;
    char const* end = out + SK_UUID_SIZE;
    strncpy(out, deviceProperties->vendorUUID, end - out);
    out += strlen(out);
    strncpy(out, deviceProperties->modelUUID, end - out);
    out += strlen(out);
    strncpy(out, deviceProperties->serialUUID, end - out);
  }

  return SK_SUCCESS;
}

static SkResult skUpdatePhysicalUdevComponentIMPL(
  SkHostApi                             hostApi,
  struct udev_device*                   uParent,
  struct udev_device*                   uDevice
) {
  // Right now, we don't have any need for component-level UDEV.
  (void)hostApi;
  (void)uParent;
  (void)uDevice;
  return SK_ERROR_NOT_IMPLEMENTED;
}

static SkResult skUpdatePhysicalUdevIMPL(
  SkHostApi                             hostApi,
  struct udev_device*                   uDevice
) {
  char const* pCharConst;
  struct udev_device* uParent;

  uParent = udev_device_get_parent(uDevice);

  if (uParent) {
    // Check if this is a UDEV "card"
    pCharConst = udev_device_get_subsystem(uParent);
    if (strcmp(pCharConst, "sound") != 0) {
      return skUpdatePhysicalUdevDeviceIMPL(hostApi, uParent, uDevice);
    }
    // Otherwise, this must be a UDEV "device"
    else {
      return skUpdatePhysicalUdevComponentIMPL(hostApi, uParent, uDevice);
    }
  }

  return SK_ERROR_NOT_FOUND;
}

#endif // PLUGIN_UDEV

static SkResult skUpdateVirtualDeviceIMPL(
  SkHostApi                             hostApi,
  void*                                 hint
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
  result = skResolveDeviceIMPL(hostApi, SK_ALSA_VIRTUAL_DEVICE_CARD_ID, deviceName, &device);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Update virtual device properties
  strcpy(device->properties.deviceName, deviceName);
  strcpy(device->properties.driverName, "N/A");
  strcpy(device->properties.mixerName, "N/A");

  // Resolve or construct the component
  result = skResolveComponentIMPL(device, SK_ALSA_VIRTUAL_DEVICE_CARD_ID, componentName, &component);
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

////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////
static SkResult skHostApiInit_ALSA(
  SkInstance                            instance,
  SkHostApi*                            pHostApi
) {
  SkResult result;
  SkHostApi hostApi;

  // Initialize the Host API's data
  result = skCreateHostApi(instance, sizeof(SkHostApi_ALSA), &skProcedure_ALSA, &hostApi);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Initialize the Host API's properties
  strcpy(hostApi->properties.identifier, "alsa");
  strcpy(hostApi->properties.hostName, "ALSA");
  strcpy(hostApi->properties.hostNameFull, "Advanced Linux Sound Architecture (ALSA)");
  strcpy(hostApi->properties.description, "User space library (alsa-lib) to simplify application programming and provide higher level functionality.");
  hostApi->properties.capabilities = SK_HOST_CAPABILITIES_PCM | SK_HOST_CAPABILITIES_SEQUENCER;

  // If UDEV is linked in, add udev support
#ifdef    PLUGIN_UDEV
  host_cast(hostApi)->udev = udev_new();
  if (!host_cast(hostApi)->udev) {
    SKWARN("Cannot create udev instance, device hotswapping might not be accurate.\n");
  }
#endif // PLUGIN_UDEV

  *pHostApi = hostApi;
  return SK_SUCCESS;
}

static SkResult skHostApiScan_ALSA(
  SkHostApi                             hostApi
) {
  int card;
  void** hint;
  void** hints;
  SkResult result;
  SkResult opResult;

  result = SK_SUCCESS;

  // Physical Devices (ALSA)
  card = -1;
  for(;;) {
    // Attempt to open the next card
    if (snd_card_next(&card) != 0) {
      return SK_ERROR_DEVICE_QUERY_FAILED;
    }
    if (card == -1) break; // Last card

    // Attempt to construct the new card handle
    opResult = skUpdatePhysicalDeviceIMPL(hostApi, (unsigned int)card);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }

  // If udev support exists, flesh-out metadata.
#ifdef    PLUGIN_UDEV

  // Physical Devices (UDEV)
  {
    struct udev_enumerate* enumerate;
    struct udev_list_entry* devices, *iterator;
    struct udev_device* device;

    // Enumerate all "sound" subsystem devices
    enumerate = udev_enumerate_new(host_cast(hostApi)->udev);
    udev_enumerate_add_match_subsystem(enumerate, "sound");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    // Process each subsystem device
    udev_list_entry_foreach(iterator, devices) {
      char const *path;
      path = udev_list_entry_get_name(iterator);
      device = udev_device_new_from_syspath(host_cast(hostApi)->udev, path);
      skUpdatePhysicalUdevIMPL(hostApi, device);
      udev_device_unref(device);
    }
  }

#endif // PLUGIN_UDEV

  // Virtual Devices
  {
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
      return SK_ERROR_DEVICE_QUERY_FAILED;
    }
    hint = hints;

    while (*hint) {
      opResult = skUpdateVirtualDeviceIMPL(hostApi, *hint);
      if (opResult != SK_SUCCESS) {
        result = opResult;
      }
      ++hint;
    }
  }

  return result;
}

static SkResult skHostApiFree_ALSA(
  SkHostApi                             hostApi
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
        SKFREE(stream->pAllocator, stream);
        stream = nextStream;
      }
      SKFREE(component->pAllocator, component);
      component = nextComponent;
    }
    SKFREE(device->pAllocator, device);
    device = nextDevice;
  }

  // Free HostApi
  SKFREE(hostApi->pAllocator, hostApi);
  return SK_SUCCESS;
}

static SkResult skGetComponentLimits_ALSA(
  SkComponent                           component,
  SkStreamType                          streamType,
  SkComponentLimits*                    pLimits
) {
  SkResult result;
  snd_pcm_t *handle;

  if (snd_pcm_open(&handle, comp_cast(component)->identifier, skToLocalPcmStreamIMPL(streamType), 0) != 0) {
    return SK_ERROR_STREAM_BUSY;
  }
  result = skPopulateComponentLimitsIMPL(handle, pLimits);
  snd_pcm_close(handle);
  return result;
}

static int64_t skStreamWriteInterleaved_ALSA(
  SkStream                              stream,
  void const*                           pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_writei, void const*);
}

static int64_t skStreamWriteNoninterleaved_ALSA(
  SkStream                              stream,
  void const* const*                    pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_writen, void**);
}

static int64_t skStreamWriteInterleavedMMap_ALSA(
  SkStream                              stream,
  void const*                           pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_mmap_writei, void const*);
}

static int64_t skStreamWriteNoninterleavedMMap_ALSA(
  SkStream                              stream,
  void const* const*                    pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_mmap_writen, void**);
}

static int64_t skStreamReadInterleaved_ALSA(
  SkStream                              stream,
  void*                                 pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_readi, void*);
}

static int64_t skStreamReadNoninterleaved_ALSA(
  SkStream                              stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_readn, void**);
}

static int64_t skStreamReadInterleavedMMap_ALSA(
  SkStream                              stream,
  void*                                 pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_mmap_readi, void*);
}

static int64_t skStreamReadNoninterleavedMMap_ALSA(
  SkStream                              stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  skStreamOperationIMPL(snd_pcm_mmap_readn, void**);
}

#define PCM_CHECK(call) if ((error = call) < 0) { SKWARNIF(error != -EINVAL, "%s\n", snd_strerror(error)); continue; }
static SkResult skRequestPcmStreamIMPL(
  SkComponent                           component,
  SkPcmStreamInfo*                      pStreamRequest,
  SkStream*                             pStream
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

  // Translate to local PCM definitions
  accessMode = skToLocalPcmAccessModeIMPL(pStreamRequest->accessMode);
  streamType = skToLocalPcmStreamIMPL(pStreamRequest->streamType);

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
      snd_pcm_format_t format = skToLocalPcmFormatIMPL(formatType);
      PCM_CHECK(snd_pcm_hw_params_set_format(pcmHandle, hwParams, format));
    }

    // Access Type (Default = First)
    if (pStreamRequest->accessType != SK_ACCESS_TYPE_ANY) {
      snd_pcm_access_t access = skToLocalPcmAccessTypeIMPL(pStreamRequest->accessType);
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
    if (pStreamRequest->periodSamples > 0) {
      ufValue = pStreamRequest->periodSamples; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_period_size_near(pcmHandle, hwParams, &ufValue, &iValue));
    }
    else if (!skTimePeriodIsZero(pStreamRequest->periodTime)) {
      uiValue = (unsigned int)skTimePeriodToQuantum(pStreamRequest->periodTime, SK_TIME_UNITS_MICROSECONDS); iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_period_time_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Buffer Size (Default = Maximum)
    if (pStreamRequest->bufferSamples > 0) {
      ufValue = pStreamRequest->bufferSamples; iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_buffer_size_near(pcmHandle, hwParams, &ufValue));
    }
    else if (!skTimePeriodIsZero(pStreamRequest->bufferTime)) {
      uiValue = (unsigned int)skTimePeriodToQuantum(pStreamRequest->bufferTime, SK_TIME_UNITS_MICROSECONDS); iValue = 0;
      PCM_CHECK(snd_pcm_hw_params_set_buffer_time_near(pcmHandle, hwParams, &uiValue, &iValue));
    }

    // Success!
    break;
  }

  // If we have an invalid format, we've failed to find a valid configuration
  if (formatType == SK_FORMAT_INVALID) {
    snd_pcm_close(pcmHandle);
    snd_pcm_hw_params_free(hwParams);
    return SK_ERROR_STREAM_REQUEST_FAILED;
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
    streamInfo.accessType = skFromLocalPcmAccessTypeIMPL(aValue);
    streamInfo.streamFlags = pStreamRequest->streamFlags;
    PCM_CHECK(snd_pcm_hw_params_get_format(hwParams, &fValue));
    streamInfo.formatType = skFromLocalPcmFormatTypeIMPL(fValue);
    streamInfo.formatBits = (uint32_t) snd_pcm_format_physical_width(fValue);
    PCM_CHECK(snd_pcm_hw_params_get_channels(hwParams, &uiValue));
    streamInfo.channels = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_rate(hwParams, &uiValue, &iValue));
    streamInfo.sampleRate = uiValue;
    streamInfo.sampleBits = streamInfo.channels * streamInfo.formatBits;
    skTimePeriodSetQuantum(streamInfo.sampleTime, SK_TIME_QUANTUM_MAX / streamInfo.sampleRate, SK_TIME_UNITS_MIN);
    PCM_CHECK(snd_pcm_hw_params_get_periods(hwParams, &uiValue, &iValue));
    streamInfo.periods = uiValue;
    PCM_CHECK(snd_pcm_hw_params_get_period_size(hwParams, &ufValue, &iValue));
    streamInfo.periodSamples = (uint32_t)ufValue;
    streamInfo.periodBits = streamInfo.sampleBits * streamInfo.periodSamples;
    skTimePeriodSetQuantum(streamInfo.periodTime, SK_TIME_QUANTUM_MAX * ((float)streamInfo.periodSamples / streamInfo.sampleRate), SK_TIME_UNITS_MIN);
    PCM_CHECK(snd_pcm_hw_params_get_buffer_size(hwParams, &ufValue));
    streamInfo.bufferSamples = (uint32_t)ufValue;
    streamInfo.bufferBits = streamInfo.bufferSamples * streamInfo.sampleBits;
    skTimePeriodSetQuantum(streamInfo.bufferTime, SK_TIME_QUANTUM_MAX * ((float)streamInfo.bufferSamples / streamInfo.sampleRate), SK_TIME_UNITS_MIN);
  } while (SK_FALSE);
  snd_pcm_hw_params_free(hwParams);

  // Success! Create the stream -
  result = skCreateStream(component, sizeof(SkStream_ALSA), &stream);
  if (result != SK_SUCCESS) {
    snd_pcm_close(pcmHandle);
    snd_pcm_hw_params_free(hwParams);
    SKWARN("%s\n", snd_strerror(error));
    return result;
  }
  stream->streamInfo.pcm = streamInfo;
  stream_cast(stream)->handle = pcmHandle;
  switch (streamInfo.streamType) {
    case SK_STREAM_TYPE_NONE:
      return SK_ERROR_STREAM_INVALID;
    case SK_STREAM_TYPE_PCM_CAPTURE:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          stream->pcmFunctions.SkStreamReadInterleaved = &skStreamReadInterleaved_ALSA;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          stream->pcmFunctions.SkStreamReadNoninterleaved = &skStreamReadNoninterleaved_ALSA;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          stream->pcmFunctions.SkStreamReadInterleaved = &skStreamReadInterleavedMMap_ALSA;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          stream->pcmFunctions.SkStreamReadNoninterleaved = &skStreamReadNoninterleavedMMap_ALSA;
          break;
        default:
          break;
      }
      break;
    case SK_STREAM_TYPE_PCM_PLAYBACK:
      switch (streamInfo.accessType) {
        case SK_ACCESS_TYPE_INTERLEAVED:
          stream->pcmFunctions.SkStreamWriteInterleaved = &skStreamWriteInterleaved_ALSA;
          break;
        case SK_ACCESS_TYPE_NONINTERLEAVED:
          stream->pcmFunctions.SkStreamWriteNoninterleaved = &skStreamWriteNoninterleaved_ALSA;
          break;
        case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
          stream->pcmFunctions.SkStreamWriteInterleaved = &skStreamWriteInterleavedMMap_ALSA;
          break;
        case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
          stream->pcmFunctions.SkStreamWriteNoninterleaved = &skStreamWriteNoninterleavedMMap_ALSA;
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
  SkComponent                           component,
  SkStreamInfo*                         pStreamRequest,
  SkStream*                             pStream
) {
  switch (pStreamRequest->streamType) {
    case SK_STREAM_TYPE_PCM_PLAYBACK:
    case SK_STREAM_TYPE_PCM_CAPTURE:
      return skRequestPcmStreamIMPL(component, &pStreamRequest->pcm, pStream);
    case SK_STREAM_TYPE_NONE:
      SKERROR("An SkStreamType other than SK_STREAM_TYPE_NONE must be provided in SkStreamRequest.\n");
      return SK_ERROR_STREAM_REQUEST_FAILED;
  }

  return SK_ERROR_STREAM_REQUEST_FAILED;
}

static SkResult skGetStreamHandle_ALSA(
  SkStream                              stream,
  void**                                pHandle
) {
  *pHandle = stream_cast(stream)->handle;
  return SK_SUCCESS;
}

static void skDestroyStream_ALSA(
  SkStream                              stream,
  SkBool32                              drain
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

  SKFREE(stream->pAllocator, stream);
}

////////////////////////////////////////////////////////////////////////////////
// Public Procedure Call
////////////////////////////////////////////////////////////////////////////////
SkResult skProcedure_ALSA(
  SkProcedure                           procedure,
  void**                                params
) {
  switch (procedure) {
    case SK_PROC_HOSTAPI_INIT:
      return skHostApiInit_ALSA(SK_PARAMS_HOSTAPI_INIT);
    case SK_PROC_HOSTAPI_SCAN:
      return skHostApiScan_ALSA(SK_PARAMS_HOSTAPI_SCAN);
    case SK_PROC_HOSTAPI_FREE:
      return skHostApiFree_ALSA(SK_PARAMS_HOSTAPI_FREE);
    case SK_PROC_COMPONENT_GET_LIMITS:
      return skGetComponentLimits_ALSA(SK_PARAMS_COMPONENT_LIMITS);
    case SK_PROC_STREAM_REQUEST:
      return skRequestStream_ALSA(SK_PARAMS_STREAM_REQUEST);
    case SK_PROC_STREAM_DESTROY:
      skDestroyStream_ALSA(SK_PARAMS_STREAM_DESTROY);
      return SK_SUCCESS;
    case SK_PROC_STREAM_GET_HANDLE:
      return skGetStreamHandle_ALSA(SK_PARAMS_STREAM_GET_HANDLE);
    default:
      return SK_ERROR_NOT_IMPLEMENTED;
  }
}
