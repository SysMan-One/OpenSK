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
#ifndef   OPENSK_EXT_GLOBAL_H
#define   OPENSK_EXT_GLOBAL_H 1

// OpenSK
#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Global Defines
////////////////////////////////////////////////////////////////////////////////

struct SkInternalObjectBase;

#define SK_INTERNAL_STRUCTURE_BASE                                              \
SkStructureType                       sType;                                    \
void const*                           pNext

#define SK_INTERNAL_OBJECT_BASE                                                 \
SkObjectType                          _oType;                                   \
struct SkInternalObjectBase*          _pNext;                                   \
struct SkInternalObjectBase*          _pParent;                                 \
uint8_t                               _iUuid[SK_UUID_SIZE];                     \
void const*                           _vtable

#define SK_INTERNAL_CONVERT_HEX_LITERAL(c)                                      \
((c >= '0' && c <= '9') ? (c - '0') :                                           \
((c >= 'A' && c <= 'F') ? (c - 'A' + 10) :                                      \
((c >= 'a' && c <= 'f') ? (c - 'a' + 10) : 0)))

#define SK_INTERNAL_CREATE_UUID_UINT8(string, offset)                           \
(  SK_INTERNAL_CONVERT_HEX_LITERAL(string[offset]) * 0x10                       \
 + SK_INTERNAL_CONVERT_HEX_LITERAL(string[offset + 1]))

#define SK_INTERNAL_CREATE_UUID(string)                                         \
((uint8_t[SK_UUID_SIZE]){                                                       \
SK_INTERNAL_CREATE_UUID_UINT8(string, 0), SK_INTERNAL_CREATE_UUID_UINT8(string, 2) ,\
SK_INTERNAL_CREATE_UUID_UINT8(string, 4), SK_INTERNAL_CREATE_UUID_UINT8(string, 6) ,\
SK_INTERNAL_CREATE_UUID_UINT8(string, 9), SK_INTERNAL_CREATE_UUID_UINT8(string, 11),\
SK_INTERNAL_CREATE_UUID_UINT8(string, 14),SK_INTERNAL_CREATE_UUID_UINT8(string, 16),\
SK_INTERNAL_CREATE_UUID_UINT8(string, 19),SK_INTERNAL_CREATE_UUID_UINT8(string, 21),\
SK_INTERNAL_CREATE_UUID_UINT8(string, 24),SK_INTERNAL_CREATE_UUID_UINT8(string, 26),\
SK_INTERNAL_CREATE_UUID_UINT8(string, 28),SK_INTERNAL_CREATE_UUID_UINT8(string, 30),\
SK_INTERNAL_CREATE_UUID_UINT8(string, 32),SK_INTERNAL_CREATE_UUID_UINT8(string, 34) \
})

////////////////////////////////////////////////////////////////////////////////
// Global Internal Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkInternalStructureBase {
  SK_INTERNAL_STRUCTURE_BASE;
} SkInternalStructureBase;

typedef struct SkInternalObjectBase {
  SK_INTERNAL_OBJECT_BASE;
} SkInternalObjectBase;

typedef struct SkInstanceFunctionTable {
  // Core: 1.0.0
  PFN_skGetInstanceProcAddr             pfnGetInstanceProcAddr;
  PFN_skCreateInstance                  pfnCreateInstance;
  PFN_skDestroyInstance                 pfnDestroyInstance;
  PFN_skEnumerateInstanceDrivers        pfnEnumerateInstanceDrivers;
} SkInstanceFunctionTable;
SK_DEFINE_HANDLE(SkInstanceLayer);

////////////////////////////////////////////////////////////////////////////////
// Global Helper Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateInstanceFunctionTable(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetInstanceProcAddr             skGetInstanceProcAddr,
  SkInstanceFunctionTable const*        pPreviousFunctionTable,
  SkInstanceFunctionTable**             ppFunctionTable
);

SKAPI_ATTR void* SKAPI_CALL skAllocate(
  SkAllocationCallbacks const*          pAllocator,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
);

SKAPI_ATTR void* SKAPI_CALL skClearAllocate(
  SkAllocationCallbacks const*          pAllocator,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
);

SKAPI_ATTR void* SKAPI_CALL skReallocate(
  SkAllocationCallbacks const*          pAllocator,
  void*                                 pOriginal,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
);

SKAPI_ATTR void SKAPI_CALL skFree(
  SkAllocationCallbacks const*          pAllocator,
  void*                                 memory
);

SKAPI_ATTR SkResult SKAPI_CALL skParseUuid(
  char const*                           pSourceStringUUID,
  uint8_t                               destinationUUID[SK_UUID_SIZE]
);

SKAPI_ATTR void SKAPI_CALL skGenerateUuid(
  uint8_t                               pUuid[SK_UUID_SIZE]
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_GLOBAL_H
