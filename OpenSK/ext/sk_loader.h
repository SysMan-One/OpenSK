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
 * OpenSK extensibility header. (Will be present in final package.)
 ******************************************************************************/

#ifndef   OPENSK_EXT_LOADER_H
#define   OPENSK_EXT_LOADER_H 1

// OpenSK
#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Loader Definitions
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkLoader);

typedef enum SkLoaderEnumerateFlagBits {
  SK_LOADER_ENUMERATE_ALL_BIT = 0x00000001,
  SK_LOADER_ENUMERATE_IMPLICIT_BIT = 0x00000002,
  SK_LOADER_ENUMERATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} SkLoaderEnumerateFlagBits;
typedef SkFlags SkLoaderEnumerateFlags;

////////////////////////////////////////////////////////////////////////////////
// Loader Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skSetLoaderAllocator(
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeLoader();

SKAPI_ATTR void SKAPI_CALL skDeinitializeLoader();

SKAPI_ATTR SkResult SKAPI_CALL skInitializeLayerCreateInfo(
  char const*                           pLayerName,
  uint32_t*                             pCreateInfos,
  SkLayerCreateInfo*                    pCreateInfo
);

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverCreateInfo(
  char const*                           pDriverName,
  uint32_t*                             pCreateInfos,
  SkDriverCreateInfo*                   pCreateInfo
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLoaderLayerProperties(
  SkLoaderEnumerateFlags                enumerateFlags,
  uint32_t*                             pLayerCount,
  SkLayerProperties*                    pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLoaderDriverProperties(
  SkLoaderEnumerateFlags                enumerateFlags,
  uint32_t*                             pDriverCount,
  SkDriverProperties*                   pProperties
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_LOADER_H
