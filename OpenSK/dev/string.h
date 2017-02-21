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
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_STRING_H
#define   OPENSK_DEV_STRING_H 1

#include <OpenSK/opensk.h>
#include <OpenSK/dev/vector.h>

SK_DEFINE_VECTOR_IMPL(StringVector, char*);

////////////////////////////////////////////////////////////////////////////////
// Internal Functions
////////////////////////////////////////////////////////////////////////////////

extern int8_t SKAPI_CALL skParseHexadecimal(
  int                                   value
);

extern char const* skParseHexadecimalUint8(
  char const*                           pHexadecimalString,
  uint8_t*                              pDestination
);

extern int SKAPI_CALL skCompareCStringCaseInsensitive(
  char const*                           lhs,
  char const*                           rhs
);

extern char* SKAPI_CALL skDuplicateCString(
  char const*                           string,
  SkAllocationCallbacks const*          pAllocator
);

extern char const* SKAPI_CALL skSearchCString(
  char const*                           string,
  char const*                           pChars,
  int                                   charCount
);

extern SkBool32 SKAPI_CALL skCStringEndsWith(
  char const*                           pString,
  char const*                           pEndsWith
);

extern SkResult SKAPI_CALL skPushStringVectorIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkStringVectorIMPL                    vector,
  char* const*                          pString
);

extern SkBool32 SKAPI_CALL skStringVectorContainsIMPL(
  SkStringVectorIMPL const              vector,
  char const*                           pString
);

extern SkResult SKAPI_CALL skPushStringVectorUniqueIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkStringVectorIMPL                    vector,
  char*                                 pValue
);

extern SkResult SKAPI_CALL skStringVectorDeserializeIMPL(
  SkStringVectorIMPL                    stringVector,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pStringList,
  char const*                           pDelimiters,
  int                                   delimiterCount
);

#endif // OPENSK_DEV_STRING_H
