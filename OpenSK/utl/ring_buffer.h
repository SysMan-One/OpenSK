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
 * Implementation of a ringbuffer for OpenSK UTLension library.
 ******************************************************************************/
#ifndef   OPENSK_UTL_RINGBUFFER_H
#define   OPENSK_UTL_RINGBUFFER_H 1

#include <OpenSK/opensk.h>

#ifdef    __cplusplus
UTLern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkRingBufferUTL);

typedef struct SkRingBufferDebugInfoUTL {
  SkBool32                              dataIsWrapping;
  void const*                           pCapacityBegin;
  void const*                           pCapacityEnd;
  void const*                           pDataBegin;
  void const*                           pDataEnd;
} SkRingBufferDebugInfoUTL;

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreateRingBufferUTL(
  size_t                                bufferSize,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkRingBufferUTL*                      pRingBuffer
);

void SKAPI_CALL skDestroyRingBufferUTL(
  SkRingBufferUTL                       ringBuffer
);

void SKAPI_CALL skRingBufferDebugInfoUTL(
  SkRingBufferUTL                       ringBuffer,
  SkRingBufferDebugInfoUTL*             pDebugInfo
);

size_t SKAPI_CALL skRingBufferCapacityUTL(
  SkRingBufferUTL                       ringBuffer
);

size_t SKAPI_CALL skRingBufferWriteRemainingUTL(
  SkRingBufferUTL                       ringBuffer
);

size_t SKAPI_CALL skRingBufferWriteRemainingContiguousUTL(
  SkRingBufferUTL                       ringBuffer
);

size_t SKAPI_CALL skRingBufferReadRemainingUTL(
  SkRingBufferUTL                       ringBuffer
);

size_t SKAPI_CALL skRingBufferReadRemainingContiguousUTL(
  SkRingBufferUTL                       ringBuffer
);

size_t SKAPI_CALL skRingBufferNUTLWriteLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  void**                                pLocation
);

size_t SKAPI_CALL skRingBufferNUTLReadLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  void**                                pLocation
);

void SKAPI_CALL skRingBufferAdvanceWriteLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  size_t                                byteAdvance
);

void SKAPI_CALL skRingBufferAdvanceReadLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  size_t                                byteAdvance
);

size_t SKAPI_CALL skRingBufferWriteUTL(
  SkRingBufferUTL                       ringBuffer,
  void*                                 pData,
  size_t                                length
);

size_t SKAPI_CALL skRingBufferReadUTL(
  SkRingBufferUTL                       ringBuffer,
  void*                                 pData,
  size_t                                length
);

void SKAPI_CALL skRingBufferClearUTL(
  SkRingBufferUTL                       ringBuffer
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //OPENSK_UTL_RINGBUFFER_H
