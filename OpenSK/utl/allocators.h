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
 * A set of helpful OpenSK allocators for utility purposes.
 ******************************************************************************/
#ifndef   OPENSK_UTL_ALLOCATORS_H
#define   OPENSK_UTL_ALLOCATORS_H 1

#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Debug Allocator
//------------------------------------------------------------------------------
// This allocator will attempt to error-check all of the memory usage within the
// context of OpenSK allocations. It attempts to handle the following cases:
// 1. Maintaining allocation statistics.
// 2. Double-Frees
// 3. Buffer over/underruns.
////////////////////////////////////////////////////////////////////////////////
typedef struct SkDebugAllocatorCreateInfoUTL {
  uint32_t                              underrunBufferSize;
  uint32_t                              overrunBufferSize;
} SkDebugAllocatorCreateInfoUTL;
typedef SkAllocationCallbacks* SkDebugAllocatorUTL;

SkResult SKAPI_CALL skCreateDebugAllocatorUTL(
  SkDebugAllocatorCreateInfoUTL*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDebugAllocatorUTL*                  pDebugAllocator
);

uint32_t SKAPI_CALL skDestroyDebugAllocatorUTL(
  SkDebugAllocatorUTL                   debugAllocator,
  SkAllocationCallbacks const*          pAllocator
);

void SKAPI_CALL skPrintDebugAllocatorStatisticsUTL(
  SkDebugAllocatorUTL                   debugAllocator
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_UTL_ALLOCATORS_H
