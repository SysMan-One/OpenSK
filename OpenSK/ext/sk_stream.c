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
 * Functions and types which assist with creating an OpenSK stream extension.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/objects.h>
#include <OpenSK/ext/sk_stream.h>
#include <OpenSK/plt/platform.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

#define HANDLE_PROC(name) if((pfnVoidFunction = pfnGetPcmStreamProcAddr(stream, "sk" #name))) pFunctionTable->pfn##name = (PFN_sk##name)pfnVoidFunction
SKAPI_ATTR SkResult SKAPI_CALL skCreatePcmStreamFunctionTable(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr,
  SkPcmStreamFunctionTable const*       pPreviousFunctionTable,
  SkPcmStreamFunctionTable**            ppFunctionTable
) {
  PFN_skVoidFunction pfnVoidFunction;
  SkPcmStreamFunctionTable *pFunctionTable;

  // Allocate the function table
  pFunctionTable = skAllocate(
    pAllocator,
    sizeof(SkPcmStreamFunctionTable),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  if (!pFunctionTable) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy the previous function table over, or memset 0.
  if (pPreviousFunctionTable) {
    memcpy(pFunctionTable, pPreviousFunctionTable, sizeof(SkPcmStreamFunctionTable));
  }
  else {
    memset(pFunctionTable, 0, sizeof(SkPcmStreamFunctionTable));
  }

  // Core 1.0.0
  HANDLE_PROC(GetPcmStreamProcAddr);
  HANDLE_PROC(DestroyPcmStream);
  HANDLE_PROC(ClosePcmStream);
  HANDLE_PROC(GetPcmStreamInfo);
  HANDLE_PROC(GetPcmStreamChannelMap);
  HANDLE_PROC(SetPcmStreamChannelMap);
  HANDLE_PROC(StartPcmStream);
  HANDLE_PROC(StopPcmStream);
  HANDLE_PROC(RecoverPcmStream);
  HANDLE_PROC(WaitPcmStream);
  HANDLE_PROC(PausePcmStream);
  HANDLE_PROC(AvailPcmStreamSamples);
  HANDLE_PROC(WritePcmStreamInterleaved);
  HANDLE_PROC(WritePcmStreamNoninterleaved);
  HANDLE_PROC(ReadPcmStreamInterleaved);
  HANDLE_PROC(ReadPcmStreamNoninterleaved);

  // Return success
  *ppFunctionTable = pFunctionTable;
  return SK_SUCCESS;
}
#undef HANDLE_PROC

SKAPI_ATTR SkResult SKAPI_CALL skInitializePcmStreamBase(
  SkPcmStreamCreateInfo const*          pPcmStreamCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream                           stream
) {
  SkResult result;
  uint8_t uuid[SK_UUID_SIZE];
  SkInternalObjectBase* pTrampolineLayer;

  // Initialize a random GUID
  skGenerateUuid(uuid);

  // Construct a dummy layer which will catch trampolined calls
  pTrampolineLayer = skAllocate(
    pAllocator,
    sizeof(SkInternalObjectBase),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  if (!pTrampolineLayer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Configure the stream's trampoline layer
  pTrampolineLayer->_oType = SK_OBJECT_TYPE_LAYER;
  pTrampolineLayer->_pNext = NULL;
  pTrampolineLayer->_pParent = (SkInternalObjectBase*)stream;
  memcpy(pTrampolineLayer->_iUuid, uuid, SK_UUID_SIZE);

  // Initialize the stream
  result = skCreatePcmStreamFunctionTable(
    stream,
    pAllocator,
    pPcmStreamCreateInfo->pfnGetPcmStreamProcAddr,
    NULL,
    (SkPcmStreamFunctionTable**)&pTrampolineLayer->_vtable
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, pTrampolineLayer);
    return result;
  }

  // Configure the stream internals
  stream->_oType = SK_OBJECT_TYPE_PCM_STREAM;
  stream->_pNext = pTrampolineLayer;
  stream->_vtable = pTrampolineLayer->_vtable;
  memcpy(stream->_iUuid, uuid, SK_UUID_SIZE);

  // Call into all of the required PCM layers
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDeinitializePcmStreamBase(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
) {
  SkInternalObjectBase* pAttachedObject;
  pAttachedObject = stream->_pNext;
  while (pAttachedObject) {
    if (pAttachedObject->_oType == SK_OBJECT_TYPE_LAYER
    &&  memcmp(pAttachedObject->_iUuid, stream->_iUuid, SK_UUID_SIZE) == 0
    ) {
      skFree(pAllocator, (void*)pAttachedObject->_vtable);
      skFree(pAllocator, pAttachedObject);
      break;
    }
    pAttachedObject = pAttachedObject->_pNext;
  }
}
