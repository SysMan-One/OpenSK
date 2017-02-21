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
 * A simple validation layer which checks inputs and outputs and logs details
 * about where failures can occur, or non-standard inputs/outputs are occurring.
 ******************************************************************************/

// OpenSK
#include <OpenSK/ext/sk_layer.h>

// C99
#include <assert.h>
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Layer Definitions
////////////////////////////////////////////////////////////////////////////////

// While it's not required to provide these defines, it is best to do so
// for readability purposes. This allows quick property changes to be made.
// Note: It is recommended that you use SK_INTERNAL_CREATE_UUID() to parse the
// stringized UUID at compile time, this will save time when requesting layers.
#define SK_LAYER_OPENSK_VALIDATION_NAME "SK_LAYER_OPENSK_VALIDATION"
#define SK_LAYER_OPENSK_VALIDATION_DISPLAY_NAME "OpenSK (Validation Layer)"
#define SK_LAYER_OPENSK_VALIDATION_DESCRIPTION "A layer which validates the input and output values for functions."
#define SK_LAYER_OPENSK_VALIDATION_UUID_STRING "8e55163b-1c23-4b16-9550-3c662f7eca89"
#define SK_LAYER_OPENSK_VALIDATION_UUID SK_INTERNAL_CREATE_UUID(SK_LAYER_OPENSK_VALIDATION_UUID_STRING)

// All newly-defined OpenSK-types MUST have SK_INTERNAL_OBJECT_BASE as the first
// field. This is all of the information the loader/API expects when using objs.
typedef struct SkInstanceLayer_T {
  SK_INTERNAL_OBJECT_BASE;
  // Other instance-information goes here.
} SkInstanceLayer_T;

typedef struct SkDriverLayer_T {
  SK_INTERNAL_OBJECT_BASE;
  // Other host-information goes here.
} SkDriverLayer_T;

typedef struct SkPcmStreamLayer_T {
  SK_INTERNAL_OBJECT_BASE;
  // Other stream-information goes here.
} SkPcmStreamLayer_T;

// skGetLayerProperties is the only function that absolutely MUST be present.
// This is because a layer could inject into Instance, Driver, a Stream, or none.
// So really, the most minimal layer is one which only defines this function.
// Note: It is the layer implementor's job to make sure that the strings copied
//       do not pass the size limits set in SkLayerProperties. BE CAREFUL!
void SKAPI_CALL skGetLayerProperties_validation(
  SkLayerProperties*                    pProperties
) {
  pProperties->apiVersion = SK_API_VERSION_0_0;
  pProperties->implVersion = SK_MAKE_VERSION(0, 0, 0);
  strcpy(pProperties->layerName, SK_LAYER_OPENSK_VALIDATION_NAME);
  strcpy(pProperties->displayName, SK_LAYER_OPENSK_VALIDATION_DISPLAY_NAME);
  strcpy(pProperties->description, SK_LAYER_OPENSK_VALIDATION_DESCRIPTION);
  memcpy(pProperties->layerUuid, SK_LAYER_OPENSK_VALIDATION_UUID, SK_UUID_SIZE);
}

