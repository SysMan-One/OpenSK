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

SkBool32 SKAPI_CALL _skCheckParamUTL(
  char const*                           string,
  ...
);
#define skCheckParamUTL(param, ...) _skCheckParamUTL(param, __VA_ARGS__, NULL)

SkBool32 SKAPI_CALL _skCheckParamBeginsUTL(
  char const*                           string,
  ...
);
#define skCheckParamBeginsUTL(param, ...) _skCheckParamBeginsUTL(param, __VA_ARGS__, NULL)

SkBool32 SKAPI_CALL _skCStrCompareEndsUTL(
  char const*                           string,
  ...
);
#define skCStrCompareEndsUTL(param, ...) _skCStrCompareEndsUTL(param, __VA_ARGS__, NULL)

SkBool32 SKAPI_CALL skCStrCompareUTL(
  char const*                           lhsString,
  char const*                           rhsString
);

SkBool32 SKAPI_CALL skCStrCompareBeginsUTL(
  char const*                           string,
  char const*                           startsWith
);

SkBool32 SKAPI_CALL skCStrCompareCaseInsensitiveUTL(
  char const*                           lhsString,
  char const*                           rhsString
);

void SKAPI_CALL skPrintUuidUTL(
  uint8_t                               uuid[SK_UUID_SIZE]
);

////////////////////////////////////////////////////////////////////////////////
// String Functions (SkStringUTL)
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreateStringUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkStringUTL*                          pString
);

void SKAPI_CALL skDestroyStringUTL(
  SkStringUTL                           string
);

SkResult SKAPI_CALL skStringResizeUTL(
  SkStringUTL                           string,
  size_t                                newSize
);

SkResult SKAPI_CALL skStringReserveUTL(
  SkStringUTL                           string,
  size_t                                reservedSize
);

SkResult SKAPI_CALL skStringNCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
);

SkResult SKAPI_CALL skStringCopyUTL(
  SkStringUTL                           string,
  char const*                           sourceString
);

SkResult SKAPI_CALL skStringNAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString,
  size_t                                sourceLength
);

SkResult SKAPI_CALL skStringAppendUTL(
  SkStringUTL                           string,
  char const*                           sourceString
);

void SKAPI_CALL skStringSwapUTL(
  SkStringUTL                           lhsString,
  SkStringUTL                           rhsString
);

char* SKAPI_CALL skStringDataUTL(
  SkStringUTL                           string
);

size_t SKAPI_CALL skStringLengthUTL(
  SkStringUTL                           string
);

size_t SKAPI_CALL skStringCapacityUTL(
  SkStringUTL                           string
);

SkBool32 SKAPI_CALL skStringEmptyUTL(
  SkStringUTL                           string
);

void SKAPI_CALL skStringClearUTL(
  SkStringUTL                           string
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_UTL_STRING_H
