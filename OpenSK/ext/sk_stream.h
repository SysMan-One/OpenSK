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
#ifndef   OPENSK_EXT_STREAM_H
#define   OPENSK_EXT_STREAM_H 1

// OpenSK
#include <OpenSK/ext/sk_global.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Public Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkPcmStreamCreateInfo {
  SK_INTERNAL_STRUCTURE_BASE;
  void*                                 pUserData;
  void const*                           pStreamInfo;
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr;
} SkPcmStreamCreateInfo;

typedef SkResult (SKAPI_PTR *PFN_skCreatePcmStream)(SkPcmStreamCreateInfo const* pCreateInfo, SkAllocationCallbacks const* pAllocator, SkPcmStream* pStream);
typedef void (SKAPI_PTR *PFN_skDestroyPcmStream)(SkPcmStream stream, SkAllocationCallbacks const* pAllocator);

typedef struct SkPcmStreamFunctionTable {
  // Core: 1.0.0
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr;
  PFN_skDestroyPcmStream                pfnDestroyPcmStream;
  PFN_skClosePcmStream                  pfnClosePcmStream;
  PFN_skGetPcmStreamInfo                pfnGetPcmStreamInfo;
  PFN_skGetPcmStreamChannelMap          pfnGetPcmStreamChannelMap;
  PFN_skSetPcmStreamChannelMap          pfnSetPcmStreamChannelMap;
  PFN_skStartPcmStream                  pfnStartPcmStream;
  PFN_skStopPcmStream                   pfnStopPcmStream;
  PFN_skRecoverPcmStream                pfnRecoverPcmStream;
  PFN_skWaitPcmStream                   pfnWaitPcmStream;
  PFN_skPausePcmStream                  pfnPausePcmStream;
  PFN_skAvailPcmStreamSamples           pfnAvailPcmStreamSamples;
  PFN_skWritePcmStreamInterleaved       pfnWritePcmStreamInterleaved;
  PFN_skWritePcmStreamNoninterleaved    pfnWritePcmStreamNoninterleaved;
  PFN_skReadPcmStreamInterleaved        pfnReadPcmStreamInterleaved;
  PFN_skReadPcmStreamNoninterleaved     pfnReadPcmStreamNoninterleaved;
} SkPcmStreamFunctionTable;
SK_DEFINE_HANDLE(SkPcmStreamLayer);

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreatePcmStream(
  SkEndpoint                            endpoint,
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream*                          pStream
);

SKAPI_ATTR void SKAPI_CALL skDestroyPcmStream(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR SkResult SKAPI_CALL skCreatePcmStreamFunctionTable(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr,
  SkPcmStreamFunctionTable const*       pPreviousFunctionTable,
  SkPcmStreamFunctionTable**            ppFunctionTable
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializePcmStreamBase(
  SkPcmStreamCreateInfo const*          pPcmStreamCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream                           stream
);

SKAPI_ATTR void SKAPI_CALL skDeinitializePcmStreamBase(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_STREAM_H
