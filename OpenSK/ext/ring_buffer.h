/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Implementation of a ringbuffer for OpenSK extension library.
 ******************************************************************************/
#ifndef   OPENSK_EXT_RINGBUFFER_H
#define   OPENSK_EXT_RINGBUFFER_H 1

#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkRingBufferEXT);

typedef struct SkRingBufferDebugInfoEXT {
  SkBool32                              dataIsWrapping;
  void const*                           pCapacityBegin;
  void const*                           pCapacityEnd;
  void const*                           pDataBegin;
  void const*                           pDataEnd;
} SkRingBufferDebugInfoEXT;

////////////////////////////////////////////////////////////////////////////////
// Ring Buffer Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateRingBufferEXT(
  SkAllocationCallbacks const*          pAllocator,
  SkRingBufferEXT*                      pRingBuffer,
  size_t                                size
);

SKAPI_ATTR void SKAPI_CALL skDestroyRingBufferEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR void SKAPI_CALL skRingBufferDebugInfoEXT(
  SkRingBufferEXT                       ringBuffer,
  SkRingBufferDebugInfoEXT*             pDebugInfo
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferCapacityEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteRemainingEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteRemainingContiguousEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadRemainingEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadRemainingContiguousEXT(
  SkRingBufferEXT                       ringBuffer
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextWriteLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  void**                                pLocation
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferNextReadLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  void**                                pLocation
);

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceWriteLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  size_t                                byteAdvance
);

SKAPI_ATTR void SKAPI_CALL skRingBufferAdvanceReadLocationEXT(
  SkRingBufferEXT                       ringBuffer,
  size_t                                byteAdvance
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferWriteEXT(
  SkRingBufferEXT                       ringBuffer,
  void*                                 pData,
  size_t                                length
);

SKAPI_ATTR size_t SKAPI_CALL skRingBufferReadEXT(
  SkRingBufferEXT                       ringBuffer,
  void*                                 pData,
  size_t                                length
);

SKAPI_ATTR void SKAPI_CALL skRingBufferClearEXT(
  SkRingBufferEXT                       ringBuffer
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif //OPENSK_EXT_RINGBUFFER_H
