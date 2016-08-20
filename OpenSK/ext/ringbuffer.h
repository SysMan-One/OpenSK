/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Implementation of a ringbuffer for OpenSK extension library.
 ******************************************************************************/

#ifndef OPENSK_RINGBUFFER_H
#define OPENSK_RINGBUFFER_H 1

#include <OpenSK/opensk.h>

SK_DEFINE_HANDLE(SkRingBuffer);

SKAPI_ATTR SkResult SKAPI_CALL skRingBufferCreate(
  SkRingBuffer*                 pRingBuffer,
  size_t                        size,
  const SkAllocationCallbacks*  pAllocator
);

SKAPI_ATTR void SKAPI_CALL skRingBufferDestroy(
  SkRingBuffer                  ringBuffer,
  const SkAllocationCallbacks*  pAllocator
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextWriteLocation(
  SkRingBuffer                  ringBuffer,
  void**                        pLocation
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextReadLocation(
  SkRingBuffer                  ringBuffer,
  void**                        pLocation
);

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceWriteLocation(
  SkRingBuffer                  ringBuffer,
  size_t                        byteAdvance
);

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceReadLocation(
  SkRingBuffer                  ringBuffer,
  size_t                        byteAdvance
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWrite(
  SkRingBuffer                  ringBuffer,
  void*                         pData,
  size_t                        length
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferRead(
  SkRingBuffer                  ringBuffer,
  void*                         pData,
  size_t                        length
);

SKAPI_ATTR void SKAPI_CALL skRingBufferClear(
  SkRingBuffer                  ringBuffer
);

SKAPI_ATTR void* SKAPI_CALL skGetRingBufferCapacityBegin(
  SkRingBuffer                  ringBuffer
);

SKAPI_ATTR void* SKAPI_CALL skGetRingBufferCapacityEnd(
  SkRingBuffer                  ringBuffer
);

SKAPI_ATTR void* SKAPI_CALL skGetRingBufferDataBegin(
  SkRingBuffer                  ringBuffer
);

SKAPI_ATTR void* SKAPI_CALL skGetRingBufferDataEnd(
  SkRingBuffer                  ringBuffer
);

#endif //OPENSK_RINGBUFFER_H
