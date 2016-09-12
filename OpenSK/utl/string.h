/*******************************************************************************
 * OpenSK (utility) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A set of string utility functions (for dynamic and C-Strings).
 ******************************************************************************/
#ifndef   OPENSK_UTL_STRING_H
#define   OPENSK_UTL_STRING_H 1

// OpenSK
#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// String Defines
////////////////////////////////////////////////////////////////////////////////

#define SKUTL_STRING_DEFAULT_CAPACITY                                         16
SK_DEFINE_HANDLE(SkStringUTL);

////////////////////////////////////////////////////////////////////////////////
// String Functions (General)
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkBool32 SKAPI_CALL _skCheckParamUTL(
  char const*                           string,
  ...
);
#define skCheckParamUTL(param, ...) _skCheckParamUTL(param, __VA_ARGS__, NULL)

SKAPI_ATTR SkBool32 SKAPI_CALL _skCheckParamBeginsUTL(
  char const*                           string,
  ...
);
#define skCheckParamBeginsUTL(param, ...) _skCheckParamBeginsUTL(param, __VA_ARGS__, NULL)

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareUTL(
  char const*                           lhsString,
  char const*                           rhsString
);

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareBeginsUTL(
  char const*                           string,
  char const*                           startsWith
);

SKAPI_ATTR SkBool32 SKAPI_CALL skCStrCompareCaseInsensitiveUTL(
  char const*                           lhsString,
  char const*                           rhsString
);

////////////////////////////////////////////////////////////////////////////////
// String Functions (SkStringUTL)
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateStringUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkStringUTL*                          pString
);

SKAPI_ATTR void SKAPI_CALL skDestroyStringUTL(
  SkStringUTL                           string
);

SKAPI_ATTR SkResult SKAPI_CALL skStringResizeUTL(
  SkStringUTL                           string,
  size_t                                newSize
);

SKAPI_ATTR SkResult SKAPI_CALL skStringReserveUTL(
  SkStringUTL                           string,
  size_t                                reservedSize
);

SKAPI_ATTR SkResult SKAPI_CALL skStringNCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
);

SKAPI_ATTR SkResult SKAPI_CALL skStringCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString
);

SKAPI_ATTR SkResult SKAPI_CALL skStringNAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
);

SKAPI_ATTR SkResult SKAPI_CALL skStringAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString
);

SKAPI_ATTR void SKAPI_CALL skStringSwapUTL(
  SkStringUTL                           lhsString,
  SkStringUTL                           rhsString
);

SKAPI_ATTR char* SKAPI_CALL skStringDataUTL(
  SkStringUTL                           string
);

SKAPI_ATTR size_t SKAPI_CALL skStringLengthUTL(
  SkStringUTL                           string
);

SKAPI_ATTR size_t SKAPI_CALL skStringCapacityUTL(
  SkStringUTL                           string
);

SKAPI_ATTR SkBool32 SKAPI_CALL skStringEmptyUTL(
  SkStringUTL                           string
);

SKAPI_ATTR void SKAPI_CALL skStringClearUTL(
  SkStringUTL                           string
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_UTL_STRING_H
