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
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef SkResult (SKAPI_PTR *PFN_skHostApiInit)(
  SkInstance                        instance,
  SkHostApi*                        pHostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef void (SKAPI_PTR *PFN_skHostApiFree)(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skScanDevices)(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skGetComponentLimits)(
  SkComponent                       component,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
);

typedef SkResult (SKAPI_PTR *PFN_skRequestStream)(
  SkComponent                       component,
  SkStreamRequest*                  pStreamRequest,
  SkStream*                         pStream,
  const SkAllocationCallbacks*      pAllocator
);

typedef int64_t (SKAPI_PTR *PFN_skStreamWriteInterleaved)(
  SkStream                          stream,
  void const*                       pBuffer,
  uint32_t                          samples
);

typedef int64_t (SKAPI_PTR *PFN_skStreamWriteNoninterleaved)(
  SkStream                          stream,
  void const* const*                pBuffer,
  uint32_t                          samples
);

typedef int64_t (SKAPI_PTR *PFN_skStreamReadInterleaved)(
  SkStream                          stream,
  void*                             pBuffer,
  uint32_t                          samples
);

typedef int64_t (SKAPI_PTR *PFN_skStreamReadNoninterleaved)(
  SkStream                          stream,
  void**                            pBuffer,
  uint32_t                          samples
);

typedef void (SKAPI_PTR *PFN_skDestroyStream)(
  SkStream                          stream,
  SkBool32                          drain
);

#define DECLARE_HOST_API(name)              \
SkResult skHostApiInit_##name(              \
  SkInstance                   instance,    \
  SkHostApi*                   pHostApi,    \
  const SkAllocationCallbacks* pAllocator   \
);

typedef struct SkHostApiImpl {
  PFN_skHostApiFree                         SkHostApiFree;
  PFN_skScanDevices                         SkScanDevices;
  PFN_skGetComponentLimits                  SkGetComponentLimits;
  PFN_skRequestStream                       SkRequestStream;
  PFN_skStreamWriteInterleaved              SkStreamWriteInterleaved;
  PFN_skStreamWriteNoninterleaved           SkStreamWriteNoninterleaved;
  PFN_skStreamReadInterleaved               SkStreamReadInterleaved;
  PFN_skStreamReadNoninterleaved            SkStreamReadNoninterleaved;
  PFN_skDestroyStream                       SkDestroyStream;
} SkHostApiImpl;

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkHostApi_T {
  SkObjectType            objectType;
  SkInstance              instance;
  SkHostApiImpl           impl;
  SkHostApiProperties     properties;
  SkDevice                pDevices;
  SkHostApi               pNext;
} SkHostApi_T;

typedef struct SkDevice_T {
  SkObjectType            objectType;
  SkHostApi               hostApi;
  SkDevice                pNext;
  SkComponent             pComponents;
  SkStream                pStreams;
  SkDeviceProperties      properties;
} SkDevice_T;

typedef struct SkComponent_T {
  SkObjectType            objectType;
  SkComponentProperties   properties;
  SkDevice                device;
  SkStream                pStreams;
  SkComponent             pNext;
} SkComponent_T;

typedef struct SkInstance_T {
  SkObjectType                  objectType;
  SkApplicationInfo             applicationInfo;
  SkInstanceCreateFlags         instanceFlags;
  SkHostApi                     hostApi;
  const SkAllocationCallbacks*  pAllocator;
} SkInstance_T;

typedef struct SkStream_T {
  SkObjectType                  objectType;
  SkStream                      pNext;
  SkComponent                   component;
  SkStreamInfo                  streamInfo;
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
  const SkAllocationCallbacks*  pAllocator,
  SkHostApi*                    pHostApi
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructDevice(
  SkHostApi                     hostApi,
  size_t                        deviceSize,
  SkBool32                      isPhysical,
  const SkAllocationCallbacks*  pAllocator,
  SkDevice*                     pDevice
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructComponent(
  SkDevice                      device,
  size_t                        componentSize,
  const SkAllocationCallbacks*  pAllocator,
  SkComponent*                  pComponent
);

SKAPI_ATTR SkResult SKAPI_CALL skConstructStream(
  SkComponent                   component,
  size_t                        streamSize,
  const SkAllocationCallbacks*  pAllocator,
  SkStream*                     pStream
);

SKAPI_ATTR SkBool32 SKAPI_CALL skEnumerateFormats(
  SkFormat                      seed,
  SkFormat*                     pIterator,
  SkFormat*                     pValue
);
////////////////////////////////////////////////////////////////////////////////
// Host API
////////////////////////////////////////////////////////////////////////////////
DECLARE_HOST_API(ALSA);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_DEV_HOSTS_H
