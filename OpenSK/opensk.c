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
 * OpenSK standard include header.
 ******************************************************************************/

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/ext/sk_driver.h>
#include <OpenSK/ext/sk_layer.h>
#include <OpenSK/ext/sk_loader.h>
#include <OpenSK/ext/sk_stream.h>

// C99
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////

#define SK_OBJECT_PATH_PREFIX_LENGTH (sizeof(SK_OBJECT_PATH_PREFIX) / sizeof(SK_OBJECT_PATH_PREFIX[0]) - 1)
#define skInstance(i) ((SkInstanceFunctionTable const*)((SkInternalObjectBase*)i)->_vtable)
#define skDriver(h) ((SkDriverFunctionTable const*)((SkInternalObjectBase*)h)->_vtable)
#define skDevice(d) ((SkDriverFunctionTable const*)((SkInternalObjectBase*)d)->_vtable)
#define skEndpoint(h) ((SkDriverFunctionTable const*)((SkInternalObjectBase*)h)->_vtable)
#define skPcmStream(s) ((SkPcmStreamFunctionTable const*)((SkInternalObjectBase*)s)->_vtable)
#define skMidiStream(s) ((SkMidiStreamFunctionTable const*)((SkInternalObjectBase*)s)->_vtable)
#define skVideoStream(s) ((SkVideoStreamFunctionTable const*)((SkInternalObjectBase*)s)->_vtable)

typedef struct SkInstance_T {
  SK_INTERNAL_OBJECT_BASE;
  SkAllocationCallbacks const*          pAllocator;
  SkInternalObjectBase                  instanceLayer;
  SkDriver*                             drivers;
  SkLayerCreateInfo*                    layers;
  uint32_t                              driverCount;
  SkLayerCreateInfo*                    pLayerCreateInfo;
} SkInstance_T;

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkResult skEnumerateLayerCreateInfoIMPL(
  SkInstanceCreateInfo const*           pCreateInfo,
  uint32_t*                             pCreateInfoCount,
  SkLayerCreateInfo*                    pCreateInfos
) {
  uint32_t count;
  SkLayerCreateInfo const* pCreateInfoInternal;

  // Count all of the static-compiled drivers
  count = 0;
  for (
    pCreateInfoInternal = (SkLayerCreateInfo const*)pCreateInfo;
    pCreateInfoInternal;
    pCreateInfoInternal = (SkLayerCreateInfo const*)pCreateInfoInternal->pNext
    ) {
    if (pCreateInfoInternal->sType != SK_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
      continue;
    }
    if (pCreateInfos) {
      if (count >= *pCreateInfoCount) {
        return SK_INCOMPLETE;
      }
      pCreateInfos[count] = *pCreateInfoInternal;
    }
    ++count;
  }

  *pCreateInfoCount = count;
  return SK_SUCCESS;
}

static SkResult skEnumerateDriverCreateInfoIMPL(
  SkInstanceCreateInfo const*           pCreateInfo,
  uint32_t*                             pCreateInfoCount,
  SkDriverCreateInfo*                   pCreateInfos
) {
  uint32_t count;
  SkDriverCreateInfo const* pCreateInfoInternal;

  // Count all of the static-compiled drivers
  count = 0;
  for (
    pCreateInfoInternal = (SkDriverCreateInfo const*)pCreateInfo;
    pCreateInfoInternal;
    pCreateInfoInternal = (SkDriverCreateInfo const*)pCreateInfoInternal->pNext
    ) {
    if (pCreateInfoInternal->sType != SK_STRUCTURE_TYPE_DRIVER_CREATE_INFO) {
      continue;
    }
    if (pCreateInfos) {
      if (count >= *pCreateInfoCount) {
        return SK_INCOMPLETE;
      }
      pCreateInfos[count] = *pCreateInfoInternal;
    }
    ++count;
  }

  *pCreateInfoCount = count;
  return SK_SUCCESS;
}

static void skConnectToDriverCreateInfoIMPL(
  SkDriverCreateInfo*                   pCreateInfo,
  SkDriverCreateInfo**                  ppTargetCreateInfo
) {
  SkDriverCreateInfo* pTargetCreateInfo;

  // See if any create-info structures have been appended, yet.
  pTargetCreateInfo = *ppTargetCreateInfo;
  if (!pTargetCreateInfo) {
    *ppTargetCreateInfo = pCreateInfo;
    return;
  }

  // Find the end of the target list and append.
  // Check to see the layer is distinct as well.
  for (;;) {
    if (strcmp(pTargetCreateInfo->properties.driverName, pCreateInfo->properties.driverName) == 0) {
      return;
    }
    if (!pTargetCreateInfo->pNext) {
      break;
    }
    pTargetCreateInfo = (void*)pTargetCreateInfo->pNext;
  }

  pTargetCreateInfo->pNext = pCreateInfo;
}

static void skConnectToLayerCreateInfoIMPL(
  SkLayerCreateInfo*                    pCreateInfo,
  SkLayerCreateInfo**                   ppTargetCreateInfo
) {
  SkLayerCreateInfo* pTargetCreateInfo;

  // See if any create-info structures have been appended, yet.
  pTargetCreateInfo = *ppTargetCreateInfo;
  if (!pTargetCreateInfo) {
    *ppTargetCreateInfo = pCreateInfo;
    return;
  }

  // Find the end of the target list and append.
  // Check to see the layer is distinct as well.
  for (;;) {
    if (strcmp(pTargetCreateInfo->properties.layerName, pCreateInfo->properties.layerName) == 0) {
      return;
    }
    if (!pTargetCreateInfo->pNext) {
      break;
    }
    pTargetCreateInfo = (void*)pTargetCreateInfo->pNext;
  }

  pTargetCreateInfo->pNext = pCreateInfo;
}

static void skConnectToCreateInfoIMPL(
  void*                                 pCreateInfo,
  void**                                ppTargetCreateInfo
) {
  SkInternalStructureBase* pTargetCreateInfo;

  // See if any create-info structures have been appended, yet.
  pTargetCreateInfo = *ppTargetCreateInfo;
  if (!pTargetCreateInfo) {
    *ppTargetCreateInfo = pCreateInfo;
    return;
  }

  // Find the end of the target list and append.
  while (pTargetCreateInfo->pNext) {
    pTargetCreateInfo = (void*)pTargetCreateInfo->pNext;
  }
  pTargetCreateInfo->pNext = pCreateInfo;
}

static char const* skGetPathDevice(char const* pPath, uint32_t* pDeviceId) {
  uint32_t physicalCard;
  uint32_t numberOfCharacters;

  // Check for the required physical device prefix
  if (strncmp(pPath, "pd", 2) != 0) {
    return NULL;
  }

  // Make sure there is at least one alpha character
  if (!isalpha(*pPath)) {
    return NULL;
  }

  // Possibly a standard device name
  // Note: The +1 basically converts this into a one-based calculation.
  pPath += 2;
  physicalCard = 0;
  numberOfCharacters = 0;
  while (isalpha(*pPath)) {
    physicalCard *= 'z' - 'a' + 1;
    physicalCard += *pPath - 'a' + 1;
    ++pPath;
    ++numberOfCharacters;
    if (numberOfCharacters > 3) {
      return NULL;
    }
  }

  // Convert back from one-based to zero-based.
  *pDeviceId = physicalCard - 1;
  return pPath;
}

