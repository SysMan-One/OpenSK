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

#ifndef   OPENSK_DEV_VECTOR_H
#define   OPENSK_DEV_VECTOR_H 1

#include <OpenSK/opensk.h>

////////////////////////////////////////////////////////////////////////////////
// Vector Defines
////////////////////////////////////////////////////////////////////////////////

#define SK_DEFINE_VECTOR_IMPL(name, elementType)                                \
typedef struct Sk##name##IMPL_T {                                               \
  elementType*                          pData;                                  \
  uint32_t                              capacity;                               \
  uint32_t                              count;                                  \
  SkSystemAllocationScope               allocationScope;                        \
} Sk##name##IMPL_T, *Sk##name##IMPL

#define SK_DEFINE_VECTOR_GROW_METHOD_IMPL(name, elementType)                    \
SkResult SKAPI_CALL skGrow##name##IMPL(                                         \
  SkAllocationCallbacks const*          pAllocator,                             \
  Sk##name##IMPL                        vector                                  \
) {                                                                             \
  return skGrowVectorIMPL(                                                      \
    pAllocator,                                                                 \
    (SkVectorIMPL)vector,                                                       \
    sizeof(elementType)                                                         \
  );                                                                            \
}

#define SK_DEFINE_VECTOR_PUSH_METHOD_IMPL(name, elementType)                    \
SkResult SKAPI_CALL skPush##name##IMPL(                                         \
  SkAllocationCallbacks const*          pAllocator,                             \
  Sk##name##IMPL                        vector,                                 \
  elementType const*                    pValue                                  \
) {                                                                             \
  return skPushVectorIMPL(                                                      \
    pAllocator,                                                                 \
    (SkVectorIMPL)vector,                                                       \
    (void const*)pValue,                                                        \
    sizeof(elementType)                                                         \
  );                                                                            \
}

SK_DEFINE_VECTOR_IMPL(Vector, char);

////////////////////////////////////////////////////////////////////////////////
// Vector Functions
////////////////////////////////////////////////////////////////////////////////

extern SkResult SKAPI_CALL skGrowVectorIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkVectorIMPL                          pVector,
  size_t                                elementSize
);

extern SkResult SKAPI_CALL skPushVectorIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkVectorIMPL                          pVector,
  void const*                           pValue,
  size_t                                elementSize
);

#endif // OPENSK_DEV_VECTOR_H
