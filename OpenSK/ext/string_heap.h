/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A type which holds individual strings in a quick-to-access manner.
 ******************************************************************************/
#ifndef   OPENSK_EXT_STRING_HEAP_H
#define   OPENSK_EXT_STRING_HEAP_H 1

// OpenSK
#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// String Heap Defines
////////////////////////////////////////////////////////////////////////////////

#define SKEXT_STRING_HEAP_ELEMENT_OVERHEAD                  (sizeof(size_t) + 1)
#define SKEXT_STRING_HEAP_MINIMUM_CAPACITY                                     1

SK_DEFINE_HANDLE(SkStringHeapEXT);

////////////////////////////////////////////////////////////////////////////////
// String Heap Functions
////////////////////////////////////////////////////////////////////////////////
// TODO: Redesign this; right now it isn't a heap, but a contiguous string.

SKAPI_ATTR SkResult SKAPI_CALL skCreateStringHeapEXT(
  SkAllocationCallbacks const*      pAllocator,
  SkStringHeapEXT*                  pStringHeap,
  size_t                            hintSize
);

SKAPI_ATTR void SKAPI_CALL skDestroyStringHeapEXT(
  SkStringHeapEXT                   stringHeap
);

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapReserveRemainingEXT(
  SkStringHeapEXT                   stringHeap,
  size_t                            requiredCapacity,
  size_t                            requiredElements
);

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapReserveEXT(
  SkStringHeapEXT                   stringHeap,
  size_t                            newCapacity
);

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCapacityEXT(
  SkStringHeapEXT                   stringHeap
);

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCapacityRemainingEXT(
  SkStringHeapEXT                   stringHeap
);

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCountEXT(
  SkStringHeapEXT                   stringHeap
);

SKAPI_ATTR char const* SKAPI_CALL skStringHeapContainsEXT(
  SkStringHeapEXT                   stringHeap,
  char const*                       pString
);

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapNAddEXT(
  SkStringHeapEXT                   stringHeap,
  char const*                       pString,
  size_t                            stringLength
);

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapAddEXT(
  SkStringHeapEXT                   stringHeap,
  char const*                       pString
);

SKAPI_ATTR char const* SKAPI_CALL skStringHeapAtEXT(
  SkStringHeapEXT                   stringHeap,
  size_t                            index
);

SKAPI_ATTR void SKAPI_CALL skStringHeapRemoveRangeEXT(
  SkStringHeapEXT                   stringHeap,
  size_t                            start,
  size_t                            end
);

SKAPI_ATTR void SKAPI_CALL skStringHeapRemoveAtEXT(
  SkStringHeapEXT                   stringHeap,
  size_t                            index
);

SKAPI_ATTR char const* SKAPI_CALL skStringHeapNextEXT(
  SkStringHeapEXT                   stringHeap,
  char const*                       pCurrent
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_STRING_HEAP_H
