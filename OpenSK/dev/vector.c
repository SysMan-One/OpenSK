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
 * Generic vector support for dynamically-sized objects.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_global.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Vector Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skGrowVectorIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkVectorIMPL                          vector,
  size_t                                elementSize
) {

  // Grow arbitrarily by 2
  if (vector->capacity == 0) {
    vector->capacity = 1;
  }
  vector->capacity *= 2;

  // Reallocate as needed
  vector->pData = skReallocate(
    pAllocator,
    vector->pData,
    elementSize * vector->capacity,
    1,
    vector->allocationScope
  );
  if (!vector->pData) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  return SK_SUCCESS;
}

SkResult SKAPI_CALL skPushVectorIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkVectorIMPL                          vector,
  void const*                           pValue,
  size_t                                elementSize
) {
  SkResult result;

  // Grow, if needed
  if (vector->capacity == vector->count) {
    result = skGrowVectorIMPL(pAllocator, vector, elementSize);
    if (result != SK_SUCCESS) {
      return result;
    }
  }

  // Otherwise memcpy into the vector
  memcpy(vector->pData + (elementSize * vector->count), pValue, elementSize);
  ++vector->count;

  return SK_SUCCESS;
}
