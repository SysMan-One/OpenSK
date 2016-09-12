/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Implementation of a ringbuffer for OpenSK extension library.
 ******************************************************************************/

// OpenSK
#include <OpenSK/ext/ring_buffer.h>
#include <OpenSK/dev/macros.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Defines
////////////////////////////////////////////////////////////////////////////////

typedef struct SkRingBufferEXT_T {
  SkAllocationCallbacks const*          pAllocator;
  SkBool32                              dataWrap;
  size_t                                capacity;
  char*                                 capacityBegin;
  char*                                 capacityEnd;
  char*                                 dataBegin;
  char*                                 dataEnd;
} SkRingBufferEXT_T;

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

SKAPI_ATTR SkResult SKAPI_CALL skCreateRingBufferEXT(
  SkAllocationCallbacks const*          pAllocator,
  SkRingBufferEXT*                      pRingBuffer,
  size_t                                size
) {
  SkRingBufferEXT ringBuffer = SKALLOC(pAllocator, sizeof(SkRingBufferEXT_T) + size, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!ringBuffer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  ringBuffer->pAllocator = pAllocator;
  ringBuffer->dataWrap = SK_FALSE;
  ringBuffer->capacity = size;
  ringBuffer->capacityBegin = ringBuffer->dataBegin = ringBuffer->dataEnd = (char*)&ringBuffer[1];
  ringBuffer->capacityEnd = ringBuffer->capacityBegin + size;
  (*pRingBuffer) = ringBuffer;

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyRingBufferEXT(
  SkRingBufferEXT                       ringBuffer
) {
  SKFREE(ringBuffer->pAllocator, ringBuffer);
}

SKAPI_ATTR void SKAPI_CALL skRingBufferDebugInfoEXT(
  SkRingBufferEXT                       ringBuffer,
  SkRingBufferDebugInfoEXT*             pDebugInfo
) {
  pDebugInfo->dataIsWrapping = ringBuffer->dataWrap;
  pDebugInfo->pCapacityBegin = ringBuffer->capacityBegin;
  pDebugInfo->pCapacityEnd = ringBuffer->capacityEnd;
  pDebugInfo->pDataBegin = ringBuffer->dataBegin;
  pDebugInfo->pDataEnd = ringBuffer->dataEnd;
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferCapacityEXT(
  SkRingBufferEXT                       ringBuffer
) {
  return ringBuffer->capacity;
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteRemainingEXT(
  SkRingBufferEXT                       ringBuffer
) {
  if (ringBuffer->dataWrap) {
    return (ringBuffer->dataBegin - ringBuffer->dataEnd);
  }
  return ringBuffer->capacity - (ringBuffer->dataEnd - ringBuffer->dataBegin);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteRemainingContiguousEXT(
  SkRingBufferEXT                       ringBuffer
) {
  return skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadRemainingEXT(
  SkRingBufferEXT                       ringBuffer
) {
  if (ringBuffer->dataWrap) {
    return ringBuffer->capacity - (ringBuffer->dataBegin - ringBuffer->dataEnd);
  }
  return (ringBuffer->dataEnd - ringBuffer->dataBegin);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadRemainingContiguousEXT(
  SkRingBufferEXT                       ringBuffer
) {
  return skRingBufferReadRemainingContiguousIMPL(ringBuffer);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextWriteLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  void**                                pLocation
) {
  (*pLocation) = ringBuffer->dataEnd;
  return skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextReadLocationEXT(
  SkRingBufferEXT ringBuffer,
  void**                                pLocation
) {
  (*pLocation) = ringBuffer->dataBegin;
  return skRingBufferReadRemainingContiguousIMPL(ringBuffer);
}

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceWriteLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  size_t                                byteAdvance
) {
  ringBuffer->dataEnd += byteAdvance;
  if (ringBuffer->dataEnd >= ringBuffer->capacityEnd) {
    ringBuffer->dataWrap = SK_TRUE;
    ringBuffer->dataEnd -= ringBuffer->capacity;
  }
}

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceReadLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  size_t                                byteAdvance
) {
  ringBuffer->dataBegin += byteAdvance;
  if (ringBuffer->dataBegin >= ringBuffer->capacityEnd) {
    ringBuffer->dataWrap = SK_FALSE;
    ringBuffer->dataBegin -= ringBuffer->capacity;
  }
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteEXT(
  SkRingBufferEXT                       ringBuffer,
  void*                                 pData,
  size_t                                length
) {
  size_t bytesAvailable = skRingBufferWriteRemainingContiguousIMPL(ringBuffer);
  if (bytesAvailable < length) {
    length = bytesAvailable;
  }
  memcpy(ringBuffer->dataEnd, pData, length);
  skRingBufferAdvanceWriteLocationEXT(ringBuffer, length);
  return length;
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadEXT(
  SkRingBufferEXT                       ringBuffer,
  void*                                 pData,
  size_t                                length
) {
  size_t bytesAvailable = skRingBufferReadRemainingContiguousIMPL(ringBuffer);
  if (bytesAvailable < length) {
    length = bytesAvailable;
  }
  memcpy(pData, ringBuffer->dataBegin, length);
  skRingBufferAdvanceReadLocationEXT(ringBuffer, length);
  return length;
}

SKAPI_ATTR void SKAPI_CALL skRingBufferClearEXT(
  SkRingBufferEXT                       ringBuffer
) {
  ringBuffer->dataWrap = SK_FALSE;
  ringBuffer->dataBegin = ringBuffer->dataEnd = ringBuffer->capacityBegin;
}
