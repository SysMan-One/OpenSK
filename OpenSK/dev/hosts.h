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
  SkHostApi*                        pHostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef void (SKAPI_PTR *PFN_skHostApiFree)(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skRefreshDevices)(
  SkHostApi                         hostApi,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skRefreshPhysicalComponents)(
  SkPhysicalDevice                  physicalDevice,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skRefreshVirtualComponents)(
  SkVirtualDevice                   virtualDevice,
  const SkAllocationCallbacks*      pAllocator
);

typedef SkResult (SKAPI_PTR *PFN_skGetPhysicalComponentLimits)(
  SkPhysicalComponent               physicalComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
);

typedef SkResult (SKAPI_PTR *PFN_skGetVirtualComponentLimits)(
  SkVirtualComponent                virtualComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
);

#define DECLARE_HOST_API(name)              \
SkResult skHostApiInit_##name(              \
  SkHostApi*                   pHostApi,    \
  const SkAllocationCallbacks* pAllocator   \
);

typedef struct SkHostApiImpl {
  PFN_skHostApiFree                         SkHostApiFree;
  PFN_skRefreshDevices                      SkRefreshPhysicalDevices;
  PFN_skRefreshDevices                      SkRefreshVirtualDevices;
  PFN_skRefreshPhysicalComponents           SkRefreshPhysicalComponents;
  PFN_skRefreshVirtualComponents            SkRefreshVirtualComponents;
  PFN_skGetPhysicalComponentLimits          SkGetPhysicalComponentLimits;
  PFN_skGetVirtualComponentLimits           SkGetVirtualComponentLimits;
} SkHostApiImpl;

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkHostApi_T {
  SkObjectType            objectType;
  SkInstance              instance;
  SkHostApiImpl           impl;
  SkHostApiProperties     properties;
  SkPhysicalDevice        physicalDevices;
  SkVirtualDevice         virtualDevices;
  SkHostApi               pNext;
} SkHostApi_T;

typedef struct SkPhysicalDevice_T {
  SkObjectType            objectType;
  SkHostApi               hostApi;
  SkDeviceProperties      properties;
  SkPhysicalDevice        pNext;
  SkPhysicalComponent     physicalComponents;
} SkPhysicalDevice_T;

typedef struct SkPhysicalComponent_T {
  SkObjectType            objectType;
  SkComponentProperties   properties;
  SkPhysicalDevice        physicalDevice;
  SkPhysicalComponent     pNext;
} SkPhysicalComponent_T;

typedef struct SkVirtualDevice_T {
  SkObjectType            objectType;
  SkHostApi               hostApi;
  SkDeviceProperties      properties;
  SkVirtualDevice         pNext;
  SkVirtualComponent      virtualComponents;
} SkVirtualDevice_T;

typedef struct SkVirtualComponent_T {
  SkObjectType            objectType;
  SkComponentProperties   properties;
  SkVirtualDevice         virtualDevice;
  SkVirtualComponent      pNext;
} SkVirtualComponent_T;

typedef struct SkInstance_T {
  SkObjectType                  objectType;
  SkApplicationInfo             applicationInfo;
  SkInstanceCreateFlags         instanceFlags;
  SkHostApi                     hostApi;
  const SkAllocationCallbacks*  pAllocator;
} SkInstance_T;

////////////////////////////////////////////////////////////////////////////////
// Internal Helper Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skGenerateObjectIdentifier(
  char*                         identifier,
  SkObject                      object,
  uint32_t                      number
);

////////////////////////////////////////////////////////////////////////////////
// Host API
////////////////////////////////////////////////////////////////////////////////
DECLARE_HOST_API(ALSA);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_DEV_HOSTS_H
