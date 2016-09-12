/*******************************************************************************
 * OpenSK (extension) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A type which holds individual strings in a quick-to-access manner.
 ******************************************************************************/

// OpenSK
#include <OpenSK/ext/string_heap.h>
#include <OpenSK/dev/macros.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// String Heap Defines
////////////////////////////////////////////////////////////////////////////////

typedef struct SkStringHeapEXT_T {
  SkAllocationCallbacks const*          pAllocator;
  char*                                 pBegin;
  char*                                 pLast;
  char*                                 pEnd;
  size_t                                count;
} SkStringHeapEXT_T;

////////////////////////////////////////////////////////////////////////////////
// String Heap Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateStringHeapEXT(
  SkAllocationCallbacks const*          pAllocator,
  SkStringHeapEXT*                      pStringHeap,
  size_t                                hintSize
) {
  SkStringHeapEXT stringHeap;

  if (hintSize < SKEXT_STRING_HEAP_MINIMUM_CAPACITY) {
    hintSize = SKEXT_STRING_HEAP_MINIMUM_CAPACITY;
  }

  // Allocate the string heap
  stringHeap = SKALLOC(pAllocator, sizeof(SkStringHeapEXT_T), SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!stringHeap) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Allocate the internal data
  stringHeap->pBegin = SKALLOC(pAllocator, hintSize + SKEXT_STRING_HEAP_ELEMENT_OVERHEAD, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!stringHeap->pBegin) {
    SKFREE(pAllocator, stringHeap);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Configure the default structure properties
  stringHeap->pAllocator = pAllocator;
  stringHeap->pLast = stringHeap->pBegin;
  stringHeap->pEnd = stringHeap->pBegin + hintSize + SKEXT_STRING_HEAP_ELEMENT_OVERHEAD;
  stringHeap->count = 0;

  *pStringHeap = stringHeap;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyStringHeapEXT(
  SkStringHeapEXT                       stringHeap
) {
  SKFREE(stringHeap->pAllocator, stringHeap->pBegin);
  SKFREE(stringHeap->pAllocator, stringHeap);
}

static SkResult skStringHeapReserveRemainingIMPL(
  SkStringHeapEXT                       stringHeap,
  size_t                                newByteCapacity,
  size_t                                dataLength
) {
  char *newData;

  // Allocate the new data set
  newData = SKALLOC(stringHeap->pAllocator, newByteCapacity, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!newData) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy information and adjust pointers
  memcpy(newData, stringHeap->pBegin, dataLength);
  SKFREE(stringHeap->pAllocator, stringHeap->pBegin);
  stringHeap->pBegin = newData;
  stringHeap->pLast = newData + dataLength;
  stringHeap->pEnd = newData + newByteCapacity;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapReserveRemainingEXT(
  SkStringHeapEXT                       stringHeap,
  size_t                                requiredCapacity,
  size_t                                requiredElements
) {
  size_t dataLength;
  size_t newByteCapacity;

  // Early-Out: If we already have enough capacity, just return
  if (skStringHeapCapacityRemainingEXT(stringHeap) >= requiredCapacity) {
    return SK_SUCCESS;
  }

  // Allocate the new data set
  dataLength = (stringHeap->pLast - stringHeap->pBegin);
  newByteCapacity = dataLength + requiredCapacity + ((requiredElements + 1) * SKEXT_STRING_HEAP_ELEMENT_OVERHEAD);
  return skStringHeapReserveRemainingIMPL(stringHeap, newByteCapacity, dataLength);
}

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapReserveEXT(
  SkStringHeapEXT                       stringHeap,
  size_t                                newCapacity
) {
  size_t dataLength;
  size_t newByteCapacity;

  // Early-Out: If our capacity is already greater-than or equal, return
  if (skStringHeapCapacityEXT(stringHeap) >= newCapacity) {
    return SK_SUCCESS;
  }

  // Allocate the new data set
  dataLength = (stringHeap->pLast - stringHeap->pBegin);
  newByteCapacity = newCapacity + ((stringHeap->count + 1) * SKEXT_STRING_HEAP_ELEMENT_OVERHEAD);
  return skStringHeapReserveRemainingIMPL(stringHeap, newByteCapacity, dataLength);
}

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCapacityEXT(
  SkStringHeapEXT                       stringHeap
) {
  return (stringHeap->pEnd - stringHeap->pBegin) - ((stringHeap->count + 1) * SKEXT_STRING_HEAP_ELEMENT_OVERHEAD);
}

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCapacityRemainingEXT(
  SkStringHeapEXT                       stringHeap
) {
  return (stringHeap->pEnd - stringHeap->pLast) - SKEXT_STRING_HEAP_ELEMENT_OVERHEAD;
}

SKAPI_ATTR size_t SKAPI_CALL skStringHeapCountEXT(
  SkStringHeapEXT                       stringHeap
) {
  return stringHeap->count;
}

SKAPI_ATTR char const* SKAPI_CALL skStringHeapContainsEXT(
  SkStringHeapEXT                       stringHeap,
  char const*                           pString
) {
  char const* pElement = NULL;
  while ((pElement = skStringHeapNextEXT(stringHeap, pElement))) {
    if (strcmp(pString, pElement) == 0) {
      return pElement;
    }
  }
  return NULL;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapNAddEXT(
  SkStringHeapEXT                       stringHeap,
  char const*                           pString,
  size_t                                stringLength
) {
  char* origLast;
  SkResult result;

  // Make sure there is enough space to hold the string
  result = skStringHeapReserveRemainingEXT(stringHeap, stringLength, 1);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Copy data into the string heap
  origLast = stringHeap->pLast;
  stringHeap->pLast += sizeof(size_t);
  while (*pString && stringLength) {
    *stringHeap->pLast = *pString;
    ++stringHeap->pLast; ++pString; --stringLength;
  }
  *stringHeap->pLast = 0;
  ++stringHeap->pLast;
  *((size_t*)origLast) = ((size_t)(stringHeap->pLast - origLast - sizeof(size_t)));
  ++stringHeap->count;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringHeapAddEXT(
  SkStringHeapEXT                       stringHeap,
  char const*                           pString
) {
  return skStringHeapNAddEXT(stringHeap, pString, strlen(pString));
}

SKAPI_ATTR char const* SKAPI_CALL skStringHeapAtEXT(
  SkStringHeapEXT                       stringHeap,
  size_t                                index
) {
  char const* pData = NULL;
  while ((pData = skStringHeapNextEXT(stringHeap, pData))) {
    if (!index) {
      break;
    }
    --index;
  }
  return pData;
}

SKAPI_ATTR void SKAPI_CALL skStringHeapRemoveRangeEXT(
  SkStringHeapEXT                       stringHeap,
  size_t                                start,
  size_t                                end
) {
  char* pFrom;
  char* pTo;
  ++end; // Note: Inclusive range

  // Early-Out: If the start position is greater than the amount of elements,
  // then don't remove anything because the range is OoB.
  pFrom = (char*)skStringHeapAtEXT(stringHeap, start);
  if (!pFrom) {
    return;
  }

  // If the end is past the end, set it to the end
  pTo = (char*)skStringHeapAtEXT(stringHeap, end);
  if (!pTo) {
    pTo = stringHeap->pLast;
  }

  // Move data to fill the newly-formed gap, and correct counts
  while (pTo !=stringHeap->pLast) {
    *pFrom = *pTo;
    ++pFrom; ++pTo;
  }
  *pFrom = 0;
  stringHeap->count -= (end - start);
}

SKAPI_ATTR void SKAPI_CALL skStringHeapRemoveAtEXT(
  SkStringHeapEXT                       stringHeap,
  size_t                                index
) {
  skStringHeapRemoveRangeEXT(stringHeap, index, index);
}

SKAPI_ATTR char const* SKAPI_CALL skStringHeapNextEXT(
  SkStringHeapEXT                       stringHeap,
  char const*                           pCurrent
) {
  if (!pCurrent) {
    pCurrent = stringHeap->pBegin;
  }
  else {
    pCurrent += *((size_t*)(pCurrent - sizeof(size_t)));
  }

  if (pCurrent == stringHeap->pLast) {
    return NULL;
  }

  return pCurrent + sizeof(size_t);
}
