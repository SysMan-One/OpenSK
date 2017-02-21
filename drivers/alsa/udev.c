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
 * Helper functions for discovering more metadata for device hardware.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/md5.h>
#include <OpenSK/ext/sk_global.h>

// C99
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Non-Standard
#include <dlfcn.h>
#include <libudev.h>

// Internal
#include "udev.h"

////////////////////////////////////////////////////////////////////////////////
// Internal Defines
////////////////////////////////////////////////////////////////////////////////

#define ishexdigit(c) (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define htoi(c) ((isdigit(c)) ? (c - '0') : (tolower(c) - 'a' + 10))

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkUDevIMPL_T {
  SkAllocationCallbacks const*          pAllocator;
  struct udev*                          udev;
  uint32_t                              udeviceCount;
  uint32_t                              udeviceCapacity;
  SkUDeviceInfoIMPL*                    pUDeviceInfo;
} SkUDevIMPL_T;

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static uint64_t
djb2(unsigned char *str)
{
  uint64_t hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash;
}

static uint32_t parseHexadecimal(char const* pString) {
  uint32_t result = 0;

  // Skip all whitespace characters
  while (*pString == ' ' || *pString == '\t') {
    ++pString;
  }

  // Skip the "0x" prefix which is sometimes attached to hexadecimal numbers.
  if (*pString == '0' && (pString[1] == 'x' || pString[1] == 'X')) {
    pString += 2;
  }

  // Read the number as-if it's hexadecimal
  while (ishexdigit(*pString)) {
    result *= 0x10;
    result += htoi(*pString);
    ++pString;
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateUDevIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkUDevIMPL*                           pUdev
) {
  SkUDevIMPL udev;
  struct udev* udevInternal;

  // Attempt to construct the udev instance
  udevInternal = udev_new();
  if (!udevInternal) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Allocate and initialize the udev instance
  udev = skClearAllocate(
    pAllocator,
    sizeof(SkUDevIMPL_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!udev) {
    udev_unref(udevInternal);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  udev->udev = udevInternal;
  udev->pAllocator = pAllocator;

  // Note: No need to allocate, everyone shares one udev instance.
  *pUdev = udev;
  return SK_SUCCESS;
}
#undef LOAD_PROC

SKAPI_ATTR void SKAPI_CALL skDestroyUDevIMPL(
  SkUDevIMPL                            udev,
  SkAllocationCallbacks const*          pAllocator
) {
  skFree(pAllocator, udev->pUDeviceInfo);
  udev_unref(udev->udev);
  skFree(pAllocator, udev);
}

/**
 * Note: Unfortunately, after much research, I've found that libudev decides
 *       a lot of things in a fairly hacky way. For example, form factor is
 *       determined by a string compare with the product name. If the name
 *       contains a string like "[sS]peaker", the form factor is "speaker".
 *       The problem with this is not manny products say what they are in the
 *       product name. Not to mention this information _is_ available by the
 *       USB standard in a safer way. The underlying problem seems to be that
 *       sysfs doesn't make this information available, and libdev is built
 *       on sysfs information.
 *       For now, we accept this shortcoming - in the future we may have to
 *       construct more reliable code to extract this information. My personal
 *       recommendation is to construct a library which abstracts access to
 *       low-level device information (USB, PCI, etc.) this may be difficult.
 */
static SkResult skInitializeUDevDeviceIMPL(
  SkUDevIMPL                            udev,
  struct udev_device*                   uDevice
) {
  MD5_CTX ctx;
  char const* pCharConst;
  SkUDeviceInfoIMPL deviceInfo;

  // Find the audio subsystem:ALSA device pairing.
  pCharConst = udev_device_get_sysnum(uDevice);
  if (!pCharConst)
  {
    return SK_INCOMPLETE;
  }
  deviceInfo.systemId = (unsigned)atoi(pCharConst);

  // Get the form factor
  pCharConst = udev_device_get_property_value(uDevice, "SOUND_FORM_FACTOR");
  if (!pCharConst) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_UNKNOWN;
  }
  else if (strcmp(pCharConst, "internal") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_INTERNAL;
  }
  else if (strcmp(pCharConst, "microphone") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_MICROPHONE;
  }
  else if (strcmp(pCharConst, "speaker") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_SPEAKER;
  }
  else if (strcmp(pCharConst, "headphone") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_HEADPHONE;
  }
  else if (strcmp(pCharConst, "headset") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_HEADSET;
  }
  else if (strcmp(pCharConst, "webcam") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_WEBCAM;
  }
  else if (strcmp(pCharConst, "headset") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_HEADSET;
  }
  else if (strcmp(pCharConst, "handset") == 0) {
    deviceInfo.deviceType = SK_DEVICE_TYPE_HANDSET;
  }
  else {
    deviceInfo.deviceType = SK_DEVICE_TYPE_OTHER;
  }

  // Get Vendor ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_VENDOR_ID");
  if (pCharConst) {
    deviceInfo.vendorId = parseHexadecimal(pCharConst);
  }

  // Get Model ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_MODEL_ID");
  if (pCharConst) {
    deviceInfo.modelId = parseHexadecimal(pCharConst);
  }

  // Get Serial ID
  pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL");
  if (!pCharConst) {
    pCharConst = udev_device_get_property_value(uDevice, "ID_SERIAL_SHORT");
  }
  if (pCharConst) {
    deviceInfo.serialId = djb2((unsigned char*)pCharConst);
  }

  // Generate device ID
  // NOTE: This is one of: ID_FOR_SEAT, ID_PATH, ID_PATH_TAG, DEVPATH in that order.
  //       If an ID was found, we should hash it twice and store in the UUID.
  //       Fallback is {vendorUUID}{modelUUID}{serialId||systemId} all appended together.
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
    MD5_Init(&ctx);
    MD5_Update(&ctx, pCharConst, strlen(pCharConst));
    MD5_Final(deviceInfo.deviceUuid, &ctx);
  }
  else {
    memcpy(&deviceInfo.deviceUuid[0], &deviceInfo.modelId, sizeof(uint32_t));
    memcpy(&deviceInfo.deviceUuid[4], &deviceInfo.vendorId, sizeof(uint32_t));
    memcpy(&deviceInfo.deviceUuid[8], &deviceInfo.serialId, sizeof(uint64_t));
    deviceInfo.serialId += deviceInfo.systemId;
    deviceInfo.serialId -= deviceInfo.systemId;
  }

  // Grow the udevice info array if needed
  if (udev->udeviceCount >= udev->udeviceCapacity) {
    if (!udev->udeviceCapacity) {
      udev->udeviceCapacity = 1;
    }
    udev->udeviceCapacity *= 2;
    udev->pUDeviceInfo = skReallocate(
      udev->pAllocator,
      udev->pUDeviceInfo,
      sizeof(SkUDeviceInfoIMPL) * udev->udeviceCapacity,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
    );
    if (!udev->pUDeviceInfo) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }

  // Copy the udevice info into the array.
  memcpy(&udev->pUDeviceInfo[udev->udeviceCount], &deviceInfo, sizeof(SkUDeviceInfoIMPL));
  ++udev->udeviceCount;

  return SK_SUCCESS;
}

static SkResult skInitializeUDevEndpointIMPL(
  SkUDevIMPL                            udev,
  struct udev_device*                   uDevice
) {
  // Right now, we don't have any need for component-level UDEV.
  (void)udev;
  (void)uDevice;
  return SK_SUCCESS;
}

static SkResult skAddOrUpdateDeviceIMPL(
  SkUDevIMPL                            udev,
  struct udev_device*                   uDevice
) {
  char const* pCharConst;
  struct udev_device* uParent;

  uParent = udev_device_get_parent(uDevice);

  if (uParent) {
    // Check if this is a UDEV "card"
    pCharConst = udev_device_get_subsystem(uParent);
    if (strcmp(pCharConst, "sound") != 0) {
      return skInitializeUDevDeviceIMPL(udev, uDevice);
    }
    // Otherwise, this must be a UDEV "device"
    else {
      return skInitializeUDevEndpointIMPL(udev, uDevice);
    }
  }

  return SK_ERROR_SYSTEM_INTERNAL;
}

static SkResult skScanUDevDevicesIMPL(
  SkUDevIMPL                            udev
) {
  char const *path;
  struct udev_device* device;
  struct udev_enumerate* enumerate;
  struct udev_list_entry* devices, *iterator;

  // Null-out the current devices.
  udev->udeviceCount = 0;

  // Enumerate all "sound" subsystem devices
  enumerate = udev_enumerate_new(udev->udev);
  udev_enumerate_add_match_subsystem(enumerate, "sound");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);

  // Process each subsystem device
  udev_list_entry_foreach(iterator, devices) {
    path = udev_list_entry_get_name(iterator);
    device = udev_device_new_from_syspath(udev->udev, path);
    skAddOrUpdateDeviceIMPL(udev, device);
    udev_device_unref(device);
  }
  udev_enumerate_unref(enumerate);

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateUDevDevicesIMPL(
  SkUDevIMPL                            udev,
  uint32_t*                             pDeviceCount,
  SkUDeviceInfoIMPL*                    pDevices
) {
  uint32_t count;
  SkResult result;

  // Update the internal device list.
  result = skScanUDevDevicesIMPL(udev);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Return the device count if it's requested.
  if (!pDevices) {
    *pDeviceCount = udev->udeviceCount;
    return SK_SUCCESS;
  }

  // Otherwise return the device information.
  for (count = 0; count < *pDeviceCount; ++count) {
    if (count >= *pDeviceCount) {
      return SK_INCOMPLETE;
    }
    memcpy(&pDevices[count], &udev->pUDeviceInfo[count], sizeof(SkUDeviceInfoIMPL));
  }

  return SK_SUCCESS;
}
