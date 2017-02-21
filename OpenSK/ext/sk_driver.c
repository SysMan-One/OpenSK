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
 * Functions and types which assist with creating an OpenSK driver.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/objects.h>
#include <OpenSK/ext/sk_driver.h>
#include <OpenSK/ext/sk_layer.h>

// C99
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

#define HANDLE_PROC(name) if((pfnVoidFunction = pfnGetDriverProcAddr(driver, "sk" #name))) pFunctionTable->pfn##name = (PFN_sk##name)pfnVoidFunction
SKAPI_ATTR SkResult SKAPI_CALL skCreateDriverFunctionTable(
  SkDriver                              driver,
  SkAllocationCallbacks const*          pAllocator,
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr,
  SkDriverFunctionTable const*          pPreviousFunctionTable,
  SkDriverFunctionTable**               ppFunctionTable
) {
  PFN_skVoidFunction pfnVoidFunction;
  SkDriverFunctionTable *pFunctionTable;

  // Allocate the function table
  pFunctionTable = skAllocate(
    pAllocator,
    sizeof(SkDriverFunctionTable),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!pFunctionTable) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy the previous function table over, or memset 0.
  if (pPreviousFunctionTable) {
    memcpy(pFunctionTable, pPreviousFunctionTable, sizeof(SkDriverFunctionTable));
  }
  else {
    memset(pFunctionTable, 0, sizeof(SkDriverFunctionTable));
  }

  // Core 1.0.0
  HANDLE_PROC(GetDriverProcAddr);
  HANDLE_PROC(CreateDriver);
  HANDLE_PROC(DestroyDriver);
  HANDLE_PROC(GetDriverProperties);
  HANDLE_PROC(GetDriverFeatures);
  HANDLE_PROC(EnumerateDriverDevices);
  HANDLE_PROC(EnumerateDriverEndpoints);
  HANDLE_PROC(QueryDeviceFeatures);
  HANDLE_PROC(QueryDeviceProperties);
  HANDLE_PROC(EnumerateDeviceEndpoints);
  HANDLE_PROC(QueryEndpointFeatures);
  HANDLE_PROC(QueryEndpointProperties);
  HANDLE_PROC(RequestPcmStream);

  // Return success
  *ppFunctionTable = pFunctionTable;
  return SK_SUCCESS;
}
#undef HANDLE_PROC

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverBase(
  SkDriverCreateInfo const*             pDriverCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver
) {
  SkResult result;
  SkInternalObjectBase* pTrampolineLayer;

  // Construct a dummy layer which will catch trampolined calls
  pTrampolineLayer = skAllocate(
    pAllocator,
    sizeof(SkInternalObjectBase),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE
  );
  if (!pTrampolineLayer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Configure the driver trampoline layer
  pTrampolineLayer->_oType = SK_OBJECT_TYPE_LAYER;
  pTrampolineLayer->_pNext = NULL;
  pTrampolineLayer->_pParent = (SkInternalObjectBase*)driver;
  memcpy(pTrampolineLayer->_iUuid, pDriverCreateInfo->properties.driverUuid, SK_UUID_SIZE);

  // Initialize the driver
  result = skCreateDriverFunctionTable(
    driver,
    pAllocator,
    pDriverCreateInfo->pfnGetDriverProcAddr,
    NULL,
    (SkDriverFunctionTable**)&pTrampolineLayer->_vtable
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, pTrampolineLayer);
    return result;
  }

  // Configure the driver internals
  driver->_oType = SK_OBJECT_TYPE_DRIVER;
  driver->_pNext = pTrampolineLayer;
  driver->_vtable = pTrampolineLayer->_vtable;
  memcpy(driver->_iUuid, pDriverCreateInfo->properties.driverUuid, SK_UUID_SIZE);

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDeviceBase(
  SkDeviceCreateInfo const*             pDeviceCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDevice                              device
) {
  (void)pAllocator;
  device->_oType = SK_OBJECT_TYPE_DEVICE;
  device->_pNext = NULL;
  device->_pParent = (SkInternalObjectBase*)pDeviceCreateInfo->deviceParent;
  device->_vtable = pDeviceCreateInfo->deviceParent->_vtable;
  memcpy(device->_iUuid, pDeviceCreateInfo->deviceUuid, SK_UUID_SIZE);
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializeEndpointBase(
  SkEndpointCreateInfo const*           pEndpointCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkEndpoint                            endpoint
) {
  (void)pAllocator;
  endpoint->_oType = SK_OBJECT_TYPE_ENDPOINT;
  endpoint->_pNext = NULL;
  endpoint->_pParent = (SkInternalObjectBase*)pEndpointCreateInfo->endpointParent;
  endpoint->_vtable = ((SkInternalObjectBase*)pEndpointCreateInfo->endpointParent)->_vtable;
  memcpy(endpoint->_iUuid, pEndpointCreateInfo->endpointUuid, SK_UUID_SIZE);
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeDriverBase(
  SkDriver                              driver,
  SkAllocationCallbacks const*          pAllocator
) {
  SkInternalObjectBase* pAttachedObject;
  pAttachedObject = driver->_pNext;
  while (pAttachedObject) {
    if (pAttachedObject->_oType == SK_OBJECT_TYPE_LAYER
    &&  memcmp(pAttachedObject->_iUuid, driver->_iUuid, SK_UUID_SIZE) == 0
    ) {
      skFree(pAllocator, (void*)pAttachedObject->_vtable);
      skFree(pAllocator, pAttachedObject);
      break;
    }
    pAttachedObject = pAttachedObject->_pNext;
  }
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeDeviceBase(
  SkDevice                              device,
  SkAllocationCallbacks const*          pAllocator
) {
  (void)device;
  (void)pAllocator;
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeEndpointBase(
  SkEndpoint                            endpoint,
  SkAllocationCallbacks const*          pAllocator
) {
  (void)endpoint;
  (void)pAllocator;
}
