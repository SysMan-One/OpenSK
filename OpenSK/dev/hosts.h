/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_HOSTS_H
#define   OPENSK_DEV_HOSTS_H 1

#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Host API
////////////////////////////////////////////////////////////////////////////////
typedef enum SkProcedure {
  SK_PROC_NONE,
  SK_PROC_HOSTAPI_INIT,
  SK_PROC_HOSTAPI_FREE,
  SK_PROC_HOSTAPI_SCAN,
  SK_PROC_COMPONENT_GET_LIMITS, // Deprecated
  SK_PROC_STREAM_REQUEST,
  SK_PROC_STREAM_GET_HANDLE,
  SK_PROC_STREAM_DESTROY
} SkProcedure;

typedef SkResult (SKAPI_PTR *PFN_skProcedure)(SkProcedure procedure, void** params);
#define DECLARE_HOST_API(name) SkResult skProcedure_##name(SkProcedure procedure, void** params);

// Parameter Unpacking
#define SK_PARAMS_HOSTAPI_INIT (SkInstance)params[0], (SkHostApi*)params[1]
#define SK_PARAMS_HOSTAPI_SCAN (SkHostApi)params[0]
#define SK_PARAMS_HOSTAPI_FREE (SkHostApi)params[0]
#define SK_PARAMS_COMPONENT_LIMITS (SkComponent)params[0], *(SkStreamType*)params[1], (SkComponentLimits*)params[2]
#define SK_PARAMS_STREAM_REQUEST (SkComponent)params[0], (SkStreamRequest*)params[1], (SkStream*)params[2]
#define SK_PARAMS_STREAM_GET_HANDLE (SkStream)params[0], (void**)&params[1]

// Critical Functions
typedef int64_t (SKAPI_PTR *PFN_skStreamWriteInterleaved)(SkStream stream, void const* pBuffer, uint32_t samples);
typedef int64_t (SKAPI_PTR *PFN_skStreamWriteNoninterleaved)(SkStream stream, void const* const* pBuffer, uint32_t samples);
typedef int64_t (SKAPI_PTR *PFN_skStreamReadInterleaved)(SkStream stream, void* pBuffer, uint32_t samples);
typedef int64_t (SKAPI_PTR *PFN_skStreamReadNoninterleaved)(SkStream stream, void** pBuffer, uint32_t samples);

// Host API list
DECLARE_HOST_API(ALSA);

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef union SkPcmFunctions_T {
  PFN_skStreamWriteInterleaved      SkStreamWriteInterleaved;
  PFN_skStreamWriteNoninterleaved   SkStreamWriteNoninterleaved;
  PFN_skStreamReadInterleaved       SkStreamReadInterleaved;
  PFN_skStreamReadNoninterleaved    SkStreamReadNoninterleaved;
} SkPcmFunctions_T;

typedef struct SkInstance_T {
  SkObjectType                  objectType;
  SkApplicationInfo             applicationInfo;
  SkInstanceCreateFlags         instanceFlags;
  SkHostApi                     hostApi;
  const SkAllocationCallbacks*  pAllocator;
} SkInstance_T;

typedef struct SkHostApi_T {
  SkObjectType                  objectType;
  SkInstance                    instance;
  PFN_skProcedure               proc;
  SkHostApiProperties           properties;
  SkDevice                      pDevices;
  SkHostApi                     pNext;
  const SkAllocationCallbacks*  pAllocator;
} SkHostApi_T;

typedef struct SkDevice_T {
  SkObjectType                  objectType;
  SkHostApi                     hostApi;
  PFN_skProcedure               proc;
  SkDevice                      pNext;
  SkComponent                   pComponents;
  SkStream                      pStreams;
  SkDeviceProperties            properties;
  const SkAllocationCallbacks*  pAllocator;
} SkDevice_T;

typedef struct SkComponent_T {
  SkObjectType                  objectType;
  SkComponentProperties         properties;
  PFN_skProcedure               proc;
  SkDevice                      device;
  SkStream                      pStreams;
  SkComponent                   pNext;
  const SkAllocationCallbacks*  pAllocator;
} SkComponent_T;

typedef struct SkStream_T {
  SkObjectType                  objectType;
  SkPcmFunctions_T              pcmFunctions;
  SkStream                      pNext;
  PFN_skProcedure               proc;
  SkComponent                   component;
  SkStreamInfo                  streamInfo;
  const SkAllocationCallbacks*  pAllocator;
} SkStream_T;

////////////////////////////////////////////////////////////////////////////////
// Internal Helper Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skGenerateDeviceIdentifier(
  char*                         identifier,
  SkDevice                      device
);

SKAPI_ATTR SkResult SKAPI_CALL skGenerateComponentIdentifier(
  char*                         identifier,
  SkComponent                   component
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructHostApi(
  SkInstance                    instance,
  size_t                        hostApiSize,
  PFN_skProcedure               proc,
  SkHostApi*                    pHostApi
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructDevice(
  SkHostApi                     hostApi,
  size_t                        deviceSize,
  SkBool32                      isPhysical,
  SkDevice*                     pDevice
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructComponent(
  SkDevice                      device,
  size_t                        componentSize,
  SkComponent*                  pComponent
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructStream(
  SkComponent                   component,
  size_t                        streamSize,
  SkStream*                     pStream
);

SKAPI_ATTR SkBool32 SKAPI_CALL skEnumerateFormats(
  SkFormat                      seed,
  SkFormat*                     pIterator,
  SkFormat*                     pValue
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_DEV_HOSTS_H
