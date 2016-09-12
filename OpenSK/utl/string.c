/*******************************************************************************
 * OpenSK (utility) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A set of string utility functions (for dynamic and C-Strings).
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/macros.h>
#include <OpenSK/utl/string.h>

// C99
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

typedef struct SkStringUTL_T {
  SkAllocationCallbacks const*          pAllocator;
  char*                                 pBegin;
  char*                                 pEnd;
  size_t                                capacity;
} SkStringUTL_T;

////////////////////////////////////////////////////////////////////////////////
// String Functions (General)
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkBool32 SKAPI_CALL _skCheckParamUTL(
  char const*                           string,
  ...
) {
  SkBool32 passing;
  va_list ap;
  char const* cmp;

  passing = SK_FALSE;
  va_start(ap, string);
  cmp = va_arg(ap, char const*);
  while (cmp != NULL)
  {
    if (skCStrCompareUTL(string, cmp)) {
      passing = SK_TRUE;
      break;
    }
    cmp = va_arg(ap, char const*);
  }
  va_end(ap);

  return passing;
}

SKAPI_ATTR SkBool32 SKAPI_CALL _skCheckParamBeginsUTL(
  char const*                           string,
  ...
) {
  SkBool32 passing;
  va_list ap;
  char const *cmp;

  passing = SK_FALSE;
  va_start(ap, string);
  cmp = va_arg(ap, char const*);
  while (cmp != NULL)
  {
    if (skCStrCompareBeginsUTL(string, cmp)) {
      passing = SK_TRUE;
      break;
    }
    cmp = va_arg(ap, char const*);
  }
  va_end(ap);

  return passing;
}

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareUTL(
  char const*                           lhsString,
  char const*                           rhsString
) {
  while (*lhsString && *rhsString && *lhsString == *rhsString) {
    ++lhsString;
    ++rhsString;
  }
  return (*lhsString == *rhsString) ? SK_TRUE : SK_FALSE;
}

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareBeginsUTL(
  char const*                           string,
  char const*                           startsWith
) {
  while (*startsWith && *string == *startsWith) {
    ++string;
    ++startsWith;
  }
  return (*startsWith == '\0') ? SK_TRUE : SK_FALSE;
}

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareCaseInsensitiveUTL(
  char const*                           lhsString,
  char const*                           rhsString
) {
  while (*lhsString && *rhsString && tolower(*lhsString) == tolower(*rhsString)) {
    ++lhsString;
    ++rhsString;
  }
  return (*lhsString == *rhsString) ? SK_TRUE : SK_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// String Functions (SkStringUTL)
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateStringUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkStringUTL*                          pString
) {
  SkStringUTL string;

  string = SKALLOC(pAllocator, sizeof(SkStringUTL_T), SK_SYSTEM_ALLOCATION_SCOPE_UTILITY);
  if (!string) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  string->pBegin = SKALLOC(pAllocator, SKUTL_STRING_DEFAULT_CAPACITY + 1, SK_SYSTEM_ALLOCATION_SCOPE_UTILITY);
  if (!string->pBegin) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  string->pEnd = string->pBegin;
  string->pAllocator = pAllocator;
  string->capacity = SKUTL_STRING_DEFAULT_CAPACITY;

  (*pString) = string;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyStringUTL(
  SkStringUTL                           string
) {
  SKFREE(string->pAllocator, string->pBegin);
  SKFREE(string->pAllocator, string);
}

SKAPI_ATTR SkResult SKAPI_CALL skStringResizeUTL(
  SkStringUTL                           string,
  size_t                                newSize
) {
  size_t copySize;
  char *newString;

  // Early-Out: String capacity already is allocated.
  if (newSize <= string->capacity) {
    if (newSize < skStringLengthUTL(string)) {
      skStringDataUTL(string)[newSize] = 0;
      string->pEnd = string->pBegin + newSize;
    }
    return SK_SUCCESS;
  }

  // Resize the string if the capacity is less than the requested size
  newString = SKALLOC(string->pAllocator, newSize + 1, SK_SYSTEM_ALLOCATION_SCOPE_UTILITY);
  if (!newString) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy the old string to the new location
  copySize = skStringLengthUTL(string);
  copySize = SKMIN(newSize, copySize);
  memcpy(newString, skStringDataUTL(string), copySize);
  newString[copySize] = 0;
  SKFREE(string->pAllocator, string->pBegin);

  // Update string metadata for dynamic allocations
  string->pBegin = newString;
  string->pEnd = newString + copySize;
  string->capacity = newSize;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringReserveUTL(
  SkStringUTL                           string,
  size_t                                reservedSize
) {
  if (string->capacity < reservedSize) {
    return skStringResizeUTL(string, reservedSize);
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringNCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
) {
  SkResult result;

  // Make sure there is enough space in the string
  if (sourceLength > string->capacity) {
    result = skStringResizeUTL(string, sourceLength);
    if (result != SK_SUCCESS) {
      return result;
    }
  }

  // Copy data into the internal buffer (adjust pEnd if needed)
  string->pEnd = string->pBegin;
  while (*sourceString && sourceLength) {
    *string->pEnd = *sourceString;
    ++string->pEnd; ++sourceString; --sourceLength;
  }
  *string->pEnd = 0;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString
) {
  return skStringNCopyUTL(string, sourceString, strlen(sourceString));
}

SKAPI_ATTR SkResult SKAPI_CALL skStringNAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
) {
  size_t stringLength;
  SkResult result;

  // Make sure there is enough space in the string
  stringLength = skStringLengthUTL(string);
  result = skStringReserveUTL(string, stringLength + sourceLength);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Copy data into the string from the source
  while (*sourceString && sourceLength) {
    *string->pEnd = *sourceString;
    ++string->pEnd; ++sourceString; --sourceLength;
  }
  *string->pEnd = 0;

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skStringAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString
) {
  return skStringNAppendUTL(string, sourceString, strlen(sourceString));
}

SKAPI_ATTR void SKAPI_CALL skStringSwapUTL(
  SkStringUTL                           stringA,
  SkStringUTL                           stringB
) {
  SkStringUTL_T stringC;
   stringC = *stringA;
  *stringA = *stringB;
  *stringB =  stringC;
}

SKAPI_ATTR char* SKAPI_CALL skStringDataUTL(
  SkStringUTL                           string
) {
  return string->pBegin;
}

SKAPI_ATTR size_t SKAPI_CALL skStringLengthUTL(
  SkStringUTL                           string
) {
  return (string->pEnd - string->pBegin);
}

SKAPI_ATTR size_t SKAPI_CALL skStringCapacityUTL(
  SkStringUTL                           string
) {
  return string->capacity;
}

SKAPI_ATTR SkBool32 SKAPI_CALL skStringEmptyUTL(
  SkStringUTL                           string
) {
  return (skStringLengthUTL(string) == 0) ? SK_TRUE : SK_FALSE;
}

SKAPI_ATTR void SKAPI_CALL skStringClearUTL(
  SkStringUTL                           string
) {
  string->pEnd = string->pBegin;
  *string->pBegin = 0;
}
