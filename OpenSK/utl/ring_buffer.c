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

// OpenSK
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/utl/ring_buffer.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Defines
////////////////////////////////////////////////////////////////////////////////

typedef struct SkRingBufferUTL_T {
  SkAllocationCallbacks const*          pAllocator;
  SkBool32                              dataWrap;
  size_t                                capacity;
  char*                                 capacityBegin;
  char*                                 capacityEnd;
  char*                                 dataBegin;
  char*                                 dataEnd;
} SkRingBufferUTL_T;

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Functions (IMPL)
////////////////////////////////////////////////////////////////////////////////

#define skRingBufferWriteRemainingContiguousIMPL(ringBuffer)                  \
((ringBuffer->dataWrap) ?                                                     \
  (ringBuffer->dataBegin   - ringBuffer->dataEnd)                             \
: (ringBuffer->capacityEnd - ringBuffer->dataEnd))

#define skRingBufferReadRemainingContiguousIMPL(ringBuffer)                   \
((ringBuffer->dataWrap) ?                                                     \
  (ringBuffer->capacityEnd - ringBuffer->dataBegin)                           \
: (ringBuffer->dataEnd     - ringBuffer->dataBegin))

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreateRingBufferUTL(
  size_t                                bufferSize,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkRingBufferUTL*                      pRingBuffer
) {
  SkRingBufferUTL ringBuffer;

  // Allocate the ring buffer object, along with it's data.
  ringBuffer = skAllocate(
    pAllocator,
    sizeof(SkRingBufferUTL_T) + bufferSize,
    1,
    allocationScope
  );
  if (!ringBuffer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  ringBuffer->pAllocator = pAllocator;
  ringBuffer->dataWrap = SK_FALSE;
  ringBuffer->capacity = bufferSize;
  ringBuffer->capacityBegin = ringBuffer->dataBegin = ringBuffer->dataEnd = (char*)&ringBuffer[1];
  ringBuffer->capacityEnd = ringBuffer->capacityBegin + bufferSize;
  (*pRingBuffer) = ringBuffer;

  return SK_SUCCESS;
}

void SKAPI_CALL skDestroyRingBufferUTL(
  SkRingBufferUTL                       ringBuffer
) {
  skFree(ringBuffer->pAllocator, ringBuffer);
}

void SKAPI_CALL skRingBufferDebugInfoUTL(
  SkRingBufferUTL                       ringBuffer,
  SkRingBufferDebugInfoUTL*             pDebugInfo
) {
  pDebugInfo->dataIsWrapping = ringBuffer->dataWrap;
  pDebugInfo->pCapacityBegin = ringBuffer->capacityBegin;
  pDebugInfo->pCapacityEnd = ringBuffer->capacityEnd;
  pDebugInfo->pDataBegin = ringBuffer->dataBegin;
  pDebugInfo->pDataEnd = ringBuffer->dataEnd;
}

size_t SKAPI_CALL skRingBufferCapacityUTL(
  SkRingBufferUTL                       ringBuffer
) {
  return ringBuffer->capacity;
}

size_t SKAPI_CALL skRingBufferWriteRemainingUTL(
  SkRingBufferUTL                       ringBuffer
) {
  if (ringBuffer->dataWrap) {
    return (ringBuffer->dataBegin - ringBuffer->dataEnd);
  }
  return ringBuffer->capacity - (ringBuffer->dataEnd - ringBuffer->dataBegin);
}

size_t SKAPI_CALL skRingBufferWriteRemainingContiguousUTL(
  SkRingBufferUTL                       ringBuffer
) {
  return skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
}

size_t SKAPI_CALL skRingBufferReadRemainingUTL(
  SkRingBufferUTL                       ringBuffer
) {
  if (ringBuffer->dataWrap) {
    return ringBuffer->capacity - (ringBuffer->dataBegin - ringBuffer->dataEnd);
  }
  return (ringBuffer->dataEnd - ringBuffer->dataBegin);
}

size_t SKAPI_CALL skRingBufferReadRemainingContiguousUTL(
  SkRingBufferUTL                       ringBuffer
) {
  return skRingBufferReadRemainingContiguousIMPL(ringBuffer);
}

size_t SKAPI_CALL skRingBufferNUTLWriteLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  void**                                pLocation
) {
  (*pLocation) = ringBuffer->dataEnd;
  return skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
}

size_t SKAPI_CALL skRingBufferNUTLReadLocationUTL(
  SkRingBufferUTL ringBuffer,
  void**                                pLocation
) {
  (*pLocation) = ringBuffer->dataBegin;
  return skRingBufferReadRemainingContiguousIMPL(ringBuffer);
}

void SKAPI_CALL skRingBufferAdvanceWriteLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  size_t                                byteAdvance
) {
  ringBuffer->dataEnd += byteAdvance;
  if (ringBuffer->dataEnd >= ringBuffer->capacityEnd) {
    ringBuffer->dataWrap = SK_TRUE;
    ringBuffer->dataEnd -= ringBuffer->capacity;
  }
}

void SKAPI_CALL skRingBufferAdvanceReadLocationUTL(
  SkRingBufferUTL                       ringBuffer,
  size_t                                byteAdvance
) {
  ringBuffer->dataBegin += byteAdvance;
  if (ringBuffer->dataBegin >= ringBuffer->capacityEnd) {
    ringBuffer->dataWrap = SK_FALSE;
    ringBuffer->dataBegin -= ringBuffer->capacity;
  }
}

size_t SKAPI_CALL skRingBufferWriteUTL(
  SkRingBufferUTL                       ringBuffer,
  void*                                 pData,
  size_t                                length
) {
  size_t bytesAvailable = skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
  if (bytesAvailable < length) {
    length = bytesAvailable;
  }
  memcpy(ringBuffer->dataEnd, pData, length);
  skRingBufferAdvanceWriteLocationUTL(ringBuffer, length);
  return length;
}

size_t SKAPI_CALL skRingBufferReadUTL(
  SkRingBufferUTL                       ringBuffer,
  void*                                 pData,
  size_t                                length
) {
  size_t bytesAvailable = skRingBufferReadRemainingContiguousIMPL(ringBuffer);
  if (bytesAvailable < length) {
    length = bytesAvailable;
  }
  memcpy(pData, ringBuffer->dataBegin, length);
  skRingBufferAdvanceReadLocationUTL(ringBuffer, length);
  return length;
}

void SKAPI_CALL skRingBufferClearUTL(
  SkRingBufferUTL                       ringBuffer
) {
  ringBuffer->dataWrap = SK_FALSE;
  ringBuffer->dataBegin = ringBuffer->dataEnd = ringBuffer->capacityBegin;
}
