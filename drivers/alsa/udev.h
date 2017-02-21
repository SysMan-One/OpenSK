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
 * Note: It may make sense to move this to a more common location for Linux.
 ******************************************************************************/

#ifndef   OPENSK_ALSA_UDEV_H
#define   OPENSK_ALSA_UDEV_H 1

// OpenSK
#include <OpenSK/opensk.h>

////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkUDevIMPL);

typedef struct SkUDeviceInfoIMPL {
  uint32_t                              vendorId;
  uint32_t                              modelId;
  uint32_t                              systemId;
  uint64_t                              serialId;
  uint8_t                               deviceUuid[SK_UUID_SIZE];
  SkDeviceType                          deviceType;
} SkUDeviceInfoIMPL;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateUDevIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkUDevIMPL*                           pUdev
);

SKAPI_ATTR void SKAPI_CALL skDestroyUDevIMPL(
  SkUDevIMPL                            udev,
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateUDevDevicesIMPL(
  SkUDevIMPL                            udev,
  uint32_t*                             pDeviceCount,
  SkUDeviceInfoIMPL*                    pDevices
);

#endif // OPENSK_ALSA_UDEV_H