////////////////////////////////////////////////////////////////////////////////
// SkInstanceLayer
// Note: To be a valid instance layer, you MUST implement skCreateInstance() and
//       skDestroyInstance(). All other instance functions are optional.
////////////////////////////////////////////////////////////////////////////////
static SkResult SKAPI_CALL skCreateInstance_validation(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstance*                           pInstance
) {
  SkResult result;
  SkInstanceLayer layer;

  // Allocate the layer to attach to the whole instance.
  // This is how we will maintain layer-level details, like
  // function pointers and virtual tables for future calls.
  // These details will be filled in after skCreateInstance completes.
  // Note: To be a valid instance layer, any allocations that remain with
  //       the instance MUST be marked SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE.
  layer = skClearAllocate(
    pAllocator,
    sizeof(SkInstanceLayer_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE
  );
  if (!layer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the layer base object information, this must be done before
  // any other changes are made to the object - it creates the vtable.
  result = skInitializeInstanceLayerBase(
    pCreateInfo,
    pAllocator,
    layer,
    SK_LAYER_OPENSK_VALIDATION_UUID,
    pInstance
  );

  return result;
}

static void SKAPI_CALL skDestroyInstance_validation(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator
) {
  SkInstanceLayer layer;
  SkInstanceFunctionTable const* vtable;

  // Get the instance layer - if this doesn't return, something is seriously wrong.
  layer = skGetInstanceLayer(instance, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);

  // Call the underlying destruction routines.
  vtable->pfnDestroyInstance(instance, pAllocator);

  // Other destruction routines can go here.
  skDeinitializeInstanceLayerBase(pAllocator, layer);
  skFree(pAllocator, layer);
}

static SkResult SKAPI_CALL skEnumerateInstanceDrivers_validation(
  SkInstance                            instance,
  uint32_t*                             pDriverCount,
  SkDriver*                            pDriver
) {
  SkResult result;
  SkInstanceFunctionTable const* vtable;
  (void)skGetInstanceLayer(instance, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnEnumerateInstanceDrivers(instance, pDriverCount, pDriver);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// SkDriverLayer
// Note: To be a valid host layer, you MUST implement skCreateDriver() and
//       skDestroyDriver(). All other host functions are optional.
////////////////////////////////////////////////////////////////////////////////
static SkResult SKAPI_CALL skCreateDriver_validation(
  SkDriverCreateInfo const*             pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDriver*                             pDriver
) {
  SkResult result;
  SkDriverLayer layer;

  // Allocate the layer to attach to the whole host.
  // This is how we will maintain layer-level details, like
  // function pointers and virtual tables for future calls.
  // These details will be filled in after skCreateDriver completes.
  // Note: To be a valid host layer, any allocations that remain with
  //       the host MUST be marked SK_SYSTEM_ALLOCATION_SCOPE_DRIVER.
  layer = skClearAllocate(
    pAllocator,
    sizeof(SkDriverLayer_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_DRIVER
  );
  if (!layer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the layer base object information, this must be done before
  // any other changes are made to the object - it creates the vtable.
  result = skInitializeDriverLayerBase(
    pCreateInfo,
    pAllocator,
    layer,
    SK_LAYER_OPENSK_VALIDATION_UUID,
    pDriver
  );

  return result;
}

static void SKAPI_CALL skDestroyDriver_validation(
  SkAllocationCallbacks const*          pAllocator,
  SkDriver                             Driver
) {
  SkDriverLayer layer;
  SkDriverFunctionTable const* vtable;

  // Get the instance layer - if this doesn't return, something is seriously wrong.
  layer = skGetDriverLayer(Driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);

  // Call the underlying destruction routines.
  vtable->pfnDestroyDriver(pAllocator, Driver);

  // Other destruction routines can go here.
  skDeinitializeDriverLayerBase(pAllocator, layer);
  skFree(pAllocator, layer);
}

static void SKAPI_CALL skGetDriverProperties_validation(
  SkDriver                             Driver,
  SkDriverProperties*                  pProperties
) {
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayer(Driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  vtable->pfnGetDriverProperties(Driver, pProperties);
}

static void SKAPI_CALL skGetDriverFeatures_validation(
  SkDriver                             Driver,
  SkDriverFeatures*                    pFeatures
) {
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayer(Driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  vtable->pfnGetDriverFeatures(Driver, pFeatures);
}

static SkResult SKAPI_CALL skEnumerateDriverEndpoints_validation(
  SkDriver                             Driver,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayer(Driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnEnumerateDriverEndpoints(Driver, pEndpointCount, pEndpoints);
  return result;
}

static SkResult SKAPI_CALL skEnumerateDriverDevices_validation(
  SkDriver                             Driver,
  uint32_t*                             pDeviceCount,
  SkDevice*                             pDevices
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayer(Driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnEnumerateDriverDevices(Driver, pDeviceCount, pDevices);
  return result;
}

static SkResult SKAPI_CALL skQueryDeviceProperties_validation(
  SkDevice                              device,
  SkDeviceProperties*                   pProperties
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayerFromDevice(device, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnQueryDeviceProperties(device, pProperties);
  return result;
}

static SkResult SKAPI_CALL skEnumerateDeviceEndpoints_validation(
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayerFromDevice(device, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnEnumerateDeviceEndpoints(device, pEndpointCount, pEndpoints);
  return result;
}

static SkResult SKAPI_CALL skQueryEndpointProperties_validation(
  SkEndpoint                            endpoint,
  SkEndpointProperties*                 pProperties
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayerFromEndpoint(endpoint, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnQueryEndpointProperties(endpoint, pProperties);
  return result;
}

static SkResult SKAPI_CALL skRequestPcmStream_validation(
  SkEndpoint                            endpoint,
  SkPcmStreamRequest const*             pStreamRequest,
  SkPcmStream*                          pStream
) {
  SkResult result;
  SkDriverFunctionTable const* vtable;
  (void)skGetDriverLayerFromEndpoint(endpoint, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnRequestPcmStream(endpoint, pStreamRequest, pStream);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// SkPcmStreamLayer
// Note: To be a valid PCM layer, you MUST implement skCreatePcmStream() and
//       skDestroyPcmStream(). All other PCM functions are optional.
////////////////////////////////////////////////////////////////////////////////
static SkResult SKAPI_CALL skCreatePcmStream_validation(
  SkPcmStreamCreateInfo const*          pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkPcmStream*                          pStream
) {
  SkResult result;
  SkPcmStreamLayer layer;

  // Allocate the layer to attach to the whole PCM stream.
  // This is how we will maintain layer-level details, like
  // function pointers and virtual tables for future calls.
  // These details will be filled in after skCreatePcmStream completes.
  // Note: To be a valid PCM layer, any allocations that remain with
  //       the host MUST be marked SK_SYSTEM_ALLOCATION_SCOPE_STREAM.
  layer = skClearAllocate(
    pAllocator,
    sizeof(SkPcmStreamLayer_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_STREAM
  );
  if (!layer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the layer base object information, this must be done before
  // any other changes are made to the object - it creates the vtable.
  result = skInitializePcmStreamLayerBase(
    pCreateInfo,
    pAllocator,
    layer,
    SK_LAYER_OPENSK_VALIDATION_UUID,
    pStream
  );

  return result;
}

static void SKAPI_CALL skDestroyPcmStream_validation(
  SkPcmStream                           stream,
  SkAllocationCallbacks const*          pAllocator
) {
  SkPcmStreamLayer layer;
  SkPcmStreamFunctionTable const* vtable;
  layer = skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);

  // Call the underlying destruction routines.
  vtable->pfnDestroyPcmStream(stream, pAllocator);

  // Other destruction routines can go here.
  skDeinitializePcmStreamLayerBase(pAllocator, layer);
  skFree(pAllocator, layer);
}

static void SKAPI_CALL skClosePcmStream_validation(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  vtable->pfnClosePcmStream(stream, drain);
}

static SkResult SKAPI_CALL skGetPcmStreamInfo_validation(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkPcmStreamInfo*                      pStreamInfo
) {
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  return vtable->pfnGetPcmStreamInfo(stream, streamType, pStreamInfo);
}

static SkResult SKAPI_CALL skGetPcmStreamChannelMap_validation(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel*                            pChannelMap
) {
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  return vtable->pfnGetPcmStreamChannelMap(stream, streamType, pChannelMap);
}

static SkResult SKAPI_CALL skSetPcmStreamChannelMap_validation(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel const*                      pChannelMap
) {
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  return vtable->pfnSetPcmStreamChannelMap(stream, streamType, pChannelMap);
}

static SkResult SKAPI_CALL skStartPcmStream_validation(
  SkPcmStream                           stream
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnStartPcmStream(stream);
  return result;
}

static SkResult SKAPI_CALL skStopPcmStream_validation(
  SkPcmStream                           stream,
  SkBool32                              drain
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnStopPcmStream(stream, drain);
  return result;
}

static SkResult SKAPI_CALL skRecoverPcmStream_validation(
  SkPcmStream                           stream
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnRecoverPcmStream(stream);
  return result;
}

static SkResult SKAPI_CALL skWaitPcmStream_validation(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  int32_t                               timeout
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnWaitPcmStream(stream, streamType, timeout);
  return result;
}

static SkResult SKAPI_CALL skPausePcmStream_validation(
  SkPcmStream                           stream,
  SkBool32                              pause
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnPausePcmStream(stream, pause);
  return result;
}

static SkResult SKAPI_CALL skAvailPcmStreamSamples_validation(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  uint32_t*                             pSamples
) {
  SkResult result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnAvailPcmStreamSamples(stream, streamType, pSamples);
  return result;
}

static int64_t SKAPI_CALL skWritePcmStreamInterleaved_validation(
  SkPcmStream                           stream,
  void const*                           pBuffer,
  uint32_t                              samples
) {
  int64_t result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnWritePcmStreamInterleaved(stream, pBuffer, samples);
  return result;
}

static int64_t SKAPI_CALL skWritePcmStreamNoninterleaved_validation(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  int64_t result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnWritePcmStreamNoninterleaved(stream, pBuffer, samples);
  return result;
}

static int64_t SKAPI_CALL skReadPcmStreamInterleaved_validation(
  SkPcmStream                           stream,
  void*                                 pBuffer,
  uint32_t                              samples
) {
  int64_t result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnReadPcmStreamInterleaved(stream, pBuffer, samples);
  return result;
}

static int64_t SKAPI_CALL skReadPcmStreamNoninterleaved_validation(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
) {
  int64_t result;
  SkPcmStreamFunctionTable const* vtable;
  (void)skGetPcmStreamLayer(stream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable);
  result = vtable->pfnReadPcmStreamNoninterleaved(stream, pBuffer, samples);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
// Layer Entrypoint
////////////////////////////////////////////////////////////////////////////////

#define HANDLE_PROC(name)                                                       \
if (strcmp(pName, #name) == 0) return (PFN_skVoidFunction)&name##_validation
SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetInstanceProcAddr_validation(
  SkInstance                            instance,
  char const*                           pName
) {
  SkInstanceFunctionTable const* vtable;

  // Instance Core 1.0
  HANDLE_PROC(skGetInstanceProcAddr);
  HANDLE_PROC(skGetLayerProperties);
  HANDLE_PROC(skCreateInstance);
  HANDLE_PROC(skDestroyInstance);
  HANDLE_PROC(skEnumerateInstanceDrivers);

  if (!skGetInstanceLayer(instance, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable)) {
    return NULL;
  }
  return vtable->pfnGetInstanceProcAddr(instance, pName);
}

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr_validation(
  SkDriver                              driver,
  char const*                           pName
) {
  SkDriverFunctionTable const* vtable;

  // Driver Core 1.0
  HANDLE_PROC(skGetDriverProcAddr);
  HANDLE_PROC(skGetLayerProperties);
  HANDLE_PROC(skCreateDriver);
  HANDLE_PROC(skDestroyDriver);
  HANDLE_PROC(skGetDriverProperties);
  HANDLE_PROC(skGetDriverFeatures);
  HANDLE_PROC(skEnumerateDriverDevices);
  HANDLE_PROC(skEnumerateDriverEndpoints);
  HANDLE_PROC(skQueryDeviceProperties);
  HANDLE_PROC(skEnumerateDeviceEndpoints);
  HANDLE_PROC(skQueryEndpointProperties);
  HANDLE_PROC(skRequestPcmStream);
  if (!skGetDriverLayer(driver, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable)) {
    return NULL;
  }
  return vtable->pfnGetDriverProcAddr(driver, pName);
}

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr_validation(
  SkPcmStream                           pcmStream,
  char const*                           pName
) {
  SkPcmStreamFunctionTable const* vtable;

  // PcmStream Core 1.0
  HANDLE_PROC(skGetPcmStreamProcAddr);
  HANDLE_PROC(skGetLayerProperties);
  HANDLE_PROC(skCreatePcmStream);
  HANDLE_PROC(skDestroyPcmStream);
  HANDLE_PROC(skClosePcmStream);
  HANDLE_PROC(skGetPcmStreamInfo);
  HANDLE_PROC(skGetPcmStreamChannelMap);
  HANDLE_PROC(skSetPcmStreamChannelMap);
  HANDLE_PROC(skStartPcmStream);
  HANDLE_PROC(skStopPcmStream);
  HANDLE_PROC(skRecoverPcmStream);
  HANDLE_PROC(skWaitPcmStream);
  HANDLE_PROC(skPausePcmStream);
  HANDLE_PROC(skAvailPcmStreamSamples);
  HANDLE_PROC(skWritePcmStreamInterleaved);
  HANDLE_PROC(skWritePcmStreamNoninterleaved);
  HANDLE_PROC(skReadPcmStreamInterleaved);
  HANDLE_PROC(skReadPcmStreamNoninterleaved);
  if (!skGetPcmStreamLayer(pcmStream, SK_LAYER_OPENSK_VALIDATION_UUID, &vtable)) {
    return NULL;
  }
  return vtable->pfnGetPcmStreamProcAddr(pcmStream, pName);
}
#undef HANDLE_PROC
