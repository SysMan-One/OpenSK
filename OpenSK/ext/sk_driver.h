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
 * OpenSK extensibility header. (Will be present in final package.)
 ******************************************************************************/
#ifndef   OPENSK_EXT_DRIVER_H
#define   OPENSK_EXT_DRIVER_H 1

// OpenSK
#include <OpenSK/ext/sk_global.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Public Types
////////////////////////////////////////////////////////////////////////////////

typedef SkResult (SKAPI_PTR *PFN_skCreateDriver)(SkDriverCreateInfo const* pCreateInfo, SkAllocationCallbacks const* pAllocator, SkDriver* pDriver);
typedef SkResult (SKAPI_PTR *PFN_skDestroyDriver)(SkAllocationCallbacks const* pAllocator, SkDriver driver);

typedef struct SkDeviceCreateInfo {
  SK_INTERNAL_STRUCTURE_BASE;
  SkDriver                              deviceParent;
  uint8_t                               deviceUuid[SK_UUID_SIZE];
} SkDeviceCreateInfo;

typedef struct SkEndpointCreateInfo {
  SK_INTERNAL_STRUCTURE_BASE;
  SkObject                              endpointParent;
  uint8_t                               endpointUuid[SK_UUID_SIZE];
} SkEndpointCreateInfo;

typedef struct SkDriverFunctionTable {
  // Core: 1.0.0
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr;
  PFN_skCreateDriver                    pfnCreateDriver;
  PFN_skDestroyDriver                   pfnDestroyDriver;
  PFN_skGetDriverProperties             pfnGetDriverProperties;
  PFN_skGetDriverFeatures               pfnGetDriverFeatures;
  PFN_skEnumerateDriverDevices          pfnEnumerateDriverDevices;
  PFN_skEnumerateDriverEndpoints        pfnEnumerateDriverEndpoints;
  PFN_skQueryDeviceFeatures             pfnQueryDeviceFeatures;
  PFN_skQueryDeviceProperties           pfnQueryDeviceProperties;
  PFN_skEnumerateDeviceEndpoints        pfnEnumerateDeviceEndpoints;
  PFN_skQueryEndpointFeatures           pfnQueryEndpointFeatures;
  PFN_skQueryEndpointProperties         pfnQueryEndpointProperties;
  PFN_skRequestPcmStream                pfnRequestPcmStream;
} SkDriverFunctionTable;
SK_DEFINE_HANDLE(SkDriverLayer);

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateDriverFunctionTable(
  SkDriver                              driver,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr,
  SkDriverFunctionTable const*          pPreviousFunctionTable,
  SkDriverFunctionTable**               ppFunctionTable
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverBase(
  SkDriverCreateInfo const*             pDriverCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDeviceBase(
  SkDeviceCreateInfo const*             pDeviceCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDevice                              device
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeEndpointBase(
  SkEndpointCreateInfo const*           pEndpointCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkEndpoint                            endpoint
);

SKAPI_ATTR void SKAPI_CALL skDeinitializeDriverBase(
  SkDriver                              driver,
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR void SKAPI_CALL skDeinitializeDeviceBase(
  SkDevice                              device,
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR void SKAPI_CALL skDeinitializeEndpointBase(
  SkEndpoint                            endpoint,
  SkAllocationCallbacks const*          pAllocator
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_DRIVER_H