static char const* skGetPathNumber(char const* pPath, uint32_t* pNumber) {
  uint32_t number;

  // Check for a number (at least one is required)
  if (!(*pPath >= '0' && *pPath <= '9')) {
    return NULL;
  }

  // If the number starts with a 0, there should only be a 0
  if (pPath[0] == '0' && (pPath[1] >= '0' && pPath[1] <= '9')) {
    return NULL;
  }

  // Otherwise, the number is good to parse.
  number = 0;
  while (*pPath >= '0' && *pPath <= '9') {
    number *= 10;
    number += *pPath - '0';
    ++pPath;
  }

  *pNumber = number;
  return pPath;
}

SkObject skResolveObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkObject                              object,
  char const*                           pPath
);

static SkObject skResolveStreamObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkObject                              stream,
  char const*                           pPath
) {
  (void)pAllocator;
  (void)stream;
  (void)pPath;
  // You cannot resolve anything from a stream object.
  return SK_NULL_HANDLE;
}

static SkObject skResolveEndpointObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkEndpoint                            endpoint,
  char const*                           pPath
) {
  (void)pAllocator;
  (void)endpoint;
  (void)pPath;
  // You cannot resolve anything from an endpoint object.
  return SK_NULL_HANDLE;
}

static SkObject skResolveDeviceEndpointByIndexIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDevice                              device,
  char const*                           pPath
) {
  SkResult result;
  SkEndpoint endpoint;
  uint32_t endpointIdx;
  uint32_t endpointCount;
  SkEndpoint* pEndpoints;

  // There are two ways to get to an endpoint object.
  // 1. The canonical way (sk://driver/device/epN)
  // 2. The short-hand way (sl://driver/deviceN)
  if (strncmp(pPath, "ep", 2) == 0) {
    pPath += 2;
  }

  // Get the endpoint number to retrieve from the device
  pPath = skGetPathNumber(pPath, &endpointIdx);
  if (!pPath) {
    return SK_NULL_HANDLE;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, NULL);
  if (result != SK_SUCCESS || endpointIdx >= endpointCount) {
    return SK_NULL_HANDLE;
  }

  // Allocate temporary storage for the endpoint list
  endpointCount = endpointIdx + 1;
  pEndpoints = skAllocate(
    pAllocator,
    sizeof(SkEndpoint) * endpointCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pEndpoints) {
    return SK_NULL_HANDLE;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, pEndpoints);
  if (result < SK_SUCCESS || endpointIdx >= endpointCount) {
    skFree(pAllocator, pEndpoints);
    return SK_NULL_HANDLE;
  }
  endpoint = pEndpoints[endpointIdx];
  skFree(pAllocator, pEndpoints);

  // If there is a path separator, we should consume it - only one is allowed.
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, endpoint, pPath);
}

static SkObject skResolveDeviceEndpointByNameIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDevice                              device,
  char const*                           pPath
) {
  uint32_t idx;
  SkResult result;
  SkEndpoint endpoint;
  uint32_t endpointCount;
  SkEndpoint* pEndpoints;
  SkEndpointProperties properties;
  uint32_t endpointIdentifierLength;

  // Find the length of the endpoint
  endpointIdentifierLength = 0;
  while (pPath[endpointIdentifierLength] && pPath[endpointIdentifierLength] != SK_OBJECT_PATH_SEPARATOR) {
    ++endpointIdentifierLength;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, NULL);
  if (result != SK_SUCCESS || endpointCount == 0) {
    return SK_NULL_HANDLE;
  }

  // Allocate temporary storage for the endpoint list
  pEndpoints = skAllocate(
    pAllocator,
    sizeof(SkEndpoint) * endpointCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pEndpoints) {
    return SK_NULL_HANDLE;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, pEndpoints);
  if (result < SK_SUCCESS || endpointCount == 0) {
    skFree(pAllocator, pEndpoints);
    return SK_NULL_HANDLE;
  }

  // Find the requested driver object
  endpoint = SK_NULL_HANDLE;
  for (idx = 0; idx < endpointCount; ++idx) {
    result = skQueryEndpointProperties(pEndpoints[idx], &properties);
    if (result != SK_SUCCESS) {
      continue;
    }
    if (strncmp(pPath, properties.endpointName, endpointIdentifierLength) == 0) {
      endpoint = pEndpoints[idx];
      break;
    }
  }
  skFree(pAllocator, pEndpoints);

  // If there is a path separator, we should consume it - only one is allowed.
  pPath += endpointIdentifierLength;
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, endpoint, pPath);
}

static SkObject skResolveDeviceObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDevice                              device,
  char const*                           pPath
) {
  SkObject object;

  // Attempt to get an endpoint from the driver
  object = skResolveDeviceEndpointByIndexIMPL(pAllocator, device, pPath);
  if (object) {
    return object;
  }

  // Attempt to get an endpoint from the driver
  object = skResolveDeviceEndpointByNameIMPL(pAllocator, device, pPath);
  if (object) {
    return object;
  }

  return SK_NULL_HANDLE;
}

static SkObject skResolveDriverDeviceByIndexIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  char const*                           pPath
) {
  char const* pEndpointPath;
  SkResult result;
  SkObject* pObjects;
  SkObject currObject;
  uint32_t objectCount;
  uint32_t maxObjectCount;
  uint32_t currDeviceIdx;
  uint32_t currEndpointIdx;

  // Handle standard device names "pd[a-z]{1,3}"
  pPath = skGetPathDevice(pPath, &currDeviceIdx);
  if (!pPath) {
    return SK_NULL_HANDLE;
  }

  // Handle standard device endpoints "[0-9]*"
  currEndpointIdx = 0;
  pEndpointPath = skGetPathNumber(pPath, &currEndpointIdx);
  if (pEndpointPath) {
    pPath = pEndpointPath;
  }

  // Find the max possible number of objects to allocate for.
  maxObjectCount =
    ((currDeviceIdx > currEndpointIdx) ?
     currDeviceIdx : currEndpointIdx) + 1;

  // Make sure the user is requesting a valid device.
  result = skEnumerateDriverDevices(driver, &objectCount, NULL);
  if (result != SK_SUCCESS || currDeviceIdx >= objectCount) {
    return SK_NULL_HANDLE;
  }

  // Allocate storage for enumerating the devices.
  pObjects = skAllocate(
    pAllocator,
    sizeof(SkObject) * maxObjectCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pObjects) {
    return SK_NULL_HANDLE;
  }

  // Grab the physical devices and again check that we have the device.
  // Note: This is required because some devices may change between calls.
  objectCount = maxObjectCount;
  result = skEnumerateDriverDevices(driver, &objectCount, (SkDevice*)pObjects);
  if (result < SK_SUCCESS || currDeviceIdx >= objectCount) {
    skFree(pAllocator, pObjects);
    return SK_NULL_HANDLE;
  }
  currObject = pObjects[currDeviceIdx];

  // If we are querying for a device endpoint, also check for that.
  if (pEndpointPath) {
    objectCount = maxObjectCount;
    result = skEnumerateDeviceEndpoints((SkDevice)currObject, &objectCount, (SkEndpoint*)pObjects);
    if (result < SK_SUCCESS || currEndpointIdx >= objectCount) {
      skFree(pAllocator, pObjects);
      return SK_NULL_HANDLE;
    }
    currObject = pObjects[currEndpointIdx];
  }

  // At this point, there is no longer a need for the object array.
  skFree(pAllocator, pObjects);

  // If there is a path separator, we should consume it - only one is allowed.
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, currObject, pPath);
}

static SkObject skResolveDriverDeviceByNameIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  char const*                           pPath
) {
  uint32_t idx;
  SkResult result;
  SkDevice device;
  uint32_t deviceCount;
  SkDevice * pDevices;
  SkDeviceProperties properties;
  uint32_t deviceIdentifierLength;

  // Find the length of the device
  deviceIdentifierLength = 0;
  while (pPath[deviceIdentifierLength] && pPath[deviceIdentifierLength] != SK_OBJECT_PATH_SEPARATOR) {
    ++deviceIdentifierLength;
  }

  // Get all of the devices and attempt to find the matching one.
  result = skEnumerateDriverDevices(driver, &deviceCount, NULL);
  if (result != SK_SUCCESS || deviceCount == 0) {
    return SK_NULL_HANDLE;
  }

  // Allocate temporary storage for the endpoint list
  pDevices = skAllocate(
    pAllocator,
    sizeof(SkEndpoint) * deviceCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pDevices) {
    return SK_NULL_HANDLE;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDriverDevices(driver, &deviceCount, pDevices);
  if (result < SK_SUCCESS || deviceCount == 0) {
    skFree(pAllocator, pDevices);
    return SK_NULL_HANDLE;
  }

  // Find the requested driver object
  device = SK_NULL_HANDLE;
  for (idx = 0; idx < deviceCount; ++idx) {
    result = skQueryDeviceProperties(pDevices[idx], &properties);
    if (result != SK_SUCCESS) {
      continue;
    }
    if (strncmp(pPath, properties.deviceName, deviceIdentifierLength) == 0) {
      device = pDevices[idx];
      break;
    }
  }
  skFree(pAllocator, pDevices);

  // If there is a path separator, we should consume it - only one is allowed.
  pPath += deviceIdentifierLength;
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, device, pPath);
}

static SkObject skResolveDriverEndpointByIndexIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  char const*                           pPath
) {
  SkResult result;
  SkEndpoint* pEndpoints;
  SkEndpoint currEndpoint;
  uint32_t endpointCount;
  uint32_t currEndpointIdx;

  // Handle standard endpoint names "ep"
  if (pPath[0] != 'e' || pPath[1] != 'p') {
    return SK_NULL_HANDLE;
  }
  pPath += 2;

  // Handle standard endpoint index "[0-9]*"
  currEndpointIdx = 0;
  pPath = skGetPathNumber(pPath, &currEndpointIdx);
  if (!pPath) {
    return SK_NULL_HANDLE;
  }

  // Make sure the user is requesting a valid endpoint.
  result = skEnumerateDriverEndpoints(driver, &endpointCount, NULL);
  if (result != SK_SUCCESS || currEndpointIdx >= endpointCount) {
    return SK_NULL_HANDLE;
  }

  // Allocate storage for enumerating the devices.
  pEndpoints = skAllocate(
    pAllocator,
    sizeof(SkEndpoint) * endpointCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pEndpoints) {
    return SK_NULL_HANDLE;
  }

  // Grab the physical devices and again check that we have the device.
  // Note: This is required because some devices may change between calls.
  result = skEnumerateDriverEndpoints(driver, &endpointCount, pEndpoints);
  if (result < SK_SUCCESS || currEndpointIdx >= endpointCount) {
    skFree(pAllocator, pEndpoints);
    return SK_NULL_HANDLE;
  }
  currEndpoint = pEndpoints[currEndpointIdx];

  // At this point, there is no longer a need for the object array.
  skFree(pAllocator, pEndpoints);

  // If there is a path separator, we should consume it - only one is allowed.
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, currEndpoint, pPath);
}

static SkObject skResolveDriverEndpointByNameIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  char const*                           pPath
) {
  uint32_t idx;
  SkResult result;
  SkEndpoint endpoint;
  uint32_t endpointCount;
  SkEndpoint* pEndpoints;
  SkEndpointProperties properties;
  uint32_t endpointIdentifierLength;

  // Find the length of the endpoint
  endpointIdentifierLength = 0;
  while (pPath[endpointIdentifierLength] && pPath[endpointIdentifierLength] != SK_OBJECT_PATH_SEPARATOR) {
    ++endpointIdentifierLength;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDriverEndpoints(driver, &endpointCount, NULL);
  if (result != SK_SUCCESS || endpointCount == 0) {
    return SK_NULL_HANDLE;
  }

  // Allocate temporary storage for the endpoint list
  pEndpoints = skAllocate(
    pAllocator,
    sizeof(SkEndpoint) * endpointCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pEndpoints) {
    return SK_NULL_HANDLE;
  }

  // Get all of the endpoints and attempt to find the matching one.
  result = skEnumerateDriverEndpoints(driver, &endpointCount, pEndpoints);
  if (result < SK_SUCCESS || endpointCount == 0) {
    skFree(pAllocator, pEndpoints);
    return SK_NULL_HANDLE;
  }

  // Find the requested driver object
  endpoint = SK_NULL_HANDLE;
  for (idx = 0; idx < endpointCount; ++idx) {
    result = skQueryEndpointProperties(pEndpoints[idx], &properties);
    if (result != SK_SUCCESS) {
      continue;
    }
    if (strncmp(pPath, properties.endpointName, endpointIdentifierLength) == 0) {
      endpoint = pEndpoints[idx];
      break;
    }
  }
  skFree(pAllocator, pEndpoints);

  // If there is a path separator, we should consume it - only one is allowed.
  pPath += endpointIdentifierLength;
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, endpoint, pPath);
}

static SkObject skResolveDriverObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  char const*                           pPath
) {
  SkObject object;

  // Attempt to get an object in the form of a device.
  object = skResolveDriverDeviceByIndexIMPL(pAllocator, driver, pPath);
  if (object) {
    return object;
  }

  // Attempt to get an object in the form of a device.
  object = skResolveDriverDeviceByNameIMPL(pAllocator, driver, pPath);
  if (object) {
    return object;
  }

  // Attempt to get an endpoint from the driver
  object = skResolveDriverEndpointByIndexIMPL(pAllocator, driver, pPath);
  if (object) {
    return object;
  }

  // Attempt to get an endpoint from the driver
  object = skResolveDriverEndpointByNameIMPL(pAllocator, driver, pPath);
  if (object) {
    return object;
  }

  return SK_NULL_HANDLE;
}

