/*******************************************************************************
 * Copyright 2016 Trent Reed
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------------------
 * The standard implementation for OpenSK's ALSA driver.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_driver.h>
#include <OpenSK/ext/sk_stream.h>
#include <OpenSK/plt/platform.h>

// C99
#include <ctype.h>
#include <inttypes.h>
#include <string.h>

// ALSA
#include <alsa/asoundlib.h>

// Non-Standard
#include <dlfcn.h>

// Internal
#include <OpenSK/icd/alsa.h>
#include <OpenSK/dev/md5.h>
#include "convert.h"
#include "udev.h"

////////////////////////////////////////////////////////////////////////////////
// Driver Definitions
////////////////////////////////////////////////////////////////////////////////

#define SK_DRIVER_OPENSK_ALSA_ID "alsa"
#define SK_DRIVER_OPENSK_ALSA_DISPLAY_NAME "OpenSK (ALSA)"
#define SK_DRIVER_OPENSK_ALSA_DESCRIPTION "Advanced Linux Sound Architecture (ALSA)"
#define SK_DRIVER_OPENSK_ALSA_UUID_STRING "bf795994-7360-4531-a0a2-65c4be3a1098"
#define SK_DRIVER_OPENSK_ALSA_UUID SK_INTERNAL_CREATE_UUID(SK_DRIVER_OPENSK_ALSA_UUID_STRING)

typedef enum SkPcmStreamIndexIMPL {
  SK_PCM_STREAM_WRITE_INDEX_IMPL = 0,
  SK_PCM_STREAM_READ_INDEX_IMPL = 1,
  SK_PCM_STREAM_INDEX_SIZE = 2
} SkPcmStreamIndexIMPL;

typedef struct SkPcmStreamUserDataIMPL {
  snd_pcm_t*                            pcmHandle;
  snd_pcm_hw_params_t*                  hwParams;
  snd_pcm_sw_params_t*                  swParams;
  SkPcmStreamInfo*                      pStreamInfo;
  SkAlsaPcmStreamInfo*                  pIcdStreamInfo;
} SkPcmStreamUserDataIMPL;

typedef struct SkPcmStreamDataIMPL {
  int                                   lastError;
  SkBool32                              isClosed;
  snd_pcm_t*                            pcmHandle;
  SkPcmStreamInfo                       streamInfo;
  SkAlsaPcmStreamInfo                   icdStreamInfo;
  SkBool32                              supportsPausing;
  SkChannel*                            pChannelMap;
} SkPcmStreamDataIMPL;

typedef struct SkPcmStream_T {
  SK_INTERNAL_OBJECT_BASE;
  SkPcmStreamDataIMPL                   data[SK_PCM_STREAM_INDEX_SIZE];
} SkPcmStream_T;

typedef struct SkEndpoint_T {
  SK_INTERNAL_OBJECT_BASE;
  SkStreamFlags                         supportedStreams;
  SkBool32                              endpointActive;
  uint32_t                              endpointNumber;
  SkObject*                             streams;
  uint32_t                              streamCount;
  uint32_t                              streamCapacity;
  char*                                 endpointIdentifier;
  char                                  endpointPath[1];
} SkEndpoint_T;

typedef struct SkDevice_T {
  SK_INTERNAL_OBJECT_BASE;
  SkBool32                              deviceActive;
  uint32_t                              deviceNumber;
  SkEndpoint*                           endpoints;
  uint32_t                              endpointCount;
  uint32_t                              endpointCapacity;
  char*                                 deviceIdentifier;
  char                                  devicePath[1];
} SkDevice_T;

typedef struct SkDriver_T {
  SK_INTERNAL_OBJECT_BASE;
  SkAllocationCallbacks const*          pAllocator;
  SkDevice*                             devices;
  uint32_t                              deviceCount;
  uint32_t                              deviceCapacity;
  SkDevice_T                            dummyDevice;
  SkUDevIMPL                            udev;
  char                                  driverPath[sizeof(SK_DRIVER_OPENSK_ALSA_ID) + 1];
} SkDriver_T;

static SkResult SKAPI_CALL skQueryEndpointFeatures_alsa(
  SkEndpoint                            endpoint,
  SkEndpointFeatures*                   pFeatures
);

static SkResult SKAPI_CALL skEnumerateDeviceEndpoints_alsa(
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
);

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkDriver skGetDriverIMPL(
  SkObject                              object
) {
  while (object && ((SkInternalObjectBase*)object)->_oType != SK_OBJECT_TYPE_DRIVER) {
    object = ((SkInternalObjectBase*)object)->_pParent;
  }
  return object;
}

static SkDevice skGetCardDeviceIMPL(
  SkObject                              object
) {
  while (object && ((SkDevice)object)->_oType != SK_OBJECT_TYPE_DEVICE) {
    object = ((SkInternalObjectBase*)object)->_pParent;
  }
  return object;
}

static SkDevice skGetDeviceByNumberIMPL(
  SkDriver                              driver,
  uint32_t                              number
) {
  uint32_t idx;
  for (idx = 0; idx < driver->deviceCount; ++idx) {
    if (driver->devices[idx]->deviceNumber == number) {
      return driver->devices[idx];
    }
  }
  return NULL;
}

static SkDevice skGetDeviceByUuidIMPL(
  SkDriver                              driver,
  uint8_t                               deviceUuid[SK_UUID_SIZE]
) {
  uint32_t idx;
  for (idx = 0; idx < driver->deviceCount; ++idx) {
    if (memcmp(driver->devices[idx]->_iUuid, deviceUuid, SK_UUID_SIZE) == 0) {
      return driver->devices[idx];
    }
  }
  return NULL;
}

static SkEndpoint skGetEndpointByNameIMPL(
  SkObject                              parent,
  char const*                           pName
) {
  uint32_t idx;
  SkDevice device;

  // Handle the case where the parent is the driver
  switch (skGetObjectType(parent)) {
    case SK_OBJECT_TYPE_DRIVER:
      device = &((SkDriver)parent)->dummyDevice;
      break;
    case SK_OBJECT_TYPE_DEVICE:
      device = parent;
      break;
    default:
      return SK_NULL_HANDLE;
  }

  // Search for the endpoint by name
  for (idx = 0; idx < device->endpointCount; ++idx) {
    if (strcmp(device->endpoints[idx]->endpointIdentifier, pName) == 0) {
      return device->endpoints[idx];
    }
  }
  return NULL;
}

static SkDevice skAllocateDeviceIMPL(
  SkDriver                              driver,
  SkUDeviceInfoIMPL*                    pHardwareInfo,
  uint32_t                              number
) {
  SkResult result;
  SkDevice  device;
  SkDevice* pDevices;
  int identifierLength;
  size_t driverPrefixLength;
  SkDeviceCreateInfo createInfo;
  char deviceIdentifier[SND_MAX_IDENTIFIER_SIZE];

  // If needed, grow the device array
  if (driver->deviceCapacity == driver->deviceCount) {

    // Scale the capacity by x2 - handle the base-case.
    if (driver->deviceCapacity == 0) {
      driver->deviceCapacity = 1;
    }
    driver->deviceCapacity *= 2;

    // Reallocate the device array
    pDevices = skReallocate(
      driver->pAllocator,
      driver->devices,
      sizeof(SkDevice) * driver->deviceCapacity,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
    );
    if (!pDevices) {
      return NULL;
    }

    // Copy-over the old subdevice array into the new one.
    memcpy(pDevices, driver->devices, sizeof(SkDevice) * driver->deviceCount);
    driver->devices = pDevices;
  }

  // Generate the device identifier for storage
  identifierLength = sprintf(deviceIdentifier, "hw:%u", number);
  if (identifierLength <= 0) {
    return NULL;
  }

  // Construct the device implementation object.
  // Note: We also allocate enough information to attach the device identifier.
  driverPrefixLength = strlen(driver->driverPath);
  device = skAllocate(
    driver->pAllocator,
    sizeof(SkDevice_T) + identifierLength + driverPrefixLength + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!device) {
    return NULL;
  }
  strcpy(device->devicePath, driver->driverPath);
  device->devicePath[driverPrefixLength] = '/';
  device->deviceIdentifier = &device->devicePath[driverPrefixLength + 1];
  strcpy(device->deviceIdentifier, deviceIdentifier);

  // Construct the base object for the device.
  createInfo.sType = SK_STRUCTURE_TYPE_INTERNAL;
  createInfo.pNext = NULL;
  createInfo.deviceParent = driver;
  if (pHardwareInfo) {
    memcpy(createInfo.deviceUuid, pHardwareInfo->deviceUuid, SK_UUID_SIZE);
  }
  else {
    memset(createInfo.deviceUuid, 0, SK_UUID_SIZE);
  }
  result = skInitializeDeviceBase(
    &createInfo,
    driver->pAllocator,
    device
  );
  if (result != SK_SUCCESS) {
    skFree(driver->pAllocator, device);
    return NULL;
  }

  // Instantiate the device object
  device->endpoints = NULL;
  device->endpointCount = 0;
  device->endpointCapacity = 0;
  device->deviceActive = SK_FALSE;
  device->deviceNumber = number;

  // "Claim" the subdevice by incrementing the count
  driver->devices[driver->deviceCount] = device;
  ++driver->deviceCount;

  return device;
}

static SkEndpoint skAllocateEndpointIMPL(
  SkObject                              parent,
  char const*                           pName
) {
  MD5_CTX md5;
  SkResult result;
  SkDriver driver;
  SkDevice device;
  SkEndpoint  endpoint;
  SkEndpoint* pEndpoints;
  size_t endpointPathLength;
  size_t endpointPrefixLength;
  SkEndpointCreateInfo createInfo;

  // Get the driver api (we should always be able to find this)
  switch (skGetObjectType(parent)) {
    case SK_OBJECT_TYPE_DRIVER:
      driver = parent;
      device = &driver->dummyDevice;
      break;
    case SK_OBJECT_TYPE_DEVICE:
      driver = skGetDriverIMPL(parent);
      device = parent;
      break;
    default:
      return SK_NULL_HANDLE;
  }
  if (!driver || !device) {
    return NULL;
  }

  // If needed, grow the endpoint array
  if (device->endpointCapacity == device->endpointCount) {

    // Scale the capacity by x2 - handle the base-case.
    if (device->endpointCapacity == 0) {
      device->endpointCapacity = 1;
    }
    device->endpointCapacity *= 2;

    // Reallocate the endpoint array
    // Note: We manually reallocate instead of using skReallocate so that if the
    //       allocation fails, we are not stuck with a null endpoint pointer that
    //       we have to build up again.
    pEndpoints = skAllocate(
      driver->pAllocator,
      sizeof(SkEndpoint) * device->endpointCapacity,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
    );
    if (!pEndpoints) {
      return NULL;
    }

    // Copy-over the old endpoint array into the new one.
    memcpy(pEndpoints, device->endpoints, sizeof(SkEndpoint) * device->endpointCount);
    skFree(driver->pAllocator, device->endpoints);
    device->endpoints = pEndpoints;
  }

  // Construct the endpoint implementation object.
  endpointPrefixLength = strlen(device->devicePath);
  endpointPathLength = endpointPrefixLength + strlen(pName) + 1;
  endpoint = skAllocate(
    driver->pAllocator,
    sizeof(SkEndpoint_T) + endpointPathLength,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!endpoint) {
    return NULL;
  }
  strcpy(endpoint->endpointPath, device->devicePath);
  endpoint->endpointPath[endpointPrefixLength] = '/';
  endpoint->endpointIdentifier = &endpoint->endpointPath[endpointPrefixLength + 1];
  strcpy(endpoint->endpointIdentifier, pName);

  // Generate the MD5 hash for the endpoint path.
  MD5_Init(&md5);
  MD5_Update(&md5, endpoint->endpointPath, endpointPathLength);
  MD5_Final(createInfo.endpointUuid, &md5);

  // Construct the base object for the endpoint.
  // Note: Because these are never de-allocated, and they belong under
  //       a given driver/device. The requirement that the endpoint is unique
  //       and unchanging between devices being plugged/unplugged from the
  //       same seat is met. Perhaps in the future we should repurpose devices?
  createInfo.sType = SK_STRUCTURE_TYPE_INTERNAL;
  createInfo.pNext = NULL;
  createInfo.endpointParent = parent;
  result = skInitializeEndpointBase(
    &createInfo,
    driver->pAllocator,
    endpoint
  );
  if (result != SK_SUCCESS) {
    skFree(driver->pAllocator, endpoint);
    return NULL;
  }

  // Instantiate the endpoint object
  endpoint->endpointActive = SK_FALSE;
  endpoint->streamCapacity = 0;
  endpoint->streamCount = 0;
  endpoint->streams = NULL;

  // "Claim" the endpoint by incrementing the count
  device->endpoints[device->endpointCount] = endpoint;
  ++device->endpointCount;

  return endpoint;
}

static void skCloseStreamIMPL(
  SkObject                              stream
) {
  switch (skGetObjectType(stream)) {
    case SK_OBJECT_TYPE_PCM_STREAM:
      skClosePcmStream(stream, SK_FALSE);
      break;
    default:
      break;
  }
}

static void skDestroyEndpointIMPL(
  SkEndpoint                            endpoint,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  for (idx = 0; idx < endpoint->streamCount; ++idx) {
    skCloseStreamIMPL(endpoint->streams[idx]);
  }
  skDeinitializeEndpointBase(endpoint, pAllocator);
  skFree(pAllocator, endpoint->streams);
  skFree(pAllocator, endpoint);
}

static void skDestroyDeviceIMPL(
  SkDevice                              device,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  for (idx = 0; idx < device->endpointCount; ++idx) {
    skDestroyEndpointIMPL(device->endpoints[idx], pAllocator);
  }
  skDeinitializeDeviceBase(device, pAllocator);
  skFree(pAllocator, device->endpoints);
  skFree(pAllocator, device);
}

static SkResult skHandleCtlErrorIMPL(
  int                                   error
) {
  switch (error) {
    case -ENOTTY:
    case -ENODEV:
      // The device was lost, destroy stream and get a new endpoint.
      return SK_ERROR_DEVICE_LOST;
    case -EBADFD:
    default:
      // Some unexpected catastrophic failure has occurred. Panic!
      return SK_ERROR_SYSTEM_INTERNAL;
  }
}

static SkResult skQueryControlEndpointFeaturesOrPropertiesIMPL(
  snd_ctl_t*                            ctlHandle,
  SkEndpoint                            endpoint,
  SkEndpointFeatures*                   pFeatures,
  SkEndpointProperties*                 pProperties
) {
  int err;
  char const* pConstChar;
  snd_pcm_info_t* pcmInfo;
  snd_pcm_stream_t pcmType;
  snd_rawmidi_info_t* midiInfo;
  snd_rawmidi_stream_t midiType;

  if (pFeatures) {
    pFeatures->supportedStreams = 0;
  }

  // Get information about the name of the device (PCM)
  snd_pcm_info_alloca(&pcmInfo);
  snd_pcm_info_set_device(pcmInfo, endpoint->endpointNumber);
  snd_pcm_info_set_subdevice(pcmInfo, 0);
  for (pcmType = (snd_pcm_stream_t)0; pcmType <= SND_PCM_STREAM_LAST; ++pcmType) {
    snd_pcm_info_set_stream(pcmInfo, pcmType);
    err = snd_ctl_pcm_info(ctlHandle, pcmInfo);
    switch (err) {
      case 0:
      case -EBUSY: // Stream exists but is busy
        if (pProperties) {
          pConstChar = snd_pcm_info_get_name(pcmInfo);
          if (pConstChar) {
            strncpy(pProperties->endpointName, pConstChar, SK_MAX_NAME_SIZE);
          }
        }
        if (pFeatures) {
          pFeatures->supportedStreams |= skConvertFromLocalPcmStreamTypeIMPL(pcmType);
        }
        break;
      case -ENOENT:// Stream type isn't supported
      case -ENXIO: // This particular set of devices isn't supported.
        break;
      default:
        return skHandleCtlErrorIMPL(err);
    }
  }

  // Get information about the name of the device (MIDI)
  snd_rawmidi_info_alloca(&midiInfo);
  snd_rawmidi_info_set_device(midiInfo, endpoint->endpointNumber);
  snd_rawmidi_info_set_subdevice(midiInfo, 0);
  for (midiType = (snd_rawmidi_stream_t)0; midiType <= SND_RAWMIDI_STREAM_LAST; ++midiType) {
    snd_rawmidi_info_set_stream(midiInfo, midiType);
    err = snd_ctl_rawmidi_info(ctlHandle, midiInfo);
    switch (err) {
      case 0:
      case -EBUSY: // Stream exists but is busy
        if (pProperties) {
          pConstChar = snd_rawmidi_info_get_name(midiInfo);
          if (pConstChar) {
            strncpy(pProperties->endpointName, pConstChar, SK_MAX_NAME_SIZE);
          }
        }
        if (pFeatures) {
          pFeatures->supportedStreams |= skConvertFromLocalMidiStreamTypeIMPL(midiType);
        }
        break;
      case -ENOENT:// Stream type isn't supported
        break;
      case -ENXIO: // This particular set of devices isn't supported.
        break;
      default:
        return skHandleCtlErrorIMPL(err);
    }
  }

  return SK_SUCCESS;
}

static SkResult skQueryDeviceFeaturesOrPropertiesIMPL(
  snd_ctl_t*                            ctlHandle,
  SkDevice                              device,
  SkDeviceFeatures*                     pFeatures,
  SkDeviceProperties*                   pProperties
) {
  int err;
  uint32_t idx;
  SkResult result;
  snd_ctl_card_info_t* pCardInformation;
  SkEndpointFeatures endpointFeatures;

  // Capture the metadata for the device
  if (pProperties) {
    snd_ctl_card_info_alloca(&pCardInformation);
    err = snd_ctl_card_info(ctlHandle, pCardInformation);
    if (err < 0) {
      return skHandleCtlErrorIMPL(err);
    }

    // Copy information from the card info into device properties
    strncpy(pProperties->deviceName, snd_ctl_card_info_get_name(pCardInformation), SK_MAX_NAME_SIZE);
    strncpy(pProperties->driverName, snd_ctl_card_info_get_driver(pCardInformation), SK_MAX_NAME_SIZE);
    strncpy(pProperties->mixerName, snd_ctl_card_info_get_mixername(pCardInformation), SK_MAX_NAME_SIZE);
    pProperties->deviceType = SK_DEVICE_TYPE_OTHER;

    // To handle the case for large names/identifiers - null the ends
    pProperties->deviceName[SK_MAX_NAME_SIZE - 1] = 0;
    pProperties->driverName[SK_MAX_NAME_SIZE - 1] = 0;
    pProperties->mixerName[SK_MAX_NAME_SIZE - 1] = 0;
  }

  if (pFeatures) {
    pFeatures->supportedStreams = 0;
    result = skEnumerateDeviceEndpoints_alsa(device, &idx, NULL);
    if (result != SK_SUCCESS) {
      return result;
    }
    for (idx = 0; idx < device->endpointCount; ++idx) {
      if (!device->endpoints[idx]->endpointActive) {
        continue;
      }
      result = skQueryControlEndpointFeaturesOrPropertiesIMPL(ctlHandle, device->endpoints[idx], &endpointFeatures, NULL);
      if (result == SK_SUCCESS) {
        pFeatures->supportedStreams |= endpointFeatures.supportedStreams;
      }
    }
  }

  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Required Implementation (Create/Destroy/Checksum Functions)
////////////////////////////////////////////////////////////////////////////////

static void skErrorHandler_alsa(char const* file, int line, char const* function, int err, char const* fmt, ...) {
  // Do nothing!
  (void)file;
  (void)line;
  (void)function;
  (void)err;
  (void)fmt;
}

static SkResult SKAPI_CALL skCreateDriver_alsa(
  SkDriverCreateInfo const*            pCreateInfo,
  SkAllocationCallbacks const*         pAllocator,
  SkDriver*                            pDriver
) {
  SkDriver driver;
  SkResult result;
  snd_lib_error_set_handler(&skErrorHandler_alsa);

  // Allocate the driver instance
  driver = skClearAllocate(
    pAllocator,
    sizeof(SkDriver_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!driver) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the different constructs for acquiring device info.
  result = skCreateUDevIMPL(pAllocator, &driver->udev);
  if (result != SK_SUCCESS) {
    skFree(pAllocator, driver);
    return result;
  }

  // Initialize any driver base information, this must be done before
  // any other changes are made to the object - it creates the vtable.
  result = skInitializeDriverBase(
    pCreateInfo,
    pAllocator,
    driver
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, driver);
    return result;
  }

  // Initialize the driver
  // Note: We don't care if we fail to allocate udev.
  driver->pAllocator = pAllocator;
  strcpy(driver->driverPath, SK_DRIVER_OPENSK_ALSA_ID);

  *pDriver = driver;
  return SK_SUCCESS;
}

static void SKAPI_CALL skDestroyDriver_alsa(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver
) {
  uint32_t idx;

  // Note: Because the root device is not dynamically-allocated,
  //       we must iterate over subdevices/endpoints manually here.
  for (idx = 0; idx < driver->deviceCount; ++idx) {
    skDestroyDeviceIMPL(driver->devices[idx], pAllocator);
  }
  skFree(pAllocator, driver->devices);

  for (idx = 0; idx < driver->dummyDevice.endpointCount; ++idx) {
    skDestroyEndpointIMPL(driver->dummyDevice.endpoints[idx], pAllocator);
  }
  skFree(pAllocator, driver->dummyDevice.endpoints);
  skDestroyUDevIMPL(driver->udev, pAllocator);

  // The deinitialize must happen in this order at the very end.
  skDeinitializeDriverBase(driver, pAllocator);
  skFree(pAllocator, driver);
}

static void SKAPI_CALL skGetDriverFeatures_alsa(
  SkDriver                             driver,
  SkDriverFeatures*                    pFeatures
) {
  (void)driver;
  pFeatures->defaultEndpoint = SK_TRUE;
  pFeatures->supportedAccessModes =
    SK_ACCESS_BLOCKING_BIT |
    SK_ACCESS_INTERLEAVED_BIT |
    SK_ACCESS_MEMORY_MAPPED_BIT;
  pFeatures->supportedStreams =
    SK_STREAM_PCM_READ_BIT | SK_STREAM_PCM_WRITE_BIT |
    SK_STREAM_MIDI_READ_BIT | SK_STREAM_MIDI_WRITE_BIT;
}

static SkUDeviceInfoIMPL* skGetUDeviceByCardInfoFromArrayIMPL(
  SkUDeviceInfoIMPL*                    pDeviceInfoArray,
  uint32_t                              deviceInfoCount,
  uint32_t                              cardNumber
) {
  uint32_t idx;
  for (idx = 0; idx < deviceInfoCount; ++idx) {
    if (pDeviceInfoArray[idx].systemId == cardNumber) {
      return &pDeviceInfoArray[idx];
    }
  }
  return NULL;
}

static SkResult SKAPI_CALL skEnumerateDriverDevices_alsa(
  SkDriver                              driver,
  uint32_t*                             pDeviceCount,
  SkDevice*                             pDevices
) {
  int card;
  uint32_t count;
  SkResult result;
  SkDevice device;
  uint32_t hardwareDevices;
  SkUDeviceInfoIMPL* pDeviceInfo;
  SkUDeviceInfoIMPL* pDeviceInfoArray;

  // Set all devices to inactive (in case anything was disconnected)
  for (count = 0; count < driver->deviceCount; ++count) {
    driver->devices[count]->deviceActive = SK_FALSE;
  }

  // Count and pre-allocate hardware device information
  result = skEnumerateUDevDevicesIMPL(driver->udev, &hardwareDevices, NULL);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Allocate a temporary structure for the hardware device information
  // Note: This is what we will look through for matching an ALSA device to
  //       initialize the device UUID. If found, this will be the primary key
  //       for the device. If not found, one will be generated and we will set
  //       a flag that the formal device UUID has not been found.
  if (hardwareDevices) {
    pDeviceInfoArray = skAllocate(
      driver->pAllocator,
      sizeof(SkUDeviceInfoIMPL) * hardwareDevices,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
    );
    if (!pDeviceInfoArray) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    result = skEnumerateUDevDevicesIMPL(driver->udev, &hardwareDevices, pDeviceInfoArray);
    if (result < SK_SUCCESS) {
      skFree(driver->pAllocator, pDeviceInfoArray);
      return result;
    }
  }

  count = 0;
  card = -1;
  for(;;) {

    // Attempt to open the next card
    if (snd_card_next(&card) != 0) {
      skFree(driver->pAllocator, pDeviceInfoArray);
      return SK_ERROR_SYSTEM_INTERNAL;
    }

    // When card == -1, we are done reading.
    if (card == -1) break;

    // Find the hardware device information (by UUID or number)
    // Note: If the UUID matches, the card number should also match.
    //       Though this should always be true, the card number will be used
    //       as a fallback for if this ever fails to be the case. :(
    pDeviceInfo = skGetUDeviceByCardInfoFromArrayIMPL(pDeviceInfoArray, hardwareDevices, (uint32_t)card);
    if (pDeviceInfo) {
      device = skGetDeviceByUuidIMPL(driver, pDeviceInfo->deviceUuid);
      if (device && device->deviceNumber != (uint32_t)card) {
        pDeviceInfo = NULL;
        device = skGetDeviceByNumberIMPL(driver, (uint32_t)card);
      }
    }
    else {
      device = skGetDeviceByNumberIMPL(driver, (uint32_t)card);
    }

    // If the device doesn't exist, it must have never been added to the list.
    // We should construct it and add the device to the list of known devices.
    if (!device) {
      device = skAllocateDeviceIMPL(driver, pDeviceInfo, (uint32_t)card);
      if (!device) {
        skFree(driver->pAllocator, pDeviceInfoArray);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    device->deviceActive = SK_TRUE;

    // Populate the enumeration.
    // Note: This is the only time we can query and build SkDeviceProperties.
    if (pDevices) {
      if (*pDeviceCount <= count) {
        skFree(driver->pAllocator, pDeviceInfoArray);
        return SK_INCOMPLETE;
      }
      pDevices[count] = device;
    }
    ++count;
  }

  *pDeviceCount = count;
  skFree(driver->pAllocator, pDeviceInfoArray);
  return SK_SUCCESS;
}

static SkResult skGetUDeviceInfo(
  SkDevice                              device,
  SkUDeviceInfoIMPL*                    pDeviceInfo
) {
  uint32_t idx;
  SkResult result;
  SkDriver driver;
  uint32_t deviceCount;
  SkUDeviceInfoIMPL* pUDeviceInfo;

  driver = skGetDriverIMPL(device);
  if (!driver) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  result = skEnumerateUDevDevicesIMPL(driver->udev, &deviceCount, NULL);
  if (result != SK_SUCCESS) {
    return result;
  }

  pUDeviceInfo = skAllocate(
    driver->pAllocator,
    sizeof(SkUDeviceInfoIMPL) * deviceCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pUDeviceInfo) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  result = skEnumerateUDevDevicesIMPL(driver->udev, &deviceCount, pUDeviceInfo);
  if (result < SK_SUCCESS) {
    skFree(driver->pAllocator, pUDeviceInfo);
    return result;
  }

  for (idx = 0; idx < deviceCount; ++idx) {
    if (pUDeviceInfo[idx].systemId == device->deviceNumber) {
      memcpy(pDeviceInfo, &pUDeviceInfo[idx], sizeof(SkUDeviceInfoIMPL));
      skFree(driver->pAllocator, pUDeviceInfo);
      return SK_SUCCESS;
    }
  }

  skFree(driver->pAllocator, pUDeviceInfo);
  return SK_ERROR_INITIALIZATION_FAILED;
}

static SkResult SKAPI_CALL skQueryDeviceFeatures_alsa(
  SkDevice                              device,
  SkDeviceFeatures*                     pFeatures
) {
  int err;
  SkResult result;
  snd_ctl_t* ctlHandle;

  // Open a handle to the device.
  err = snd_ctl_open(&ctlHandle, device->deviceIdentifier, SND_CTL_READONLY);
  if (err < 0) {
    return skHandleCtlErrorIMPL(err);
  }

  // Get information about the device
  result = skQueryDeviceFeaturesOrPropertiesIMPL(
    ctlHandle,
    device,
    pFeatures,
    NULL
  );

  snd_ctl_close(ctlHandle);
  return result;
}

static SkResult SKAPI_CALL skQueryDeviceProperties_alsa(
  SkDevice                              device,
  SkDeviceProperties*                   pProperties
) {
  int err;
  SkResult result;
  snd_ctl_t* ctlHandle;
  SkUDeviceInfoIMPL deviceInfo;

  // Open a handle to the device.
  err = snd_ctl_open(&ctlHandle, device->deviceIdentifier, SND_CTL_READONLY);
  if (err < 0) {
    return skHandleCtlErrorIMPL(err);
  }

  // Get information about the device.
  result = skQueryDeviceFeaturesOrPropertiesIMPL(
    ctlHandle,
    device,
    NULL,
    pProperties
  );
  if (result != SK_SUCCESS) {
    snd_ctl_close(ctlHandle);
    return result;
  }

  // Get the system device information (different from high-level ALSA info.)
  result = skGetUDeviceInfo(device, &deviceInfo);
  if (result != SK_SUCCESS) {
    snd_ctl_close(ctlHandle);
    return result;
  }

  // Populate the high-level device information.
  pProperties->vendorID = deviceInfo.vendorId;
  pProperties->deviceID = deviceInfo.modelId;
  pProperties->deviceType = deviceInfo.deviceType;
  memcpy(pProperties->deviceUuid, deviceInfo.deviceUuid, SK_UUID_SIZE);

  snd_ctl_close(ctlHandle);
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skEnumerateDriverEndpoints_alsa(
  SkDriver                             driver,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  void** hint;
  void** hints;
  uint32_t count;
  char const* pName;
  char const* pIoid;
  SkEndpoint endpoint;

  // Set all of the endpoints to inactive
  for (count = 0; count < driver->dummyDevice.endpointCount; ++count) {
    driver->dummyDevice.endpoints[count]->supportedStreams = 0;
    driver->dummyDevice.endpoints[count]->endpointActive = SK_FALSE;
  }
  count = 0;

  // Find all of the PCM endpoints
  {
    if (snd_device_name_hint(-1, "pcm", &hints) != 0) {
      return SK_ERROR_SYSTEM_INTERNAL;
    }
    hint = hints;

    while (*hint) {
      pName = snd_device_name_get_hint(*hint, "NAME");
      if (!pName) {
        ++hint;
        continue;
      }

      // Get the proper endpoint, if it exists.
      endpoint = skGetEndpointByNameIMPL(driver, pName);
      if (!endpoint) {
        endpoint = skAllocateEndpointIMPL(driver, pName);
        if (!endpoint) {
          return SK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }

      // Find stream support type
      pIoid = snd_device_name_get_hint(*hint, "IOID");
      if (!pIoid || strcmp(pIoid, "Output") == 0) endpoint->supportedStreams |= SK_STREAM_PCM_WRITE_BIT;
      if (!pIoid || strcmp(pIoid, "Input") == 0) endpoint->supportedStreams |= SK_STREAM_PCM_READ_BIT;

      // Populate the enumeration
      if (pEndpoints) {
        if (*pEndpointCount <= count) {
          return SK_INCOMPLETE;
        }
        pEndpoints[count] = endpoint;
      }
      ++count;
      ++hint;
    }
  }

  // Find all of the MIDI endpoints
  {
    if (snd_device_name_hint(-1, "rawmidi", &hints) != 0) {
      return SK_ERROR_SYSTEM_INTERNAL;
    }
    hint = hints;

    while (*hint) {
      pName = snd_device_name_get_hint(*hint, "NAME");
      if (!pName) {
        ++hint;
        continue;
      }

      // Get the proper endpoint, if it exists.
      endpoint = skGetEndpointByNameIMPL(driver, pName);
      if (!endpoint) {
        endpoint = skAllocateEndpointIMPL(driver, pName);
        if (!endpoint) {
          return SK_ERROR_OUT_OF_HOST_MEMORY;
        }
      }
      endpoint->endpointActive = SK_TRUE;

      // Find stream support type
      pIoid = snd_device_name_get_hint(*hint, "IOID");
      if (!pIoid || strcmp(pIoid, "Output") == 0) endpoint->supportedStreams |= SK_STREAM_MIDI_WRITE_BIT;
      if (!pIoid || strcmp(pIoid, "Input") == 0) endpoint->supportedStreams |= SK_STREAM_MIDI_READ_BIT;

      // Populate the enumeration.
      if (pEndpoints) {
        if (*pEndpointCount <= count) {
          return SK_INCOMPLETE;
        }
        pEndpoints[count] = endpoint;
      }
      ++count;
      ++hint;
    }
  }

  *pEndpointCount = count;
  return SK_SUCCESS;
}

static SkResult skEnumerateDeviceEndpointsIMPL_endpoint(
  snd_ctl_t*                            ctlHandle,
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  int err;
  int curr;
  uint32_t count;
  SkEndpoint endpoint;
  char endpointIdentifier[SND_MAX_IDENTIFIER_SIZE];

  // Enumerate the endpoints
  curr = -1;
  count = 0;
  for (;;) {

    // Attempt to get the next endpoint.
    err = snd_ctl_pcm_next_device(ctlHandle, &curr);
    if (err < 0) {
      snd_ctl_close(ctlHandle);
      return skHandleCtlErrorIMPL(err);
    }

    // When curr == -1, we are done reading.
    if (curr == -1) break;

    // Format the endpoint identifier
    if (sprintf(endpointIdentifier, "%s,%u", device->deviceIdentifier, curr) <= 0) {
      continue;
    }

    // Get the proper endpoint, if it exists.
    endpoint = skGetEndpointByNameIMPL(device, endpointIdentifier);
    if (!endpoint) {
      endpoint = skAllocateEndpointIMPL(device, endpointIdentifier);
      if (!endpoint) {
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }
    }
    endpoint->endpointNumber = (uint32_t)curr;
    endpoint->endpointActive = SK_TRUE;

    // Populate the enumeration.
    if (pEndpoints) {
      if (*pEndpointCount <= count) {
        return SK_INCOMPLETE;
      }
      pEndpoints[count] = endpoint;
    }
    ++count;
  }

  *pEndpointCount = count;
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skEnumerateDeviceEndpoints_alsa(
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  int err;
  SkResult result;
  snd_ctl_t* ctlHandle;

  // Open a handle to the device.
  err = snd_ctl_open(&ctlHandle, device->deviceIdentifier, SND_CTL_READONLY);
  if (err < 0) {
    return skHandleCtlErrorIMPL(err);
  }

  // Call the proper method for enumerating endpoints.
  result = skEnumerateDeviceEndpointsIMPL_endpoint(
    ctlHandle,
    device,
    pEndpointCount,
    pEndpoints
  );

  snd_ctl_close(ctlHandle);
  return result;
}

static SkResult SKAPI_CALL skQueryEndpointFeatures_alsa(
  SkEndpoint                            endpoint,
  SkEndpointFeatures*                   pFeatures
) {
  int err;
  SkResult result;
  SkDevice device;
  snd_ctl_t* ctlHandle;
  snd_pcm_t* pcmHandle;
  snd_pcm_stream_t pcmType;

  memset(pFeatures, 0, sizeof(SkEndpointFeatures));

  // Get the card device (if no device, it's a driver endpoint)
  device = skGetCardDeviceIMPL(endpoint);
  if (device) {
    // Open a handle to the device.
    err = snd_ctl_open(&ctlHandle, device->deviceIdentifier, SND_CTL_READONLY);
    if (err < 0) {
      return skHandleCtlErrorIMPL(err);
    }

    // Call the proper method for enumerating endpoints.
    result = skQueryControlEndpointFeaturesOrPropertiesIMPL(
      ctlHandle,
      endpoint,
      pFeatures,
      NULL
    );

    snd_ctl_close(ctlHandle);
    return result;
  }

  pFeatures->supportedStreams = 0;
  for (pcmType = (snd_pcm_stream_t)0; pcmType <= SND_PCM_STREAM_LAST; ++pcmType) {
    err = snd_pcm_open(&pcmHandle, endpoint->endpointIdentifier, pcmType, 0);
    if (err < 0) {
      switch (err) {
        case -EBUSY:
          // The stream is supported, but it is in-use.
          pFeatures->supportedStreams |= skConvertFromLocalPcmStreamTypeIMPL(pcmType);
          continue;
        case -ENOENT:
          // The stream type is not supported, continue.
          continue;
        case -EINVAL:
          // The stream is invalid, and cannot be used.
          return SK_ERROR_INVALID;
        default:
          // Unknown error code!
          return SK_ERROR_SYSTEM_INTERNAL;
      }
    }
    pFeatures->supportedStreams |= skConvertFromLocalPcmStreamTypeIMPL(pcmType);
    snd_pcm_close(pcmHandle);
  }

  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skQueryEndpointProperties_alsa(
  SkEndpoint                            endpoint,
  SkEndpointProperties*                 pProperties
) {
  int err;
  SkResult result;
  SkDevice device;
  snd_ctl_t* ctlHandle;
  snd_pcm_info_t* pcmInfo;
  snd_pcm_stream_t pcmType;

  memset(pProperties, 0, sizeof(SkEndpointProperties));
  memcpy(pProperties->endpointUuid, endpoint->_iUuid, SK_UUID_SIZE);

  // Get the card device (if no device, it's a driver endpoint)
  device = skGetCardDeviceIMPL(endpoint);
  if (!device) {
    strcpy(pProperties->displayName, endpoint->endpointIdentifier);
    strcpy(pProperties->endpointName, endpoint->endpointIdentifier);
    return SK_SUCCESS;
  }

  // If we aren't looking at a driver endpoint, we should have a CTL to query.
  err = snd_ctl_open(&ctlHandle, device->deviceIdentifier, 0);
  if (err != 0) {
      switch (err) {
        case -EBUSY:
          // The stream is supported, but it is in-use.
          return SK_ERROR_BUSY;
        case -EINVAL:
          // The stream is invalid, and cannot be used.
          return SK_ERROR_INVALID;
        default:
          // Unknown error code!
          return SK_ERROR_SYSTEM_INTERNAL;
      }
  }

  // Configure the PCM info request.
  snd_pcm_info_alloca(&pcmInfo);
  snd_pcm_info_set_device(pcmInfo, endpoint->endpointNumber);
  snd_pcm_info_set_subdevice(pcmInfo, 0);

  result = SK_SUCCESS;
  for (pcmType = (snd_pcm_stream_t)0; pcmType <= SND_PCM_STREAM_LAST; ++pcmType) {
    snd_pcm_info_set_stream(pcmInfo, pcmType);
    err = snd_ctl_pcm_info(ctlHandle, pcmInfo);
    if (err < 0) {
      switch (err) {
        case -ENOENT:
          // The stream type is not supported, continue.
          continue;
        default:
          snd_ctl_close(ctlHandle);
          return SK_ERROR_SYSTEM_INTERNAL;
      }
    }
    strncpy(pProperties->displayName, snd_pcm_info_get_name(pcmInfo), SK_MAX_NAME_SIZE);
    strncpy(pProperties->endpointName, endpoint->endpointIdentifier, SK_MAX_NAME_SIZE);
    break;
  }

  snd_ctl_close(ctlHandle);
  return result;
}

#define ALSA_CHECK(call) if ((err = call) < 0) { snd_pcm_close(pcmHandle); return SK_ERROR_NOT_SUPPORTED; }
#define EXPT_CHECK(call) if ((err = call) < 0) { snd_pcm_close(pcmHandle); return SK_ERROR_SYSTEM_INTERNAL; }
static SkResult SKAPI_CALL skRequestPcmStream_alsaIMPL(
  SkEndpoint                            endpoint,
  SkAlsaPcmStreamRequest*               pStreamRequest,
  SkPcmStream*                          pStream
) {
  int err;
  SkResult result;
  snd_pcm_t* pcmHandle;
  int minimumPeriodsDir;
  unsigned int minimumPeriods;
  snd_pcm_hw_params_t* hwParams;
  snd_pcm_sw_params_t* swParams;
  SkPcmStreamInfo streamInfo;
  SkAlsaPcmStreamInfo icdStreamInfo;
  SkPcmStreamCreateInfo createInfo;
  SkPcmStreamUserDataIMPL userData;

  //----------------------------------------------------------------------------
  // Check all of the request values to make sure they make sense.
  //----------------------------------------------------------------------------
  if (pStreamRequest->hw.periods == 1) {
    return SK_ERROR_INVALID;
  }

  //----------------------------------------------------------------------------
  // Open the PCM stream, will work if subdevices are available.
  //----------------------------------------------------------------------------
  err = snd_pcm_open(&pcmHandle, endpoint->endpointIdentifier, pStreamRequest->hw.streamType, pStreamRequest->hw.blockingMode);
  if (err < 0) {
    switch (err) {
      case -ENOENT:
        return SK_ERROR_DEVICE_LOST;
      case -EBUSY:
        return SK_ERROR_BUSY;
      default:
        return SK_ERROR_SYSTEM_INTERNAL;
    }
  }

  //----------------------------------------------------------------------------
  // Consume and configure the hardware parameters from the request values.
  // As-per standard, a "zero" value means "any". Drivers only need to handle
  // values that they understand, so any unhandled values here is fine.
  //----------------------------------------------------------------------------
  snd_pcm_hw_params_alloca(&hwParams);
  ALSA_CHECK(snd_pcm_hw_params_any(pcmHandle, hwParams));
  {
    // Format (Default = First)
    if (pStreamRequest->hw.formatType != SND_PCM_FORMAT_ANY) {
      ALSA_CHECK(snd_pcm_hw_params_set_format(pcmHandle, hwParams, pStreamRequest->hw.formatType));
    }

    // Subformat Type (Default = First)
    if (pStreamRequest->hw.subformatType != SND_PCM_SUBFORMAT_ANY) {
      ALSA_CHECK(snd_pcm_hw_params_set_subformat(pcmHandle, hwParams, pStreamRequest->hw.subformatType));
    }

    // Access Type (Default = First)
    if (pStreamRequest->hw.accessMode != SND_PCM_ACCESS_ANY) {
      ALSA_CHECK(snd_pcm_hw_params_set_access(pcmHandle, hwParams, pStreamRequest->hw.accessMode));
    }

    // Channels (Default = Minimum)
    if (pStreamRequest->hw.channels) {
      ALSA_CHECK(snd_pcm_hw_params_set_channels_near(pcmHandle, hwParams, &pStreamRequest->hw.channels));
    }

    // Sample Rate (Default = Minimum)
    if (pStreamRequest->hw.rate) {
      ALSA_CHECK(snd_pcm_hw_params_set_rate_near(pcmHandle, hwParams, &pStreamRequest->hw.rate, &pStreamRequest->hw.rateDir));
    }

    // Configure minimum period size
    // Note: Period size of 2 is the minimum sensible size, because a period
    //       represents the current subset of a buffer being processed. If there
    //       is only 1 period, then we would immediately XRUN because another
    //       period worth of samples is not available to play.
    minimumPeriods = 2;
    minimumPeriodsDir = 0;
    ALSA_CHECK(snd_pcm_hw_params_set_periods_min(pcmHandle, hwParams, &minimumPeriods, &minimumPeriodsDir));

    // Periods (Default = Maximum)
    if (pStreamRequest->hw.periods) {
      ALSA_CHECK(snd_pcm_hw_params_set_periods_near(pcmHandle, hwParams, &pStreamRequest->hw.periods, &pStreamRequest->hw.periodsDir));
    }

    // Force periods to be an integer value.
    ALSA_CHECK(snd_pcm_hw_params_set_periods_integer(pcmHandle, hwParams));

    // Period Time (Default = Maximum)
    if (pStreamRequest->hw.periodTime) {
      ALSA_CHECK(snd_pcm_hw_params_set_period_time_near(pcmHandle, hwParams, &pStreamRequest->hw.periodTime, &pStreamRequest->hw.periodTimeDir));
    }

    // Buffer Time (Default = Maximum)
    if (pStreamRequest->hw.bufferTime) {
      ALSA_CHECK(snd_pcm_hw_params_set_buffer_time_near(pcmHandle, hwParams, &pStreamRequest->hw.bufferTime, &pStreamRequest->hw.bufferTimeDir));
    }

    // Period Size (Default = Maximum)
    if (pStreamRequest->hw.periodSize) {
      ALSA_CHECK(snd_pcm_hw_params_set_period_size_near(pcmHandle, hwParams, &pStreamRequest->hw.periodSize, &pStreamRequest->hw.periodSizeDir));
    }

    // Buffer Size (Default = Maximum)
    if (pStreamRequest->hw.bufferSize) {
      ALSA_CHECK(snd_pcm_hw_params_set_buffer_size_near(pcmHandle, hwParams, &pStreamRequest->hw.bufferSize));
    }

    // Apply the hardware parameters and see if the configuration sticks.
    err = snd_pcm_hw_params(pcmHandle, hwParams);
    if (err < 0) {
      snd_pcm_close(pcmHandle);
      return SK_ERROR_NOT_SUPPORTED;
    }
  }

  //----------------------------------------------------------------------------
  // Consume and configure the software parameters from the request values.
  // As-per standard, a "zero" value means "any". Drivers only need to handle
  // values that they understand, so any unhandled values here is fine.
  //----------------------------------------------------------------------------
  snd_pcm_sw_params_alloca(&swParams);
  ALSA_CHECK(snd_pcm_sw_params_current(pcmHandle, swParams));
  {
    // Available Minimum
    if (pStreamRequest->sw.availMin) {
      ALSA_CHECK(snd_pcm_sw_params_set_avail_min(pcmHandle, swParams, pStreamRequest->sw.availMin));
    }

    // Silence Size
    if (pStreamRequest->sw.silenceSize) {
      ALSA_CHECK(snd_pcm_sw_params_set_silence_size(pcmHandle, swParams, pStreamRequest->sw.silenceSize));
    }

    // Silence Threshold
    if (pStreamRequest->sw.silenceThreshold) {
      ALSA_CHECK(snd_pcm_sw_params_set_silence_threshold(pcmHandle, swParams, pStreamRequest->sw.silenceThreshold));
    }

    // Start Threshold
    if (pStreamRequest->sw.startThreshold) {
      ALSA_CHECK(snd_pcm_sw_params_set_start_threshold(pcmHandle, swParams, pStreamRequest->sw.startThreshold));
    }

    // Stop Threshold
    if (pStreamRequest->sw.stopThreshold) {
      ALSA_CHECK(snd_pcm_sw_params_set_stop_threshold(pcmHandle, swParams, pStreamRequest->sw.stopThreshold));
    }

    // Apply the software parameters and see if the configuration sticks.
    err = snd_pcm_sw_params(pcmHandle, swParams);
    if (err < 0) {
      snd_pcm_close(pcmHandle);
      return SK_ERROR_NOT_SUPPORTED;
    }
  }

  //----------------------------------------------------------------------------
  // Gather all of the ICD parameters set from the stream.
  // If we cannot do this, the stream will be impossible to use - so we fail.
  //----------------------------------------------------------------------------
  {
    icdStreamInfo.sType = SK_STRUCTURE_TYPE_ICD_PCM_STREAM_INFO;
    icdStreamInfo.pNext = NULL;

    // Hardware Parameters
    icdStreamInfo.hw.blockingMode = pStreamRequest->hw.blockingMode;
    icdStreamInfo.hw.streamType = pStreamRequest->hw.streamType;
    EXPT_CHECK(snd_pcm_hw_params_get_access(hwParams, &icdStreamInfo.hw.accessMode));
    EXPT_CHECK(snd_pcm_hw_params_get_format(hwParams, &icdStreamInfo.hw.formatType));
    EXPT_CHECK(snd_pcm_hw_params_get_subformat(hwParams, &icdStreamInfo.hw.subformatType));
    EXPT_CHECK(snd_pcm_hw_params_get_channels(hwParams, &icdStreamInfo.hw.channels));
    EXPT_CHECK(snd_pcm_hw_params_get_rate(hwParams, &icdStreamInfo.hw.rate, &icdStreamInfo.hw.rateDir));
    EXPT_CHECK(snd_pcm_hw_params_get_periods(hwParams, &icdStreamInfo.hw.periods, &icdStreamInfo.hw.periodsDir));
    EXPT_CHECK(snd_pcm_hw_params_get_period_size(hwParams, &icdStreamInfo.hw.periodSize, &icdStreamInfo.hw.periodSizeDir));
    EXPT_CHECK(snd_pcm_hw_params_get_period_time(hwParams, &icdStreamInfo.hw.periodTime, &icdStreamInfo.hw.periodTimeDir));
    EXPT_CHECK(snd_pcm_hw_params_get_buffer_size(hwParams, &icdStreamInfo.hw.bufferSize));
    EXPT_CHECK(snd_pcm_hw_params_get_buffer_time(hwParams, &icdStreamInfo.hw.bufferTime, &icdStreamInfo.hw.bufferTimeDir));

    // Software Parameters
    EXPT_CHECK(snd_pcm_sw_params_get_avail_min(swParams, &icdStreamInfo.sw.availMin));
    EXPT_CHECK(snd_pcm_sw_params_get_silence_size(swParams, &icdStreamInfo.sw.silenceSize));
    EXPT_CHECK(snd_pcm_sw_params_get_silence_threshold(swParams, &icdStreamInfo.sw.silenceThreshold));
    EXPT_CHECK(snd_pcm_sw_params_get_start_threshold(swParams, &icdStreamInfo.sw.startThreshold));
    EXPT_CHECK(snd_pcm_sw_params_get_stop_threshold(swParams, &icdStreamInfo.sw.stopThreshold));
  }

  //----------------------------------------------------------------------------
  // Calculate all of the OpenSK parameters returned from the stream.
  // This should not fail, but it's instead a collection of calculated results.
  //----------------------------------------------------------------------------
  {
    streamInfo.sType = SK_STRUCTURE_TYPE_PCM_STREAM_INFO;
    streamInfo.pNext = NULL;
    streamInfo.streamType = skConvertFromLocalPcmStreamTypeIMPL(icdStreamInfo.hw.streamType);
    streamInfo.accessFlags = skConvertFromLocalAccessTypeIMPL(icdStreamInfo.hw.accessMode, icdStreamInfo.hw.blockingMode);
    streamInfo.formatType = skConvertFromLocalPcmFormatIMPL(icdStreamInfo.hw.formatType);
    streamInfo.formatBits = (uint32_t)snd_pcm_format_physical_width(icdStreamInfo.hw.formatType);
    streamInfo.sampleRate = icdStreamInfo.hw.rate;
    streamInfo.sampleBits = (uint32_t)snd_pcm_format_width(icdStreamInfo.hw.formatType);
    if (snd_pcm_format_big_endian(icdStreamInfo.hw.formatType)) {
      streamInfo.offsetBits = streamInfo.sampleBits - streamInfo.formatBits;
    }
    else {
      streamInfo.offsetBits = 0;
    }
    streamInfo.channels = icdStreamInfo.hw.channels;
    streamInfo.frameBits = (uint32_t)snd_pcm_frames_to_bytes(pcmHandle, 1) * 8;
    streamInfo.periodSamples = (uint32_t)icdStreamInfo.hw.periodSize;
    streamInfo.periodBits = streamInfo.frameBits * streamInfo.periodSamples;
    streamInfo.bufferSamples = (uint32_t)icdStreamInfo.hw.bufferSize;
    streamInfo.bufferBits = streamInfo.sampleBits * streamInfo.bufferSamples;
  }

  // Check if the returned information is not supported by OpenSK API
  if (streamInfo.accessFlags == SK_ACCESS_FLAG_BITS_MAX_ENUM) {
    snd_pcm_close(pcmHandle);
    return SK_ERROR_NOT_SUPPORTED;
  }

  //----------------------------------------------------------------------------
  // Success! Create the ALSA PCM stream!
  //----------------------------------------------------------------------------
  memset(&createInfo, 0, sizeof(SkPcmStreamCreateInfo));
  createInfo.sType = SK_STRUCTURE_TYPE_INTERNAL;
  createInfo.pfnGetPcmStreamProcAddr = &skGetPcmStreamProcAddr_alsa;
  createInfo.pUserData = &userData;
  userData.hwParams = hwParams;
  userData.swParams = swParams;
  userData.pcmHandle = pcmHandle;
  userData.pStreamInfo = &streamInfo;
  userData.pIcdStreamInfo = &icdStreamInfo;
  result = skCreatePcmStream(
    endpoint,
    &createInfo,
    skGetDriverIMPL(endpoint)->pAllocator,
    pStream
  );
  if (result != SK_SUCCESS) {
    snd_pcm_close(pcmHandle);
  }

  return result;
}
#undef ALSA_CHECK

static SkResult skConvertToLocalPcmRequestIMPL(
  SkPcmStreamRequest const*             pStreamRequest,
  SkAlsaPcmStreamRequest*               pIcdStreamRequest
) {
  SkStreamFlags originalStreamFlags;
  SkAccessFlags originalAccessFlags;

  // Initialize the stream request
  memset(pIcdStreamRequest, 0, sizeof(SkAlsaPcmStreamRequest));
  pIcdStreamRequest->sType = SK_STRUCTURE_TYPE_ICD_PCM_STREAM_REQUEST;
  pIcdStreamRequest->pNext = NULL;

  //----------------------------------------------------------------------------
  // Copy the access flags, and be sure that all of the bits were consumed.
  // This is useful in case OpenSK adds functionality, the user will get an
  // error that the stream construction fails instead of blindly ignoring flags.
  //----------------------------------------------------------------------------
  originalAccessFlags = pStreamRequest->accessFlags;
  {
    // Translate the PCM blocking flags
    if (originalAccessFlags & SK_ACCESS_BLOCKING_BIT) {
      pIcdStreamRequest->hw.blockingMode = 0;
      originalAccessFlags &= ~SK_ACCESS_BLOCKING_BIT;
    }
    else {
      pIcdStreamRequest->hw.blockingMode = SND_PCM_NONBLOCK;
    }

    // Translate to PCM access mode
    if (originalAccessFlags & SK_ACCESS_INTERLEAVED_BIT) {
      pIcdStreamRequest->hw.accessMode = SND_PCM_ACCESS_RW_INTERLEAVED;
      originalAccessFlags &= ~SK_ACCESS_INTERLEAVED_BIT;
    }
    else {
      pIcdStreamRequest->hw.accessMode = SND_PCM_ACCESS_RW_NONINTERLEAVED;
    }

    // Check for the memory mapping bit
    if (originalAccessFlags & SK_ACCESS_MEMORY_MAPPED_BIT) {
      switch (pIcdStreamRequest->hw.accessMode) {
        case SND_PCM_ACCESS_RW_INTERLEAVED:
          pIcdStreamRequest->hw.accessMode = SND_PCM_ACCESS_MMAP_INTERLEAVED;
          break;
        case SND_PCM_ACCESS_RW_NONINTERLEAVED:
          pIcdStreamRequest->hw.accessMode = SND_PCM_ACCESS_MMAP_NONINTERLEAVED;
          break;
        default:
          return SK_ERROR_INVALID;
      }
      originalAccessFlags &= ~SK_ACCESS_MEMORY_MAPPED_BIT;
    }

    // Check that the entire access flag was consumed and handled
    if (originalAccessFlags) {
      return SK_ERROR_NOT_SUPPORTED;
    }
  }

  //----------------------------------------------------------------------------
  // Copy the stream flags, and be sure that all of the bits were consumed.
  // This is less useful than for SkAccessFlags, but it's still safer to do.
  //----------------------------------------------------------------------------
  originalStreamFlags = pStreamRequest->streamType;
  {
    // Translate to PCM stream type
    if (originalStreamFlags & SK_STREAM_PCM_READ_BIT) {
      pIcdStreamRequest->hw.streamType = SND_PCM_STREAM_CAPTURE;
      originalStreamFlags &= ~SK_STREAM_PCM_READ_BIT;
    }
    else if (originalStreamFlags & SK_STREAM_PCM_WRITE_BIT) {
      pIcdStreamRequest->hw.streamType = SND_PCM_STREAM_PLAYBACK;
      originalStreamFlags &= ~SK_STREAM_PCM_WRITE_BIT;
    }
    else {
      // Somehow, this function got called for PCM streams w/o a PCM stream type.
      // Obviously this doesn't make sense, and we should fail as invalid.
      return SK_ERROR_INVALID;
    }

    // Check that the entire stream flag was consumed and handled
    if (originalStreamFlags) {
      return SK_ERROR_NOT_SUPPORTED;
    }
  }

  //----------------------------------------------------------------------------
  // Translate to PCM format type - if an unsupported type is passed in we will
  // get "unknown", if the type is undefined in pStreamRequest we will get "any"
  //----------------------------------------------------------------------------
  pIcdStreamRequest->hw.formatType = skConvertToLocalPcmFormatIMPL(pStreamRequest->formatType);
  if (pIcdStreamRequest->hw.formatType == SND_PCM_FORMAT_UNKNOWN) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  //----------------------------------------------------------------------------
  // Translate the numeric data for the PCM request - simple to do.
  //----------------------------------------------------------------------------
  pIcdStreamRequest->hw.channels = pStreamRequest->channels;
  pIcdStreamRequest->hw.rate = pStreamRequest->sampleRate;
  pIcdStreamRequest->hw.bufferSize = pStreamRequest->bufferSamples;

  return SK_SUCCESS;
}

static SkResult skJoinPcmStreamIMPL(
  SkPcmStream                           readStream,
  SkPcmStream                           writeStream,
  SkPcmStream*                          pStream
) {
  int err;
  SkDriver driver;

  // Check for invalid input.
  if (!readStream && !writeStream) {
    return SK_ERROR_INVALID;
  }

  // Check for early-out scenarios.
  if (readStream || writeStream) {
    if (!writeStream) {
      *pStream = readStream;
      return SK_SUCCESS;
    }
    else if (!readStream) {
      *pStream = writeStream;
      return SK_SUCCESS;
    }
  }

  // Grab the driver to assist in joining
  driver = skGetDriverIMPL(readStream);
  if (!driver || skGetObjectType(driver) != SK_OBJECT_TYPE_DRIVER) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Join the stream into one
  readStream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL] =
    writeStream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
  skDeinitializePcmStreamBase(writeStream, driver->pAllocator);
  skFree(driver->pAllocator, writeStream);

  // Attempt to form a hardware-synchronization between the streams.
  err = snd_pcm_link(
    readStream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle,
    readStream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle
  );
  (void)err;
  // TODO: Handle err code.

  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skRequestPcmStream_alsa(
  SkEndpoint                            endpoint,
  SkPcmStreamRequest const*             pStreamRequest,
  SkPcmStream*                          pStream
) {
  SkResult result;
  SkDriver driver;
  SkPcmStream readStream;
  SkPcmStream writeStream;
  SkAlsaPcmStreamRequest request;
  SkPcmStreamRequest const* pCurrStreamRequest;

  // Grab the driver instance (required for allocation routines)
  driver = skGetDriverIMPL(endpoint);
  if (!driver || skGetObjectType(driver) != SK_OBJECT_TYPE_DRIVER) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Initialize the stream instances
  result = SK_ERROR_INVALID;
  readStream = SK_NULL_HANDLE;
  writeStream = SK_NULL_HANDLE;

  // Enumerate through PCM requests until one is filled.
  // We must fill all valid request types to handle duplex streams.
  pCurrStreamRequest = pStreamRequest;
  while (pCurrStreamRequest) {

    // Convert the stream request into the ICD request type.
    switch (pCurrStreamRequest->sType) {
      case SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST:
        result = skConvertToLocalPcmRequestIMPL(pCurrStreamRequest, &request);
        break;
      case SK_STRUCTURE_TYPE_ICD_PCM_STREAM_REQUEST:
        result = SK_SUCCESS;
        memcpy(&request, pCurrStreamRequest, sizeof(SkAlsaPcmStreamRequest));
        break;
      default:
        result = SK_ERROR_INVALID;
        break;
    }
    if (result != SK_SUCCESS) {
      break;
    }

    // Construct the requested stream type if it doesn't already exist.
    switch (request.hw.streamType) {
      case SND_PCM_STREAM_PLAYBACK:
        if (!writeStream) {
          result = skRequestPcmStream_alsaIMPL(endpoint, &request, &writeStream);
        }
        break;
      case SND_PCM_STREAM_CAPTURE:
        if (!readStream) {
          result = skRequestPcmStream_alsaIMPL(endpoint, &request, &readStream);
        }
        break;
      default:
        result = SK_ERROR_INVALID;
        break;
    }
    if (result != SK_SUCCESS) {
      break;
    }

    // Early-Out: If both types are constructed, there is nothing more to do.
    if (readStream && writeStream) {
      break;
    }

    pCurrStreamRequest = (SkPcmStreamRequest const*)pCurrStreamRequest->pNext;
  }

  // If the streams were not constructed properly, abort and destroy.
  if (result != SK_SUCCESS) {
    if (readStream) {
      skDestroyPcmStream(readStream, driver->pAllocator);
    }
    if (writeStream) {
      skDestroyPcmStream(writeStream, driver->pAllocator);
    }
    return result;
  }

  // If multiple streams were constructed, join them into one.
  result = skJoinPcmStreamIMPL(readStream, writeStream, pStream);
  if (result != SK_SUCCESS) {
    if (readStream) {
      skDestroyPcmStream(readStream, driver->pAllocator);
    }
    if (writeStream) {
      skDestroyPcmStream(writeStream, driver->pAllocator);
    }
    return result;
  }

  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skCreatePcmStream_alsa(
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream*                          pStream
) {
  uint32_t idx;
  SkResult result;
  SkPcmStream stream;
  snd_pcm_chmap_t* channelMap;
  SkPcmStreamUserDataIMPL* pUserData;
  SkPcmStreamDataIMPL* pStreamData;

  // Find the data
  pUserData = pCreateInfo->pUserData;

  // Allocate the stream
  stream = skClearAllocate(
    pAllocator,
    sizeof(SkPcmStream_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  if (!stream) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Find out what kind of stream we are creating
  switch (pUserData->pStreamInfo->streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      skFree(pAllocator, stream);
      return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Allocate stream channel data
  pStreamData->pChannelMap = skClearAllocate(
    pAllocator,
    sizeof(SkChannel) * pUserData->pStreamInfo->channels,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  if (!stream) {
    skFree(pAllocator, stream);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Set the default channel mapping (in case the next call fails)
  for (idx = 0; idx < pUserData->pStreamInfo->channels; ++idx) {
    pStreamData->pChannelMap[idx] = SK_CHANNEL_UNKNOWN;
  }

  // Attempt to find the channel mapping provided by the device.
  channelMap = snd_pcm_get_chmap(pUserData->pcmHandle);
  if (channelMap) {
    for (idx = 0; idx < channelMap->channels && idx < pUserData->pStreamInfo->channels; ++idx) {
      pStreamData->pChannelMap[idx] = skConvertFromLocalChannelIMPL(channelMap->pos[idx]);
    }
    free(channelMap);
  }

  // Initialize the PCM stream internals
  pStreamData->pcmHandle = pUserData->pcmHandle;
  pStreamData->supportsPausing = (SkBool32)snd_pcm_hw_params_can_pause(pUserData->hwParams);
  memcpy(&pStreamData->streamInfo, pUserData->pStreamInfo, sizeof(SkPcmStreamInfo));
  memcpy(&pStreamData->icdStreamInfo, pUserData->pIcdStreamInfo, sizeof(SkAlsaPcmStreamInfo));

  // Create the create-info structure so that layers can construct.
  result = skInitializePcmStreamBase(
    pCreateInfo,
    pAllocator,
    stream
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, stream);
    return result;
  }

  *pStream = stream;
  return SK_SUCCESS;
}

static void SKAPI_CALL skDestroyPcmStream_alsa(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
) {
  skDeinitializePcmStreamBase(stream, pAllocator);
  skFree(pAllocator, stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pChannelMap);
  skFree(pAllocator, stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pChannelMap);
  skFree(pAllocator, stream);
}

static SkResult skClosePcmStreamDataIMPL(
  SkPcmStreamDataIMPL*                  pStreamData,
  SkBool32                              drain
) {
  int err;

  // If the stream was already closed, ignore.
  if (pStreamData->isClosed) {
    return SK_SUCCESS;
  }

  // First, attempt to drain or drop the existing samples.
  if (drain) {
    err = snd_pcm_drain(pStreamData->pcmHandle);
  }
  else {
    err = snd_pcm_drop(pStreamData->pcmHandle);
  }
  if (err < 0) {
    switch (err) {
      case -EIO:
        // This stream was already closed!
        return SK_ERROR_INVALID;
      default:
        return SK_ERROR_SYSTEM_INTERNAL;
    }
  }

  // Close the PCM stream and deallocate.
  err = snd_pcm_close(pStreamData->pcmHandle);
  if (err < 0) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }
  pStreamData->isClosed = SK_TRUE;

  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skClosePcmStream_alsa(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  SkResult result;
  if (stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle) {
    result = skClosePcmStreamDataIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL], drain);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  if (stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle) {
    result = skClosePcmStreamDataIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL], drain);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  skDestroyPcmStream(stream, skGetDriverIMPL(stream)->pAllocator);
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skGetPcmStreamInfo_alsa(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkPcmStreamInfo*                      pStreamInfo
) {
  SkPcmStreamDataIMPL* pStreamData;

  // Grab the stream index type.
  switch (streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      return SK_ERROR_INVALID;
  }

  // If this stream does not contain this kind of type, return that.
  if (!pStreamData->pcmHandle) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  memcpy(pStreamInfo, &pStreamData->streamInfo, sizeof(SkPcmStreamInfo));
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skGetPcmStreamChannelMap_alsa(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel*                            pChannelMap
) {
  SkPcmStreamDataIMPL* pStreamData;

  // Grab the stream index type.
  switch (streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      return SK_ERROR_INVALID;
  }

  // If this stream does not contain this kind of type, return that.
  if (!pStreamData->pcmHandle) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  // Copy the channel map data into the provided pointer.
  memcpy(
    pChannelMap,
    pStreamData->pChannelMap,
    sizeof(SkChannel) * pStreamData->streamInfo.channels
  );
  return SK_SUCCESS;
}

static SkResult skSetPcmStreamChannelMap_alsa(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel const*                      pChannelMap
) {
  SkPcmStreamDataIMPL* pStreamData;

  // Grab the stream index type.
  switch (streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      return SK_ERROR_INVALID;
  }

  // If this stream does not contain this kind of type, return that.
  if (!pStreamData->pcmHandle) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  // Copy the channel map data into the provided pointer.
  memcpy(
    pStreamData->pChannelMap,
    pChannelMap,
    sizeof(SkChannel) * pStreamData->streamInfo.channels
  );
  return SK_SUCCESS;
}

static SkResult skHandlePcmStreamErrorsIMPL(
  SkPcmStreamDataIMPL*                  pStreamData,
  int                                   error
) {
  switch (error) {
    case -EPIPE:
      // The device over/under-ran, recover it.
      pStreamData->lastError = error;
      return SK_ERROR_XRUN;
    case -ESTRPIPE:
      // The device was suspended (low power, manually, etc.), recover it.
      pStreamData->lastError = error;
      return SK_ERROR_SUSPENDED;
    case -EINTR:
      // The device was interrupted and isn't sure if it should continue, recover it.
      pStreamData->lastError = error;
      return SK_ERROR_INTERRUPTED;
    case -EAGAIN:
      // Try again later, the device is busy - does not need to be recovered.
      pStreamData->lastError = 0;
      return SK_ERROR_BUSY;
    default:
      // Something really bad happened - check to see what it was.
      pStreamData->lastError = error;
      return skHandleCtlErrorIMPL(error);
  }
}

static SkResult skStartPcmStreamIMPL(
  SkPcmStreamDataIMPL*                  pStreamData
) {
  int err;
  err = snd_pcm_start(pStreamData->pcmHandle);
  if (err < 0) {
    return skHandlePcmStreamErrorsIMPL(pStreamData, err);
  }
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skStartPcmStream_alsa(
  SkPcmStream                           stream
) {
  SkResult result;
  if (stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle) {
    result = skStartPcmStreamIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL]);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  if (stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle) {
    result = skStartPcmStreamIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL]);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  return SK_SUCCESS;
}

static SkResult skStopPcmStreamIMPL(
  SkPcmStreamDataIMPL*                  pStreamData,
  SkBool32                              drain
) {
  int err;
  if (drain) {
    err = snd_pcm_drain(pStreamData->pcmHandle);
  }
  else {
    err = snd_pcm_drop(pStreamData->pcmHandle);
  }
  if (err < 0) {
    return skHandlePcmStreamErrorsIMPL(pStreamData, err);
  }
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skStopPcmStream_alsa(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  SkResult result;
  if (stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle) {
    result = skStopPcmStreamIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL], drain);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  if (stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle) {
    result = skStopPcmStreamIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL], drain);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  return SK_SUCCESS;
}

static SkResult skPausePcmStreamIMPL(
  SkPcmStreamDataIMPL*                  pStreamData,
  SkBool32                              pause
) {
  int err;
  if (!pStreamData->supportsPausing) {
    return SK_ERROR_FEATURE_NOT_PRESENT;
  }
  err = snd_pcm_pause(pStreamData->pcmHandle, pause);
  return skHandlePcmStreamErrorsIMPL(pStreamData, err);
}

static SkResult SKAPI_CALL skPausePcmStream_alsa(
  SkPcmStream                           stream,
  SkBool32                              pause
) {
  SkResult result;
  if (stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle) {
    result = skPausePcmStreamIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL], pause);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  if (stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle) {
    result = skPausePcmStreamIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL], pause);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  return SK_SUCCESS;
}

static SkResult skRecoverPcmStreamIMPL(
  SkPcmStreamDataIMPL*                  pStreamData
) {
  // If there was no error, return - nothing to do.
  if (!pStreamData->lastError) {
    return SK_SUCCESS;
  }
  // Otherwise, attempt to recover the error.
  pStreamData->lastError = snd_pcm_recover(pStreamData->pcmHandle, pStreamData->lastError, 1);
  if (pStreamData->lastError < 0) {
    return skHandlePcmStreamErrorsIMPL(pStreamData, pStreamData->lastError);
  }
  pStreamData->lastError = 0;
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skRecoverPcmStream_alsa(
  SkPcmStream                           stream
) {
  SkResult result;
  if (stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle) {
    result = skRecoverPcmStreamIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL]);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  if (stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle) {
    result = skRecoverPcmStreamIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL]);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skWaitPcmStream_alsa(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  int32_t                               timeout
) {
  int err;
  SkPcmStreamDataIMPL* pStreamData;

  // Grab the stream index type.
  switch (streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      return SK_ERROR_INVALID;
  }

  // If this stream does not contain this kind of type, return that.
  if (!pStreamData->pcmHandle) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  // Attempt to wait for a given timeout.
  if (timeout) {
    err = snd_pcm_wait(pStreamData->pcmHandle, timeout);
    if (err < 0) {
      return skHandlePcmStreamErrorsIMPL(pStreamData, err);
    }
    if (err == 0) {
      return SK_TIMEOUT;
    }
  }

  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skAvailPcmStreamSamples_alsa(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  uint32_t*                             pAvailable
) {
  snd_pcm_sframes_t err;
  SkPcmStreamDataIMPL* pStreamData;

  // Grab the stream index type.
  switch (streamType) {
    case SK_STREAM_PCM_READ_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_READ_INDEX_IMPL];
      break;
    case SK_STREAM_PCM_WRITE_BIT:
      pStreamData = &stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL];
      break;
    default:
      return SK_ERROR_INVALID;
  }

  // If this stream does not contain this kind of type, return that.
  if (!pStreamData->pcmHandle) {
    return SK_ERROR_NOT_SUPPORTED;
  }

  // Attempt to find the number of available samples.
  err = snd_pcm_avail(pStreamData->pcmHandle);
  if (err < 0) {
    return skHandlePcmStreamErrorsIMPL(pStreamData, (int)err);
  }
  *pAvailable = (uint32_t)err;
  return SK_SUCCESS;
}

static int64_t SKAPI_CALL skWritePcmStreamInterleaved_alsa(
  SkPcmStream                           stream,
  void const*                           pBuffer,
  uint32_t                              samples
) {
  snd_pcm_sframes_t frames;
  frames = snd_pcm_writei(stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle, pBuffer, (snd_pcm_uframes_t)samples);
  if (frames < 0) {
    return skHandlePcmStreamErrorsIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL], (int)frames);
  }
  return frames;
}

static int64_t SKAPI_CALL skWritePcmStreamNoninterleaved_alsa(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  snd_pcm_sframes_t frames;
  frames = snd_pcm_writen(stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL].pcmHandle, pBuffer, (snd_pcm_uframes_t)samples);
  if (frames < 0) {
    return skHandlePcmStreamErrorsIMPL(&stream->data[SK_PCM_STREAM_WRITE_INDEX_IMPL], (int)frames);
  }
  return frames;
}

static int64_t SKAPI_CALL skReadPcmStreamInterleaved_alsa(
  SkPcmStream                           stream,
  void*                                 pBuffer,
  uint32_t                              samples
) {
  snd_pcm_sframes_t frames;
  frames = snd_pcm_readi(stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle, pBuffer, (snd_pcm_uframes_t)samples);
  if (frames < 0) {
    return skHandlePcmStreamErrorsIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL], (int)frames);
  }
  return frames;
}

static int64_t SKAPI_CALL skReadPcmStreamNoninterleaved_alsa(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  snd_pcm_sframes_t frames;
  frames = snd_pcm_readn(stream->data[SK_PCM_STREAM_READ_INDEX_IMPL].pcmHandle, pBuffer, (snd_pcm_uframes_t)samples);
  if (frames < 0) {
    return skHandlePcmStreamErrorsIMPL(&stream->data[SK_PCM_STREAM_READ_INDEX_IMPL], (int)frames);
  }
  return frames;
}

////////////////////////////////////////////////////////////////////////////////
// Driver Entrypoint (Also Required)
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR void SKAPI_CALL skGetDriverProperties_alsa(
  SkDriver                             driver,
  SkDriverProperties*                  pProperties
) {
  (void)driver;
  pProperties->apiVersion = SK_API_VERSION_0_0;
  pProperties->implVersion = SK_MAKE_VERSION(0, 0, 0);
  strcpy(pProperties->driverName, SK_DRIVER_OPENSK_ALSA);
  strcpy(pProperties->description, SK_DRIVER_OPENSK_ALSA_DESCRIPTION);
  strcpy(pProperties->displayName, SK_DRIVER_OPENSK_ALSA_DISPLAY_NAME);
  strcpy(pProperties->identifier, SK_DRIVER_OPENSK_ALSA_ID);
  memcpy(pProperties->driverUuid, SK_DRIVER_OPENSK_ALSA_UUID, SK_UUID_SIZE);
}

#define HANDLE_PROC(name)                                                       \
if (strcmp(symbol, #name) == 0) return (PFN_skVoidFunction)&name##_alsa
SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr_alsa(
  SkDriver                              driver,
  char const*                           symbol
) {
  (void)driver;
  HANDLE_PROC(skGetDriverProcAddr);
  HANDLE_PROC(skCreateDriver);
  HANDLE_PROC(skDestroyDriver);
  HANDLE_PROC(skGetDriverProperties);
  HANDLE_PROC(skGetDriverFeatures);
  HANDLE_PROC(skEnumerateDriverDevices);
  HANDLE_PROC(skEnumerateDriverEndpoints);
  HANDLE_PROC(skQueryDeviceFeatures);
  HANDLE_PROC(skQueryDeviceProperties);
  HANDLE_PROC(skEnumerateDeviceEndpoints);
  HANDLE_PROC(skQueryEndpointFeatures);
  HANDLE_PROC(skQueryEndpointProperties);
  HANDLE_PROC(skRequestPcmStream);
  return NULL;
}

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr_alsa(
  SkPcmStream                           stream,
  char const*                           symbol
) {
  (void)stream;
  HANDLE_PROC(skGetPcmStreamProcAddr);
  HANDLE_PROC(skCreatePcmStream);
  HANDLE_PROC(skDestroyPcmStream);
  HANDLE_PROC(skClosePcmStream);
  HANDLE_PROC(skGetPcmStreamInfo);
  HANDLE_PROC(skGetPcmStreamChannelMap);
  HANDLE_PROC(skSetPcmStreamChannelMap);
  HANDLE_PROC(skStartPcmStream);
  HANDLE_PROC(skStopPcmStream);
  HANDLE_PROC(skRecoverPcmStream);
  HANDLE_PROC(skWaitPcmStream);
  HANDLE_PROC(skPausePcmStream);
  HANDLE_PROC(skAvailPcmStreamSamples);
  HANDLE_PROC(skWritePcmStreamInterleaved);
  HANDLE_PROC(skWritePcmStreamNoninterleaved);
  HANDLE_PROC(skReadPcmStreamInterleaved);
  HANDLE_PROC(skReadPcmStreamNoninterleaved);
  return NULL;
}
#undef HANDLE_PROC
