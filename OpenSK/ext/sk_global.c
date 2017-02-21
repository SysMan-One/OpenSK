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
 * Helper functions intended for use by users creating an OpenSK driver/layer/ext.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/string.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/plt/platform.h>

// C99
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static void* SKAPI_CALL skDefaultAllocationIMPL(
  void*                                 pUserData,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  void* pMemory;
  (void)pUserData;
  (void)allocationScope;

  // Allocate and bake size into the type.
  pMemory = skAllocatePLT(size + sizeof(size_t), alignment);
  if (pMemory) {
    *((size_t*)pMemory) = size;
    pMemory = ((char*)pMemory) + sizeof(size_t);
  }

  return pMemory;
}

static void SKAPI_CALL skDefaultFreeIMPL(
  void*                                 pUserData,
  void*                                 memory
) {
  (void)pUserData;
  if (!memory) return;
  skFreePLT(((char*)memory) - sizeof(size_t));
}

static void* SKAPI_CALL skDefaultReallocationIMPL(
  void*                                 pUserData,
  void*                                 pOriginal,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  void* pMemory;
  size_t minSize;
  (void)pUserData;

  // Allocate the reallocated size for the data
  // Note: We cannot do realloc because the alignment may have changed.
  pMemory = skDefaultAllocationIMPL(pUserData, size, alignment, allocationScope);
  if (pOriginal) {
    if (pMemory) {
      minSize = *(size_t*)(((char*)pOriginal) - sizeof(size_t));
      memcpy(pMemory, pOriginal, (size < minSize) ? size : minSize);
      skDefaultFreeIMPL(pUserData, pOriginal);
    }
  }

  return pMemory;
}

static SkAllocationCallbacks const skDefaultAllocationCallbacksIMPL = {
  NULL,
  &skDefaultAllocationIMPL,
  &skDefaultReallocationIMPL,
  &skDefaultFreeIMPL
};

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR void* SKAPI_CALL skAllocate(
  SkAllocationCallbacks const*          pAllocator,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  if (!pAllocator) {
    pAllocator = &skDefaultAllocationCallbacksIMPL;
  }
  return pAllocator->pfnAllocation(
    pAllocator->pUserData,
    size,
    alignment,
    allocationScope
  );
}

SKAPI_ATTR void* SKAPI_CALL skClearAllocate(
  SkAllocationCallbacks const*          pAllocator,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  void* pMemory;
  pMemory = skAllocate(pAllocator, size, alignment, allocationScope);
  if (pMemory) {
    memset(pMemory, 0, size);
  }
  return pMemory;
}

SKAPI_ATTR void* SKAPI_CALL skReallocate(
  SkAllocationCallbacks const*          pAllocator,
  void*                                 pOriginal,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  if (!pAllocator) {
    pAllocator = &skDefaultAllocationCallbacksIMPL;
  }
  return pAllocator->pfnReallocation(
    pAllocator->pUserData,
    pOriginal,
    size,
    alignment,
    allocationScope
  );
}

SKAPI_ATTR void SKAPI_CALL skFree(
  SkAllocationCallbacks const*          pAllocator,
  void*                                 memory
) {
  if (!pAllocator) {
    pAllocator = &skDefaultAllocationCallbacksIMPL;
  }
  pAllocator->pfnFree(
    pAllocator->pUserData,
    memory
  );
}

#define HANDLE_PROC(name) if((pfnVoidFunction = skGetInstanceProcAddr(instance, "sk" #name))) pFunctionTable->pfn##name = (PFN_sk##name)pfnVoidFunction
SKAPI_ATTR SkResult SKAPI_CALL skCreateInstanceFunctionTable(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetInstanceProcAddr             skGetInstanceProcAddr,
  SkInstanceFunctionTable const*        pPreviousFunctionTable,
  SkInstanceFunctionTable**             ppFunctionTable
) {
  PFN_skVoidFunction pfnVoidFunction;
  SkInstanceFunctionTable *pFunctionTable;

  // Allocate the function table
  pFunctionTable = skAllocate(
    pAllocator,
    sizeof(SkInstanceFunctionTable),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE
  );
  if (!pFunctionTable) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy the previous function table over, or memset 0.
  if (pPreviousFunctionTable) {
    memcpy(pFunctionTable, pPreviousFunctionTable, sizeof(SkInstanceFunctionTable));
  }
  else {
    memset(pFunctionTable, 0, sizeof(SkInstanceFunctionTable));
  }

  // Core 1.0.0
  HANDLE_PROC(GetInstanceProcAddr);
  HANDLE_PROC(CreateInstance);
  HANDLE_PROC(DestroyInstance);
  HANDLE_PROC(EnumerateInstanceDrivers);

  // Return success
  *ppFunctionTable = pFunctionTable;
  return SK_SUCCESS;
}
#undef HANDLE_PROC

#define skParseUUIDPart(pSourceString, pDestinationChar)                        \
pSourceString = skParseHexadecimalUint8(pSourceString, pDestinationChar);       \
if (!pSourceString) return SK_ERROR_INVALID

#define skExpectUUIDPart(pSourceString, character)                              \
if (*pSourceString != character) { return SK_ERROR_INVALID; }                   \
++pSourceString

SKAPI_ATTR SkResult SKAPI_CALL skParseUuid(
  char const*                           pSourceStringUUID,
  uint8_t                               destinationUUID[SK_UUID_SIZE]
) {

  // Parse the first UUID section
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[0]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[1]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[2]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[3]);
  skExpectUUIDPart(pSourceStringUUID, '-');

  // Parse the second UUID section
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[4]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[5]);
  skExpectUUIDPart(pSourceStringUUID, '-');

  // Parse the third UUID section
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[6]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[7]);
  skExpectUUIDPart(pSourceStringUUID, '-');

  // Parse the fourth UUID section
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[8]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[9]);
  skExpectUUIDPart(pSourceStringUUID, '-');

  // Parse the fifth UUID section
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[10]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[11]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[12]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[13]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[14]);
  skParseUUIDPart(pSourceStringUUID, &destinationUUID[15]);

  // Check that this is the end of the UUID
  if (*pSourceStringUUID) {
    return SK_ERROR_INVALID;
  }
  return SK_SUCCESS;
}
#undef skExpectUUIDPart
#undef skParseUUIDPart