static SkObject SKAPI_CALL skResolveInstanceObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkInstance                            instance,
  char const*                           pPath
) {
  uint32_t idx;
  SkDriver driver;
  uint32_t driverIdentifierLength;
  SkDriverProperties driverProperties;

  // Calculate the drivername length
  driverIdentifierLength = 0;
  while (pPath[driverIdentifierLength] && pPath[driverIdentifierLength] != SK_OBJECT_PATH_SEPARATOR) {
    ++driverIdentifierLength;
  }

  // Find the requested driver object
  driver = SK_NULL_HANDLE;
  for (idx = 0; idx < instance->driverCount; ++idx) {
    skGetDriverProperties(instance->drivers[idx], &driverProperties);
    if (strncmp(pPath, driverProperties.identifier, driverIdentifierLength) == 0) {
      driver = instance->drivers[idx];
      break;
    }
  }
  if (!driver) {
    return SK_NULL_HANDLE;
  }

  // If there is a path separator, we should consume it - only one is allowed.
  pPath += driverIdentifierLength;
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    ++pPath;
  }

  return skResolveObjectIMPL(pAllocator, driver, pPath);
}

SkObject skResolveObjectIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkObject                              object,
  char const*                           pPath
) {

  // If object is null, we cannot request anything, return null.
  if (!object) {
    return SK_NULL_HANDLE;
  }

  // This particular function never expects to begin with a "/".
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    return SK_NULL_HANDLE;
  }

  // If the request is exactly ".", it means get the current device.
  // This is a No-Op which maps back to standard directory traversal.
  while (pPath[0] == '.' && pPath[1] == SK_OBJECT_PATH_SEPARATOR) {
    pPath += 2;
  }
  if (pPath[0] == '.' && !pPath[1]) {
    ++pPath;
  }

  // If the request is to go "back" an object, we should request the parent.
  if (pPath[0] == '.' && pPath[1] == '.' && pPath[2] == SK_OBJECT_PATH_SEPARATOR) {
    pPath += 3;
    return skResolveObjectIMPL(pAllocator, ((SkInternalObjectBase*)object)->_pParent, pPath);
  }
  if (pPath[0] == '.' && pPath[1] == '.' && !pPath[2]) {
    pPath += 2;
    return skResolveObjectIMPL(pAllocator, ((SkInternalObjectBase*)object)->_pParent, pPath);
  }

  // If this is the end of the request, we have requested the object passed-in.
  if (!pPath[0]) {
    return object;
  }

  // Otherwise, what we do depends on what kind of object we have.
  switch (skGetObjectType(object)) {
    case SK_OBJECT_TYPE_INVALID:
    case SK_OBJECT_TYPE_LAYER:
      return SK_NULL_HANDLE;
    case SK_OBJECT_TYPE_INSTANCE:
      return skResolveInstanceObjectIMPL(pAllocator, object, pPath);
    case SK_OBJECT_TYPE_DRIVER:
      return skResolveDriverObjectIMPL(pAllocator, object, pPath);
    case SK_OBJECT_TYPE_DEVICE:
      return skResolveDeviceObjectIMPL(pAllocator, object, pPath);
    case SK_OBJECT_TYPE_ENDPOINT:
      return skResolveEndpointObjectIMPL(pAllocator, object, pPath);
    case SK_OBJECT_TYPE_PCM_STREAM:
    case SK_OBJECT_TYPE_MIDI_STREAM:
    case SK_OBJECT_TYPE_VIDEO_STREAM:
      return skResolveStreamObjectIMPL(pAllocator, object, pPath);
    case SK_OBJECT_TYPE_RANGE_SIZE:
    case SK_OBJECT_TYPE_MAX_ENUM:
      return NULL;
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Internal Functions
////////////////////////////////////////////////////////////////////////////////

static PFN_skVoidFunction SKAPI_CALL skGetInstanceProcAddr_internal(
  SkInstance                            instance,
  char const*                           pName
);

static SkResult SKAPI_CALL skCreateInstance_internal(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstance*                           pInstance
) {
  uint32_t idx;
  SkResult result;
  SkDriver driver;
  SkInstance instance;
  uint32_t layerCount;
  SkResult finalResult;
  uint32_t driverCount;
  PFN_skCreateDriver pfnCreateDriver;
  SkDriverCreateInfo driverCreateInfo;
  SkLayerCreateInfo const* pLayerCreateInfo;
  SkDriverCreateInfo const* pDriverCreateInfo;

  // Find out the number of drivers to create
  result = skEnumerateDriverCreateInfoIMPL(
    pCreateInfo,
    &driverCount,
    NULL
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Find out the number of layers to copy
  result = skEnumerateLayerCreateInfoIMPL(
    pCreateInfo,
    &layerCount,
    NULL
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Allocate the instance object
  instance = skClearAllocate(
    pAllocator,
    sizeof(SkInstance_T) +
    sizeof(SkDriver) * driverCount +
    sizeof(SkLayerCreateInfo) * layerCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE
  );
  if (!instance) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Configure the instance
  instance->_oType = SK_OBJECT_TYPE_INSTANCE;
  instance->_pNext = &instance->instanceLayer;
  instance->instanceLayer._oType = SK_OBJECT_TYPE_LAYER;
  instance->pAllocator = pAllocator;
  instance->drivers = (SkDriver*)&instance[1];
  instance->layers = (SkLayerCreateInfo*)&instance->drivers[driverCount];

  // Copy the layer create info, and re-connect all of the pNext pointers.
  // Note: The -1 on layerCount is okay because there is always at least one.
  result = skEnumerateLayerCreateInfoIMPL(
    pCreateInfo,
    &layerCount,
    instance->layers
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, instance);
    return result;
  }
  for (idx = 0; idx < layerCount - 1; ++idx) {
    instance->layers[idx].pNext = &instance->layers[idx + 1];
  }
  instance->layers[idx].pNext = NULL;

  // Create the function table for the instance
  result = skCreateInstanceFunctionTable(
    instance,
    pAllocator,
    &skGetInstanceProcAddr_internal,
    NULL,
    (SkInstanceFunctionTable**)&instance->_vtable
  );
  if (result != SK_SUCCESS) {
    skFree(pAllocator, instance);
    return result;
  }
  instance->instanceLayer._vtable = instance->_vtable;

  // Create all of the requested drivers
  finalResult = SK_SUCCESS;
  for (pDriverCreateInfo = pCreateInfo->pNext; pDriverCreateInfo; pDriverCreateInfo = pDriverCreateInfo->pNext) {
    if (pDriverCreateInfo->sType != SK_STRUCTURE_TYPE_DRIVER_CREATE_INFO) {
      continue;
    }

    // Make a copy of the driver creation info, so we don't break pNext chain.
    driverCreateInfo = *pDriverCreateInfo;
    driverCreateInfo.pNext = pCreateInfo->pNext;

    // Grab the first layer's driver creation function.
    // In this case, we expect that there is at least one layer which can
    // create a driver. In the base case, this is the default layer.
    pfnCreateDriver = NULL;
    pLayerCreateInfo = (SkLayerCreateInfo*)&driverCreateInfo;
    while ((pLayerCreateInfo = pLayerCreateInfo->pNext)) {
      if (pLayerCreateInfo->sType != SK_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
        continue;
      }
      if (!pLayerCreateInfo->pfnGetDriverProcAddr) {
        continue;
      }
      pfnCreateDriver = (PFN_skCreateDriver)pLayerCreateInfo->pfnGetDriverProcAddr(NULL, "skCreateDriver");
      break;
    }
    if (!pfnCreateDriver) {
      skDestroyInstance(instance, pAllocator);
      return SK_ERROR_INITIALIZATION_FAILED;
    }

    // Attempt to create the driver (does not check driver contents)
    // Unlike layering, in this case if we fail to create a driver we will
    // simply skip the driver. This is because there is no precieved change in
    // functionality - the user can simply select a working driver.
    result = pfnCreateDriver(
      &driverCreateInfo,
      pAllocator,
      &driver
    );
    if (result != SK_SUCCESS) {
      finalResult = SK_INCOMPLETE;
      continue;
    }
    ((SkInternalObjectBase*)driver)->_pParent = (SkInternalObjectBase*)instance;

    instance->drivers[instance->driverCount] = driver;
    ++instance->driverCount;
  }

  *pInstance = instance;
  return finalResult;
}

static void SKAPI_CALL skDestroyInstance_internal(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  for (idx = 0; idx < instance->driverCount; ++idx) {
    skDriver(instance->drivers[idx])->pfnDestroyDriver(
      pAllocator,
      instance->drivers[idx]
    );
  }
  skFree(pAllocator, (void*)instance->instanceLayer._vtable);
  skFree(pAllocator, instance);
}

static SkResult SKAPI_CALL skEnumerateInstanceDrivers_internal(
  SkInstance                            instance,
  uint32_t*                             pDriverCount,
  SkDriver*                             pDriver
) {
  uint32_t idx;

  // Base-case, only provide the driver count.
  if (!pDriver) {
    *pDriverCount = instance->driverCount;
    return SK_SUCCESS;
  }

  // Otherwise, provide all of the drivers.
  // If the user cannot hold more drivers, return that the results are incomplete.
  for (idx = 0; idx < instance->driverCount; ++idx) {
    if (idx >= *pDriverCount) {
      return SK_INCOMPLETE;
    }
    pDriver[idx] = instance->drivers[idx];
  }

  // If the user could hold more than what we have, provide the actual count.
  *pDriverCount = idx;
  return SK_SUCCESS;
}

static SkResult SKAPI_CALL skCreateDriver_internal(
  SkDriverCreateInfo const*             pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriver*                             pDriver
) {
  PFN_skCreateDriver pfnCreateDriver;
  pfnCreateDriver = (PFN_skCreateDriver)
    pCreateInfo->pfnGetDriverProcAddr(NULL, "skCreateDriver");
  if (!pfnCreateDriver) {
    return SK_ERROR_INCOMPATIBLE_DRIVER;
  }
  return pfnCreateDriver(pCreateInfo, pAllocator, pDriver);
}

static SkResult SKAPI_CALL skCreatePcmStream_internal(
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream*                          pStream
) {
  PFN_skCreatePcmStream pfnCreatePcmStream;
  pfnCreatePcmStream = (PFN_skCreatePcmStream)
    pCreateInfo->pfnGetPcmStreamProcAddr(NULL, "skCreatePcmStream");
  if (!pfnCreatePcmStream) {
    return SK_ERROR_INCOMPATIBLE_DRIVER;
  }
  return pfnCreatePcmStream(pCreateInfo, pAllocator, pStream);
}

#define HANDLE_PROC(name) if (strcmp(pName, #name) == 0) return (PFN_skVoidFunction)&name##_internal
static PFN_skVoidFunction SKAPI_CALL skGetInstanceProcAddr_internal(
  SkInstance                            instance,
  char const*                           pName
) {
  (void)instance;
  // Core 1.0
  HANDLE_PROC(skGetInstanceProcAddr);
  HANDLE_PROC(skCreateInstance);
  HANDLE_PROC(skDestroyInstance);
  HANDLE_PROC(skEnumerateInstanceDrivers);
  return NULL;
}

static PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr_internal(
  SkDriver                              driver,
  char const*                           pName
) {
  (void)driver;
  // Core 1.0
  HANDLE_PROC(skGetDriverProcAddr);
  HANDLE_PROC(skGetInstanceProcAddr);
  HANDLE_PROC(skCreateDriver);
  return NULL;
}

static PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr_internal(
  SkPcmStream                           stream,
  char const*                           pName
) {
  (void)stream;
  // Core 1.0
  HANDLE_PROC(skGetPcmStreamProcAddr);
  HANDLE_PROC(skCreatePcmStream);
  return NULL;
}
#undef HANDLE_PROC

////////////////////////////////////////////////////////////////////////////////
// External Functions
////////////////////////////////////////////////////////////////////////////////

#define HANDLE_PROC(name) if (strcmp(pName, "sk" #name) == 0) return (PFN_skVoidFunction)&sk##name
#define HANDLE_JUMP(name) if (strcmp(pName, "sk" #name) == 0) return (PFN_skVoidFunction)skInstance(instance)->pfn##name;
SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetInstanceProcAddr(
  SkInstance                            instance,
  char const*                           pName
) {
  // Handle pre-construct functions (regular function mapping)
  HANDLE_PROC(EnumerateLayerProperties);
  HANDLE_PROC(ResolveParent);
  HANDLE_PROC(ResolveObject);
  HANDLE_PROC(CreateInstance);
  if (instance == SK_NULL_HANDLE) {
    return NULL;
  }

  // Handle post-construct instance functions (efficient function mapping)
  HANDLE_JUMP(DestroyInstance);
  HANDLE_JUMP(EnumerateInstanceDrivers);

  // Handle post-construct API functions (inefficient function mapping)
  HANDLE_PROC(GetDriverProcAddr);
  HANDLE_PROC(GetDriverProperties);
  HANDLE_PROC(GetDriverFeatures);
  HANDLE_PROC(EnumerateDriverEndpoints);
  HANDLE_PROC(EnumerateDriverDevices);
  HANDLE_PROC(QueryDeviceProperties);
  HANDLE_PROC(QueryDeviceFeatures);
  HANDLE_PROC(EnumerateDeviceEndpoints);
  HANDLE_PROC(QueryEndpointFeatures);
  HANDLE_PROC(QueryEndpointProperties);
  HANDLE_PROC(RequestPcmStream);
  HANDLE_PROC(GetPcmStreamProcAddr);
  HANDLE_PROC(ClosePcmStream);
  HANDLE_PROC(GetPcmStreamInfo);
  HANDLE_PROC(GetPcmStreamChannelMap);
  HANDLE_PROC(SetPcmStreamChannelMap);
  HANDLE_PROC(StartPcmStream);
  HANDLE_PROC(StopPcmStream);
  HANDLE_PROC(WaitPcmStream);
  HANDLE_PROC(PausePcmStream);
  HANDLE_PROC(AvailPcmStreamSamples);
  HANDLE_PROC(WritePcmStreamInterleaved);
  HANDLE_PROC(WritePcmStreamNoninterleaved);
  HANDLE_PROC(ReadPcmStreamInterleaved);
  HANDLE_PROC(ReadPcmStreamNoninterleaved);

  // No function found
  return NULL;
}
#undef HANDLE_JUMP
#undef HANDLE_PROC

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLayerProperties(
  uint32_t*                             pLayerCount,
  SkLayerProperties*                    pProperties
) {
  return skEnumerateLoaderLayerProperties(
    SK_LOADER_ENUMERATE_ALL_BIT,
    pLayerCount,
    pProperties
  );
}

SKAPI_ATTR SkObject SKAPI_CALL skResolveParent(
  SkObject                              object
) {
  return ((SkInternalObjectBase*)object)->_pParent;
}

SKAPI_ATTR SkObject SKAPI_CALL skResolveObject(
  SkObject                              object,
  char const*                           pPath
) {
  SkInstance instance;

  // First, find the instance - it will be needed for later calculations.
  // We should always find an instance. But in case the driver is malformed,
  // we check anyways to make sure that the instance exists when needed.
  instance = object;
  while (instance && skGetObjectType(instance) != SK_OBJECT_TYPE_INSTANCE) {
    instance = (SkInstance)instance->_pParent;
  }

  // The object path prefix is completely optional. If it's present, skip it.
  // This is only supported so that an OpenSK path is obvious to the end-user.
  if (strncmp(pPath, SK_OBJECT_PATH_PREFIX, SK_OBJECT_PATH_PREFIX_LENGTH) == 0) {
    pPath += SK_OBJECT_PATH_PREFIX_LENGTH;
  }

  // If there is a root specifier on the object, we must first resolve the
  // instance. This allows us to traverse back to any owning instance.
  if (pPath[0] == SK_OBJECT_PATH_SEPARATOR) {
    object = instance;
    ++pPath;
  }

  // Before we pass this off to the driver to handle, check if the user
  // is requesting a basic physical device or standard path.
  return skResolveObjectIMPL(instance->pAllocator, object, pPath);
}

SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstance*                           pInstance
) {
  uint32_t idx;
  SkResult result;
  SkInstanceCreateInfo createInfo;
  char* pRawMemory;

  // Drivers
  uint32_t totalDriverCount;
  uint32_t createDriverCount;
  uint32_t systemDriverCount;
  uint32_t staticDriverCount;
  uint32_t implicitDriverCount;
  SkDriverProperties* pDriverProperties;
  SkDriverCreateInfo* pDriverCreateInfo;
  SkDriverCreateInfo* pDistinctDriverCreateInfo;

  // Layers
  uint32_t totalLayerCount;
  uint32_t createLayerCount;
  uint32_t systemLayerCount;
  uint32_t staticLayerCount;
  uint32_t dynamicLayerCount;
  uint32_t implicitLayerCount;
  uint32_t explicitLayerCount;
  SkLayerProperties* pLayerProperties;
  SkLayerCreateInfo* pLayerCreateInfo;
  SkLayerCreateInfo* pDistinctLayerCreateInfo;
  PFN_skCreateInstance pfnCreateInstance;

  // Find the number of explicit-static objects which need to be constructed.
  // Note: This is useful if the user wishes to statically-compile a driver or
  //       layer into OpenSK. In these cases, any create-info structures
  //       attached to the Instance create-info will be constructed.
  result = skEnumerateDriverCreateInfoIMPL(pCreateInfo, &staticDriverCount, NULL);
  if (result != SK_SUCCESS) {
    return result;
  }
  result = skEnumerateLayerCreateInfoIMPL(pCreateInfo, &staticLayerCount, NULL);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Find the number of explicit-dynamic objects which need to be constructed.
  // Note: The only reason this number is different from the passed-in number
  //       is because multiple manifests may offer multiple solutions.
  dynamicLayerCount = 0;
  for (idx = 0; idx < pCreateInfo->enabledLayerCount; ++idx) {
    result = skInitializeLayerCreateInfo(
      pCreateInfo->ppEnabledLayerNames[idx],
      &totalLayerCount,
      NULL
    );
    if (result != SK_SUCCESS) {
      return result;
    }
    dynamicLayerCount += totalLayerCount;
  }

  // Find the number of implicit objects which need to be constructed.
  // Note: If the no-implicit bit is set, we will not look for any implicit
  //       layers or drivers, so we instead just assume they are "0" count.
  if (!(pCreateInfo->flags & SK_INSTANCE_CREATE_NO_IMPLICIT_OBJECTS_BIT)) {
    result = skEnumerateLoaderLayerProperties(SK_TRUE, &implicitLayerCount, NULL);
    if (result != SK_SUCCESS) {
      return result;
    }
    result = skEnumerateLoaderDriverProperties(SK_TRUE, &implicitDriverCount, NULL);
    if (result != SK_SUCCESS) {
      return result;
    }
  }
  else {
    implicitLayerCount = 0;
    implicitDriverCount = 0;
  }

  // Find the number of system static objects which need to be constructed.
  // Note: These are objects which OpenSK defines as a requirement for a valid
  //       OpenSK implementation. Disabled when the no-system bit is set.
  systemLayerCount = 0;
  systemDriverCount = 0;

  // Add the create-info counts together to form a final aggregate count.
  // Note: This count represents the maximum possible number of objects which
  //       could be constructed on this instance. It's more likely that a value
  //       smaller than the total is actually constructed.
  // Note: There is always at least 1 layer - one which calls into the module.
  totalLayerCount = staticLayerCount + dynamicLayerCount + implicitLayerCount + systemLayerCount + 1;
  totalDriverCount = staticDriverCount + implicitDriverCount + systemDriverCount;

  // Allocate one large blob for all of the command structures.
  // This is easier to keep track of and easier to free later on.
  pRawMemory = skClearAllocate(
    pAllocator,
    sizeof(SkLayerCreateInfo) * totalLayerCount +
    sizeof(SkDriverCreateInfo) * totalDriverCount +
    sizeof(SkLayerProperties) * implicitLayerCount +
    sizeof(SkDriverProperties) * implicitDriverCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pRawMemory) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  pLayerCreateInfo = (SkLayerCreateInfo*)pRawMemory;
  pDriverCreateInfo = (SkDriverCreateInfo*)&pLayerCreateInfo[totalLayerCount];
  pLayerProperties = (SkLayerProperties*)&pDriverCreateInfo[totalDriverCount];
  pDriverProperties = (SkDriverProperties*)&pLayerProperties[implicitLayerCount];

  // Grab the properties for all of the implicit objects
  result = skEnumerateLoaderLayerProperties(SK_TRUE, &implicitLayerCount, pLayerProperties);
  if (result < SK_SUCCESS) {
    skFree(pAllocator, pRawMemory);
    return result;
  }
  result = skEnumerateLoaderDriverProperties(SK_TRUE, &implicitDriverCount, pDriverProperties);
  if (result < SK_SUCCESS) {
    skFree(pAllocator, pRawMemory);
    return result;
  }

  // Create an ordered list of all the layer create-info structures.
  // Note: The order is important! We expect objects instantiated in this order
  //       {Explicit-Static} + {Explicit-Enabled} + {Implicit-Enabled} + {System-Enabled}
  createLayerCount = 0;
  {
    // Handle Explicit-Static
    result = skEnumerateLayerCreateInfoIMPL(pCreateInfo, &staticLayerCount, pLayerCreateInfo);
    if (result < SK_SUCCESS) {
      skFree(pAllocator, pRawMemory);
      return result;
    }
    createLayerCount += staticLayerCount;

    // Handle Explicit-Enabled
    for (idx = 0; idx < pCreateInfo->enabledLayerCount; ++idx) {
      explicitLayerCount = totalLayerCount - createLayerCount;
      result = skInitializeLayerCreateInfo(
        pCreateInfo->ppEnabledLayerNames[idx],
        &explicitLayerCount,
        &pLayerCreateInfo[createLayerCount]
      );
      if (result < SK_SUCCESS) {
        skFree(pAllocator, pRawMemory);
        return result;
      }
      createLayerCount += explicitLayerCount;
    }

    // Handle Implicit-Enabled
    for (idx = 0; idx < implicitLayerCount; ++idx) {
      explicitLayerCount = totalLayerCount - createLayerCount;
      result = skInitializeLayerCreateInfo(
        pLayerProperties[idx].layerName,
        &explicitLayerCount,
        &pLayerCreateInfo[createLayerCount]
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pRawMemory);
        return result;
      }
      createLayerCount += explicitLayerCount;
    }

    // Handle Default-Layer
    pLayerCreateInfo[createLayerCount].sType = SK_STRUCTURE_TYPE_LAYER_CREATE_INFO;
    pLayerCreateInfo[createLayerCount].pfnGetInstanceProcAddr = &skGetInstanceProcAddr_internal;
    pLayerCreateInfo[createLayerCount].pfnGetDriverProcAddr = &skGetDriverProcAddr_internal;
    pLayerCreateInfo[createLayerCount].pfnGetPcmStreamProcAddr = &skGetPcmStreamProcAddr_internal;
    ++createLayerCount;
  }

  // Create an ordered list of all the driver create-info structures.
  // Note: The order is important! We expect objects instantiated in this order
  //       {Explicit-Static} + {Implicit-Enabled} + {System-Enabled}
  createDriverCount = 0;
  {
    // Handle Explicit-Static
    result = skEnumerateDriverCreateInfoIMPL(
      pCreateInfo,
      &staticDriverCount,
      pDriverCreateInfo
    );
    if (result < SK_SUCCESS) {
      skFree(pAllocator, pRawMemory);
      return result;
    }
    createDriverCount += staticDriverCount;

    // Handle Implicit-Enabled
    for (idx = 0; idx < implicitDriverCount; ++idx) {
      implicitDriverCount = totalDriverCount - createDriverCount;
      result = skInitializeDriverCreateInfo(
        pDriverProperties[idx].driverName,
        &implicitDriverCount,
        &pDriverCreateInfo[createDriverCount]
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pRawMemory);
        return result;
      }
      createDriverCount += implicitDriverCount;
    }
  }

  // Connect all of the objects together, iff the object wasn't already added.
  pDistinctLayerCreateInfo = NULL;
  for (idx = 0; idx < createLayerCount; ++idx) {
    skConnectToLayerCreateInfoIMPL(
      &pLayerCreateInfo[idx],
      &pDistinctLayerCreateInfo
    );
  }
  pDistinctDriverCreateInfo = NULL;
  for (idx = 0; idx < createDriverCount; ++idx) {
    skConnectToDriverCreateInfoIMPL(
      &pDriverCreateInfo[idx],
      &pDistinctDriverCreateInfo
    );
  }

  // Make a copy of the create info structure.
  // This is so that we can make modifications without messing with user-provided data.
  // Creates a pNext list like: {Instance}->{Layers}->{Driver}
  createInfo = *pCreateInfo;
  createInfo.pNext = NULL;
  skConnectToCreateInfoIMPL(pDistinctLayerCreateInfo, (void**)&createInfo.pNext);
  skConnectToCreateInfoIMPL(pDistinctDriverCreateInfo, (void**)&createInfo.pNext);

  // Recurse through the layers until we find one with a valid instance layer.
  // This should _always_ succeed, and is considered an internal error otherwise.
  pfnCreateInstance = (PFN_skCreateInstance)pDistinctLayerCreateInfo->pfnGetInstanceProcAddr(NULL, "skCreateInstance");
  while (!pfnCreateInstance) {
    pDistinctLayerCreateInfo = (SkLayerCreateInfo*)pDistinctLayerCreateInfo->pNext;
    if (!pDistinctLayerCreateInfo || pDistinctLayerCreateInfo->sType != SK_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
      skFree(pAllocator, pRawMemory);
      return SK_ERROR_SYSTEM_INTERNAL;
    }
    pfnCreateInstance = (PFN_skCreateInstance)pDistinctLayerCreateInfo->pfnGetInstanceProcAddr(NULL, "skCreateInstance");
  }

  // Instantiate the call to actually construct instance/layers
  // This actually constructs the instance, which should call into each appropriate layer constructor.
  // Regardless of the results, we will destroy the driver and layer creation information.
  result = pfnCreateInstance(
    &createInfo,
    pAllocator,
    pInstance
  );
  skFree(pAllocator, pRawMemory);

  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skDestroyInstance(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator
) {
  return skInstance(instance)->pfnDestroyInstance(
    instance,
    pAllocator
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateInstanceDrivers(
  SkInstance                            instance,
  uint32_t*                             pdriverCount,
  SkDriver*                             pDriver
) {
  return skInstance(instance)->pfnEnumerateInstanceDrivers(
    instance,
    pdriverCount,
    pDriver
  );
}

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr(
  SkDriver                              driver,
  char const*                           pName
) {
  if (!driver) {
    return skGetDriverProcAddr_internal(
      NULL,
      pName
    );
  }
  return skDriver(driver)->pfnGetDriverProcAddr(
    driver,
    pName
  );
}

SKAPI_ATTR void SKAPI_CALL skGetDriverProperties(
  SkDriver                              driver,
  SkDriverProperties*                   pProperties
) {
  skDriver(driver)->pfnGetDriverProperties(
    driver,
    pProperties
  );
}

SKAPI_ATTR void SKAPI_CALL skGetDriverFeatures(
  SkDriver                              driver,
  SkDriverFeatures*                     pFeatures
) {
  skDriver(driver)->pfnGetDriverFeatures(
    driver,
    pFeatures
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDriverDevices(
  SkDriver                              driver,
  uint32_t*                             pDeviceCount,
  SkDevice*                             pDevices
) {
  return skDriver(driver)->pfnEnumerateDriverDevices(
    driver,
    pDeviceCount,
    pDevices
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDriverEndpoints(
  SkDriver                              driver,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  return skDriver(driver)->pfnEnumerateDriverEndpoints(
    driver,
    pEndpointCount,
    pEndpoints
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skQueryDeviceFeatures(
  SkDevice                              device,
  SkDeviceFeatures*                     pFeatures
) {
  return skDevice(device)->pfnQueryDeviceFeatures(
    device,
    pFeatures
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skQueryDeviceProperties(
  SkDevice                              device,
  SkDeviceProperties*                   pProperties
) {
  return skDevice(device)->pfnQueryDeviceProperties(
    device,
    pProperties
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDeviceEndpoints(
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  return skDevice(device)->pfnEnumerateDeviceEndpoints(
    device,
    pEndpointCount,
    pEndpoints
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skQueryEndpointFeatures(
  SkEndpoint                            endpoint,
  SkEndpointFeatures*                   pFeatures
) {
  return skEndpoint(endpoint)->pfnQueryEndpointFeatures(
    endpoint,
    pFeatures
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skQueryEndpointProperties(
  SkEndpoint                            endpoint,
  SkEndpointProperties*                 pProperties
) {
  return skEndpoint(endpoint)->pfnQueryEndpointProperties(
    endpoint,
    pProperties
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestPcmStream(
  SkEndpoint                            endpoint,
  SkPcmStreamRequest const*             pStreamRequest,
  SkPcmStream*                          pStream
) {
  return skEndpoint(endpoint)->pfnRequestPcmStream(
    endpoint,
    pStreamRequest,
    pStream
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skCreatePcmStream(
  SkEndpoint                            endpoint,
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream*                          pStream
) {
  SkResult result;
  SkInstance instance;
  SkPcmStreamCreateInfo createInfo;
  PFN_skCreatePcmStream pfnCreatePcmStream;
  SkLayerCreateInfo const* pLayerCreateInfo;

  // Grab the instance object.
  instance = (SkInstance)endpoint;
  while ((instance = (SkInstance)instance->_pParent)) {
    if (skGetObjectType(instance) == SK_OBJECT_TYPE_INSTANCE) {
      break;
    }
  }
  if (!instance) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Copy the createInfo and set pNext
  createInfo = *pCreateInfo;
  createInfo.pNext = instance->layers;

  // Grab the first layer's PCM creation function.
  // In this case, we expect that there is at least one layer which can
  // create a PCM. In the base case, this is the default layer.
  pfnCreatePcmStream = NULL;
  pLayerCreateInfo = (SkLayerCreateInfo*)&createInfo;
  while ((pLayerCreateInfo = pLayerCreateInfo->pNext)) {
    if (pLayerCreateInfo->sType != SK_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
      continue;
    }
    if (!pLayerCreateInfo->pfnGetPcmStreamProcAddr) {
      continue;
    }
    pfnCreatePcmStream = (PFN_skCreatePcmStream)pLayerCreateInfo->pfnGetPcmStreamProcAddr(NULL, "skCreatePcmStream");
    break;
  }
  if (!pfnCreatePcmStream) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Call into the creation function.
  result = pfnCreatePcmStream(&createInfo, pAllocator, pStream);
  if (result != SK_SUCCESS) {
    return result;
  }

  ((SkInternalObjectBase*)*pStream)->_pParent = (SkInternalObjectBase*)endpoint;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyPcmStream(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
) {
  skPcmStream(stream)->pfnDestroyPcmStream(
    stream,
    pAllocator
  );
}

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr(
  SkPcmStream                           stream,
  char const*                           pName
) {
  if (!stream) {
    return skGetPcmStreamProcAddr_internal(
      NULL,
      pName
    );
  }
  return skPcmStream(stream)->pfnGetPcmStreamProcAddr(
    stream,
    pName
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skClosePcmStream(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  return skPcmStream(stream)->pfnClosePcmStream(
    stream,
    drain
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skGetPcmStreamInfo(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkPcmStreamInfo*                      pStreamInfo
) {
  return skPcmStream(stream)->pfnGetPcmStreamInfo(
    stream,
    streamType,
    pStreamInfo
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skGetPcmStreamChannelMap(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel*                            pChannelMap
) {
  return skPcmStream(stream)->pfnGetPcmStreamChannelMap(
    stream,
    streamType,
    pChannelMap
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skSetPcmStreamChannelMap(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel const*                      pChannelMap
) {
  return skPcmStream(stream)->pfnSetPcmStreamChannelMap(
    stream,
    streamType,
    pChannelMap
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skStartPcmStream(
  SkPcmStream                           stream
) {
  return skPcmStream(stream)->pfnStartPcmStream(
    stream
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skStopPcmStream(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  return skPcmStream(stream)->pfnStopPcmStream(
    stream,
    drain
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skPausePcmStream(
  SkPcmStream                           stream,
  SkBool32                              pause
) {
  return skPcmStream(stream)->pfnPausePcmStream(
    stream,
    pause
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skRecoverPcmStream(
  SkPcmStream                           stream
) {
  return skPcmStream(stream)->pfnRecoverPcmStream(
    stream
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skWaitPcmStream(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  int32_t                               timeout
) {
  return skPcmStream(stream)->pfnWaitPcmStream(
    stream,
    streamType,
    timeout
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skAvailPcmStreamSamples(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  uint32_t*                             pSamples
) {
  return skPcmStream(stream)->pfnAvailPcmStreamSamples(
    stream,
    streamType,
    pSamples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skWritePcmStreamInterleaved(
  SkPcmStream                           stream,
  void const*                           pBuffer,
  uint32_t                              samples
) {
  return skPcmStream(stream)->pfnWritePcmStreamInterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skWritePcmStreamNoninterleaved(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  return skPcmStream(stream)->pfnWritePcmStreamNoninterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skReadPcmStreamInterleaved(
  SkPcmStream                           stream,
  void*                                 pBuffer,
  uint32_t                              samples
) {
  return skPcmStream(stream)->pfnReadPcmStreamInterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skReadPcmStreamNoninterleaved(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  return skPcmStream(stream)->pfnReadPcmStreamNoninterleaved(
    stream,
    pBuffer,
    samples
  );
}
