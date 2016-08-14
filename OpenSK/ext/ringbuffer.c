/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Implementation of a ringbuffer for OpenSK extension library.
 ******************************************************************************/

#include "ringbuffer.h"
#include <OpenSK/dev/macros.h>
#include <string.h>

typedef struct SkRingBuffer_T {
  void* capacityBegin;
  void* capacityEnd;
  void* dataBegin;
  void* dataEnd;
} SkRingBuffer_T;

SKAPI_ATTR SkResult SKAPI_CALL skRingBufferCreate(
  SkRingBuffer*                 pRingBuffer,
  size_t                        size,
  const SkAllocationCallbacks*  pAllocator
) {
  if (!pAllocator) pAllocator = &SkDefaultAllocationCallbacks;
  SkRingBuffer ringBuffer = SKALLOC(pAllocator, sizeof(SkRingBuffer_T) + size, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!ringBuffer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  ringBuffer->capacityBegin = ringBuffer->dataBegin = ringBuffer->dataEnd = &ringBuffer[1];
  ringBuffer->capacityEnd = ringBuffer->capacityBegin + size;
  (*pRingBuffer) = ringBuffer;

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skRingBufferDestroy(
  SkRingBuffer                  ringBuffer,
  const SkAllocationCallbacks*  pAllocator
) {
  if (!pAllocator) pAllocator = &SkDefaultAllocationCallbacks;
  SKFREE(pAllocator, ringBuffer);
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextWriteLocation(
  SkRingBuffer                  ringBuffer,
  void**                        pLocation
) {
  (*pLocation) = ringBuffer->dataEnd;
  if (ringBuffer->dataEnd >= ringBuffer->dataBegin) {
    if (ringBuffer->dataBegin == ringBuffer->capacityBegin) {
      return ringBuffer->capacityEnd - ringBuffer->dataEnd - 1;
    }
    return ringBuffer->capacityEnd - ringBuffer->dataEnd;
  }
  return ringBuffer->dataBegin - ringBuffer->dataEnd - 1;
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextReadLocation(
  SkRingBuffer                  ringBuffer,
  void**                        pLocation
) {
  (*pLocation) = ringBuffer->dataBegin;
  if (ringBuffer->dataEnd >= ringBuffer->dataBegin) {
    return ringBuffer->dataEnd - ringBuffer->dataBegin;
  }
  return ringBuffer->capacityEnd - ringBuffer->dataBegin;
}

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceWriteLocation(
  SkRingBuffer                  ringBuffer,
  size_t                        byteAdvance
) {
  ringBuffer->dataEnd += byteAdvance;
  if (ringBuffer->dataEnd >= ringBuffer->capacityEnd) {
    ringBuffer->dataEnd -= ringBuffer->capacityEnd - ringBuffer->capacityBegin;
  }
}

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceReadLocation(
  SkRingBuffer                  ringBuffer,
  size_t                        byteAdvance
) {
  ringBuffer->dataBegin += byteAdvance;
  if (ringBuffer->dataBegin >= ringBuffer->capacityEnd) {
    ringBuffer->dataBegin -= ringBuffer->capacityEnd - ringBuffer->capacityBegin;
  }
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWrite(
  SkRingBuffer                  ringBuffer,
  void*                         pData,
  size_t                        length
) {
  void* pLocation;
  size_t bytesAvailable;
  size_t bytesWritten = 0;

  // Write into the ring buffer until it's full or we're out of data
  do {
    bytesAvailable = skRingBufferNextWriteLocation(ringBuffer, &pLocation);
    if (bytesAvailable >= length) {
      bytesAvailable = length;
    }
    bytesWritten += bytesAvailable;
    memcpy(pLocation, pData, bytesAvailable);
    skRingBufferAdvanceWriteLocation(ringBuffer, bytesAvailable);
  } while (bytesAvailable != 0 && bytesWritten != length);

  return bytesWritten;
}

SKAPI_ATTR size_t SKAPI_CALL skRingBufferRead(
  SkRingBuffer                  ringBuffer,
  void*                         pData,
  size_t                        length
) {
  void* pLocation;
  size_t bytesAvailable;
  size_t bytesRead = 0;

  // Read from the ring buffer until we're full or it's out of data
  do {
    bytesAvailable = skRingBufferNextReadLocation(ringBuffer, &pLocation);
    if (bytesAvailable >= length) {
      bytesAvailable = length;
    }
    bytesRead += bytesAvailable;
    memcpy(pData, pLocation, bytesAvailable);
    skRingBufferAdvanceWriteLocation(ringBuffer, bytesAvailable);
  } while (bytesAvailable != 0 && bytesRead != length);

  return bytesRead;
}

SKAPI_ATTR void SKAPI_CALL skRingBufferClear(
  SkRingBuffer                  ringBuffer
) {
  ringBuffer->dataBegin = ringBuffer->dataEnd = ringBuffer->capacityBegin;
}
