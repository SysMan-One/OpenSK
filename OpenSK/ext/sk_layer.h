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
#ifndef   OPENSK_EXT_LAYER_H
#define   OPENSK_EXT_LAYER_H 1

// OpenSK
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/ext/sk_driver.h>
#include <OpenSK/ext/sk_stream.h>

// C99
#include <string.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (SKAPI_PTR *PFN_skGetLayerProperties)(SkLayerProperties *pProperties);

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skInitializeInstanceLayerBase(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstanceLayer                       instanceLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkInstance*                           pInstance
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverLayerBase(
  SkDriverCreateInfo const*             pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriverLayer                         driverLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkDriver*                             pDriver
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializePcmStreamLayerBase(
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStreamLayer                      pcmStreamLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkPcmStream *                         pPcmStream
);

SKAPI_ATTR void SKAPI_CALL skDeinitializeInstanceLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkInstanceLayer                       instanceLayer
);

SKAPI_ATTR void SKAPI_CALL skDeinitializeDriverLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkDriverLayer                         driverLayer
);

SKAPI_ATTR void SKAPI_CALL skDeinitializePcmStreamLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStreamLayer                      pcmStreamLayer
);

SKAPI_ATTR SkInstanceLayer SKAPI_CALL skGetInstanceLayer(
  SkInstance                            instance,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkInstanceFunctionTable const**       ppFunctionTable
);

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayer(
  SkDriver                              driver,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
);

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayerFromDevice(
  SkDevice                              device,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
);

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayerFromEndpoint(
  SkEndpoint                            endpoint,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
);

SKAPI_ATTR SkPcmStreamLayer SKAPI_CALL skGetPcmStreamLayer(
  SkPcmStream                           pcmStream,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkPcmStreamFunctionTable const**      ppFunctionTable
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_LAYER_H
