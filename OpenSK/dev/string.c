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

#include <OpenSK/dev/string.h>
#include <OpenSK/ext/sk_global.h>

#include <ctype.h>
#include <string.h>

SK_DEFINE_VECTOR_PUSH_METHOD_IMPL(StringVector, char*)

int8_t SKAPI_CALL skParseHexadecimal(
  int                                   value
) {
  value = toupper(value);
  if (value >= '0' && value <= '9') {
    return (int8_t)(value - '0');
  }
  else if (value >= 'A' && value <= 'F') {
    return (int8_t)(value - 'A' + 10);
  }
  return -1;
}

char const* skParseHexadecimalUint8(
  char const*                           pHexadecimalString,
  uint8_t*                              pDestination
) {
  int8_t _hexIdx;
  int8_t _hexCharValue;
  *pDestination = 0;
  for (_hexIdx = 0; _hexIdx < 2; ++_hexIdx) {
    *pDestination *= 0x10;
    _hexCharValue = skParseHexadecimal(*pHexadecimalString);
    if (_hexCharValue == -1) {
      return NULL;
    }
    *pDestination += (uint8_t)_hexCharValue;
    ++pHexadecimalString;
  }
  return pHexadecimalString;
}

int SKAPI_CALL skCompareCStringCaseInsensitive(
  char const*                           lhs,
  char const*                           rhs
) {
  while (*lhs && *rhs) {
    if (tolower(*lhs) != tolower(*rhs)) {
      return -1;
    }
    ++lhs;
    ++rhs;
  }
  if (tolower(*lhs) != tolower(*rhs)) {
    return 1;
  }
  return 0;
}

char* SKAPI_CALL skDuplicateCString(
  char const*                           string,
  SkAllocationCallbacks const*          pAllocator
) {
  char* pChar;
  size_t stringLength;

  // Duplicate the string (if possible)
  stringLength = strlen(string);
  pChar = skAllocate(
    pAllocator,
    stringLength + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pChar) {
    return NULL;
  }

  strcpy(pChar, string);
  return pChar;
}

char const* SKAPI_CALL skSearchCString(
  char const*                           string,
  char const*                           pChars,
  int                                   charCount
) {
  char const* foundLocation;
  while (charCount) {
    foundLocation = strchr(string, *pChars);
    if (foundLocation) return foundLocation;
    --charCount;
    ++pChars;
  }
  return NULL;
}

SkBool32 SKAPI_CALL skCStringEndsWith(
  char const*                           pString,
  char const*                           pEndsWith
) {
  char const* pStringEnd;
  char const* pEndsWithEnd;

  // Get the end pointers
  pStringEnd = pString;
  pEndsWithEnd = pEndsWith;
  while (*pStringEnd) ++pStringEnd;
  while (*pEndsWithEnd) ++pEndsWithEnd;

  // Early-out (if the end string's length > test string's length,
  // then it is impossible for this function to ever return true.)
  if (pStringEnd - pString < pEndsWithEnd - pEndsWith) {
    return SK_FALSE;
  }

  // Move backwards through the string until we hit an end case.
  // Notice that we first go back one character, this is okay since we are at
  // the null-terminator when we enter into this section.
  //
  // Case 1: We have covered the length of the "ends with" string.
  // Case 2: We find a non-matching character.
  while (pEndsWith != pEndsWithEnd) {
    --pStringEnd;
    --pEndsWithEnd;
    if (*pEndsWithEnd != *pStringEnd) {
      return SK_FALSE;
    }
  }

  return SK_TRUE;
}

SkBool32 SKAPI_CALL skStringVectorContainsIMPL(
  SkStringVectorIMPL const              vector,
  char const*                           pString
) {
  uint32_t idx;

  for (idx = 0; idx < vector->count; ++idx) {
    if (strcmp(vector->pData[idx], pString) == 0) {
      return SK_TRUE;
    }
  }

  return SK_FALSE;
}

SkResult SKAPI_CALL skPushStringVectorUniqueIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkStringVectorIMPL                    vector,
  char*                                 pValue
) {
  SkResult result;
  char* pNewValue;

  // If the search path already exists, skip adding this.
  if (skStringVectorContainsIMPL(vector, pValue)) {
    return SK_SUCCESS;
  }

  // Add the search path to the vector
  result = skPushStringVectorIMPL(
    pAllocator,
    vector,
    &pNewValue
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, pNewValue);
    return result;
  }

  return result;
}

SkResult SKAPI_CALL skStringVectorDeserializeIMPL(
  SkStringVectorIMPL                    stringVector,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pStringList,
  char const*                           pDelimiters,
  int                                   delimiterCount
) {
  char* pBuffer;
  SkResult result;
  size_t stringLength;
  char const* pNextString;

  while (pStringList) {

    // Find the end of the string, and then allocate space for it.
    pNextString = skSearchCString(pStringList, pDelimiters, delimiterCount);

    // Make a clone of the environment path.
    stringLength = pNextString - pStringList;
    pBuffer = skAllocate(
      pAllocator,
      stringLength + 1,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pBuffer) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Copy the string information, and then add it to the system paths.
    strncpy(pBuffer, pStringList, stringLength);
    pBuffer[stringLength] = 0;
    result = skPushStringVectorIMPL(
      pAllocator,
      stringVector,
      &pBuffer
    );
    if (result != SK_SUCCESS) {
      skFree(pAllocator, pBuffer);
      return result;
    }

    // Move our pointer to the next string.
    pStringList = pNextString;
    if (!*pStringList) {
      break;
    }
    ++pStringList;
  }

  return SK_SUCCESS;
}
