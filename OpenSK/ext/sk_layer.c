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
 * Functions and types which assist with creating an OpenSK layer extension.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/objects.h>
#include <OpenSK/ext/sk_layer.h>

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkLayerCreateInfo const* skGetLayerCreateInfoStructure(
  SkLayerCreateInfo const*              pCreateInfo,
  uint8_t const                         lUuid[SK_UUID_SIZE]
) {
  while (pCreateInfo) {
    if (pCreateInfo->sType == SK_STRUCTURE_TYPE_LAYER_CREATE_INFO
    &&  memcmp(pCreateInfo->properties.layerUuid, lUuid, SK_UUID_SIZE) == 0
    ) {
      break;
    }
    pCreateInfo = pCreateInfo->pNext;
  }
  return pCreateInfo;
}

static SkResult skAttachInstanceLayer(
  SkAllocationCallbacks const*          pAllocator,
  SkInstance                            instance,
  SkInstanceLayer                       instanceLayer,
  PFN_skGetInstanceProcAddr             pfnGetInstanceProcAddr
) {
  SkResult result;

  // Create the current layer's function table
  result = skCreateInstanceFunctionTable(
    instance,
    pAllocator,
    pfnGetInstanceProcAddr,
    instance->_vtable,
    (SkInstanceFunctionTable**)&instanceLayer->_vtable
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Attach the layer to the instance...
  instanceLayer->_pNext = instance->_pNext;
  instance->_pNext = (SkInternalObjectBase*)instanceLayer;
  instance->_vtable = instanceLayer->_vtable;
  return SK_SUCCESS;
}

static SkResult skAttachDriverLayer(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                              driver,
  SkDriverLayer                         driverLayer,
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr
) {
  SkResult result;

  // Create the current layer's function table
  result = skCreateDriverFunctionTable(
    driver,
    pAllocator,
    pfnGetDriverProcAddr,
    driver->_vtable,
    (SkDriverFunctionTable**)&driverLayer->_vtable
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Attach the layer to the object...
  driverLayer->_pNext = driver->_pNext;
  driver->_pNext = (SkInternalObjectBase*)driverLayer;
  driver->_vtable = driverLayer->_vtable;
  return SK_SUCCESS;
}

static SkResult skAttachPcmStreamLayer(
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream                           pcmStream,
  SkPcmStreamLayer                      pcmStreamLayer,
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr
) {
  SkResult result;

  // Create the current layer's function table
  result = skCreatePcmStreamFunctionTable(
    pcmStream,
    pAllocator,
    pfnGetPcmStreamProcAddr,
    pcmStream->_vtable,
    (SkPcmStreamFunctionTable**)&pcmStreamLayer->_vtable
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Attach the layer to the object...
  pcmStreamLayer->_pNext = pcmStream->_pNext;
  pcmStream->_pNext = (SkInternalObjectBase*)pcmStreamLayer;
  pcmStream->_vtable = pcmStreamLayer->_vtable;
  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skInitializeInstanceLayerBase(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstanceLayer                       instanceLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkInstance*                           pInstance
) {
  SkResult result;
  PFN_skCreateInstance pfnCreateInstance;
  SkLayerCreateInfo const* pLayerCreateInfo;
  SkLayerCreateInfo const* pNextLayerCreateInfo;

  // Grab the create info structure for this layer. This contains the
  // pfnNextCreateInstance function, which constructs the next layer or
  // the underlying instance (depending on layer order).
  // If this fails, we should always return SK_ERROR_SYSTEM_INTERNAL.
  pLayerCreateInfo = skGetLayerCreateInfoStructure(
    pCreateInfo->pNext,
    pLayerUuid
  );
  if (!pLayerCreateInfo) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Find the next layer which claims it will support instance layering.
  // Then find the skCreateInstance function so that we can call into it.
  // Note: If the layer claims to support instance-layers, but no creation
  //       function is found, this _is_ considered a fatal error.
  pfnCreateInstance = NULL;
  pNextLayerCreateInfo = pLayerCreateInfo;
  while ((pNextLayerCreateInfo = pNextLayerCreateInfo->pNext)) {
    if (pNextLayerCreateInfo->pfnGetInstanceProcAddr) {
      pfnCreateInstance = (PFN_skCreateInstance)
        pNextLayerCreateInfo->pfnGetInstanceProcAddr(NULL, "skCreateInstance");
      break;
    }
  }
  if (!pfnCreateInstance) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Call the next instance creation function, assess the results.
  // If this fails, we should always return the failure provided.
  result = pfnCreateInstance(
    pCreateInfo,
    pAllocator,
    pInstance
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Configure and initialize the layer, following these rules:
  // 1. _oType must be set to SK_OBJECT_TYPE_LAYER.
  // 3. _pNext must be set to NULL. (recommended: use skClearAllocate)
  // 4. _iUuid must be set to the UUID of this layer.
  instanceLayer->_oType = SK_OBJECT_TYPE_LAYER;
  instanceLayer->_pNext = NULL;
  instanceLayer->_pParent = (SkInternalObjectBase*)*pInstance;
  memcpy(instanceLayer->_iUuid, pLayerUuid, SK_UUID_SIZE);

  // Attach the layer to the instance
  // This adds the layer to the object's pNext list, creates and initializes
  // the layer's vtable (which is what controls the next function calls),
  // and updates the instance's vtable to allow these calls to happen.
  // If this fails, we should always return the failure provided.
  result = skAttachInstanceLayer(
    pAllocator,
    *pInstance,
    instanceLayer,
    pLayerCreateInfo->pfnGetInstanceProcAddr
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverLayerBase(
  SkDriverCreateInfo const*             pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriverLayer                         driverLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkDriver*                             pDriver
) {
  SkResult result;
  PFN_skCreateDriver pfnCreateDriver;
  SkLayerCreateInfo const* pLayerCreateInfo;
  SkLayerCreateInfo const* pNextLayerCreateInfo;

  // Grab the create info structure for this layer. This contains the
  // pfnNextCreateInstance function, which constructs the next layer or
  // the underlying instance (depending on layer order).
  // If this fails, we should always return SK_ERROR_SYSTEM_INTERNAL.
  pLayerCreateInfo = skGetLayerCreateInfoStructure(
    pCreateInfo->pNext,
    pLayerUuid
  );
  if (!pLayerCreateInfo) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Find the next layer which claims it will support driver layering.
  // Then find the skCreateInstance function so that we can call into it.
  // Note: If the layer claims to support driver-layers, but no creation
  //       function is found, this _is_ considered a fatal error.
  pfnCreateDriver = NULL;
  pNextLayerCreateInfo = pLayerCreateInfo;
  while ((pNextLayerCreateInfo = pNextLayerCreateInfo->pNext)) {
    if (pNextLayerCreateInfo->pfnGetDriverProcAddr) {
      pfnCreateDriver = (PFN_skCreateDriver)
        pNextLayerCreateInfo->pfnGetDriverProcAddr(NULL, "skCreateDriver");
      break;
    }
  }
  if (!pfnCreateDriver) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Call the next instance creation function, assess the results.
  // If this fails, we should always return the failure provided.
  result = pfnCreateDriver(
    pCreateInfo,
    pAllocator,
    pDriver
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Configure and initialize the layer, following these rules:
  // 1. _oType must be set to SK_OBJECT_TYPE_LAYER.
  // 3. _pNext must be set to NULL. (recommended: use skClearAllocate)
  // 4. _iUuid must be set to the UUID of this layer.
  driverLayer->_oType = SK_OBJECT_TYPE_LAYER;
  driverLayer->_pNext = NULL;
  driverLayer->_pParent = (SkInternalObjectBase*)*pDriver;
  memcpy(driverLayer->_iUuid, pLayerUuid, SK_UUID_SIZE);

  // Attach the layer to the driver
  // This adds the layer to the object's pNext list, creates and initializes
  // the layer's vtable (which is what controls the next function calls),
  // and updates the driver's vtable to allow these calls to happen.
  // If this fails, we should always return the failure provided.
  result = skAttachDriverLayer(
    pAllocator,
    *pDriver,
    driverLayer,
    pLayerCreateInfo->pfnGetDriverProcAddr
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializePcmStreamLayerBase(
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStreamLayer                      pcmStreamLayer,
  uint8_t const                         pLayerUuid[SK_UUID_SIZE],
  SkPcmStream *                         pPcmStream
) {
  SkResult result;
  PFN_skCreatePcmStream pfnCreatePcmStream;
  SkLayerCreateInfo const* pLayerCreateInfo;
  SkLayerCreateInfo const* pNextLayerCreateInfo;

  // Grab the create info structure for this layer. This contains the
  // pfnNextCreateInstance function, which constructs the next layer or
  // the underlying instance (depending on layer order).
  // If this fails, we should always return SK_ERROR_SYSTEM_INTERNAL.
  pLayerCreateInfo = skGetLayerCreateInfoStructure(
    pCreateInfo->pNext,
    pLayerUuid
  );
  if (!pLayerCreateInfo) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Find the next layer which claims it will support PCM layering.
  // Then find the skCreateInstance function so that we can call into it.
  // Note: If the layer claims to support PCM-layers, but no creation
  //       function is found, this _is_ considered a fatal error.
  pfnCreatePcmStream = NULL;
  pNextLayerCreateInfo = pLayerCreateInfo;
  while ((pNextLayerCreateInfo = pNextLayerCreateInfo->pNext)) {
    if (pNextLayerCreateInfo->pfnGetPcmStreamProcAddr) {
      pfnCreatePcmStream = (PFN_skCreatePcmStream)
        pNextLayerCreateInfo->pfnGetPcmStreamProcAddr(NULL, "skCreatePcmStream");
      break;
    }
  }
  if (!pfnCreatePcmStream) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Call the next instance creation function, assess the results.
  // If this fails, we should always return the failure provided.
  result = pfnCreatePcmStream(
    pCreateInfo,
    pAllocator,
    pPcmStream
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Configure and initialize the layer, following these rules:
  // 1. _oType must be set to SK_OBJECT_TYPE_LAYER.
  // 3. _pNext must be set to NULL. (recommended: use skClearAllocate)
  // 4. _iUuid must be set to the UUID of this layer.
  pcmStreamLayer->_oType = SK_OBJECT_TYPE_LAYER;
  pcmStreamLayer->_pNext = NULL;
  pcmStreamLayer->_pParent = (SkInternalObjectBase*)*pPcmStream;
  memcpy(pcmStreamLayer->_iUuid, pLayerUuid, SK_UUID_SIZE);

  // Attach the layer to the PCM
  // This adds the layer to the object's pNext list, creates and initializes
  // the layer's vtable (which is what controls the next function calls),
  // and updates the PCM's vtable to allow these calls to happen.
  // If this fails, we should always return the failure provided.
  result = skAttachPcmStreamLayer(
    pAllocator,
    *pPcmStream,
    pcmStreamLayer,
    pLayerCreateInfo->pfnGetPcmStreamProcAddr
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeInstanceLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkInstanceLayer                       instanceLayer
) {
  skFree(pAllocator, (void*)instanceLayer->_vtable);
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeDriverLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkDriverLayer                         driverLayer
) {
  skFree(pAllocator, (void*)driverLayer->_vtable);
}

SKAPI_ATTR void SKAPI_CALL skDeinitializePcmStreamLayerBase(
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStreamLayer                      pcmStreamLayer
) {
  skFree(pAllocator, (void*)pcmStreamLayer->_vtable);
}

SKAPI_ATTR SkInstanceLayer SKAPI_CALL skGetInstanceLayer(
  SkInstance                            instance,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkInstanceFunctionTable const**       ppFunctionTable
) {
  while (instance) {
    if (instance->_oType == SK_OBJECT_TYPE_LAYER
    && (memcmp(instance->_iUuid, iUuid, SK_UUID_SIZE) == 0)
    ) {
      *ppFunctionTable = instance->_pNext->_vtable;
      break;
    }
    instance = (SkInstance)instance->_pNext;
  }
  return (SkInstanceLayer)instance;
}

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayer(
  SkDriver                              driver,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
) {
  while (driver) {
    if (driver->_oType == SK_OBJECT_TYPE_LAYER
    && (memcmp(driver->_iUuid, iUuid, SK_UUID_SIZE) == 0)
    ) {
      *ppFunctionTable = driver->_pNext->_vtable;
      break;
    }
    driver = (SkDriver)driver->_pNext;
  }
  return (SkDriverLayer)driver;
}

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayerFromDevice(
  SkDevice                              device,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
) {
  return skGetDriverLayer((SkDriver)device->_pParent, iUuid, ppFunctionTable);
}

SKAPI_ATTR SkDriverLayer SKAPI_CALL skGetDriverLayerFromEndpoint(
  SkEndpoint                            endpoint,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkDriverFunctionTable const**         ppFunctionTable
) {
  if (skGetObjectType(endpoint->_pParent) == SK_OBJECT_TYPE_DRIVER) {
    return skGetDriverLayer((SkDriver)endpoint->_pParent, iUuid, ppFunctionTable);
  }
  else {
    return skGetDriverLayer((SkDriver)endpoint->_pParent->_pParent, iUuid, ppFunctionTable);
  }
}

SKAPI_ATTR SkPcmStreamLayer SKAPI_CALL skGetPcmStreamLayer(
  SkPcmStream                           pcmStream,
  uint8_t const                         iUuid[SK_UUID_SIZE],
  SkPcmStreamFunctionTable const**      ppFunctionTable
) {
  while (pcmStream) {
    if (pcmStream->_oType == SK_OBJECT_TYPE_LAYER
    && (memcmp(pcmStream->_iUuid, iUuid, SK_UUID_SIZE) == 0)
    ) {
      *ppFunctionTable = pcmStream->_pNext->_vtable;
      break;
    }
    pcmStream = (SkPcmStream)pcmStream->_pNext;
  }
  return (SkPcmStreamLayer)pcmStream;
}