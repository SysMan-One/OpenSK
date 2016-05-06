/*******************************************************************************
 * selkie - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * SelKie standard include header. (Implementation must adhere to this API)
 ******************************************************************************/
#ifndef SELKIE_H
#define SELKIE_H 1

#include "selkie_platform.h"

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Standard Defines
////////////////////////////////////////////////////////////////////////////////

#define SK_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define SK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#define SK_API_VERSION_0_0 SK_MAKE_VERSION(0, 0, 0)

#define SK_VERSION_MAJOR(version) ((uint32_t)(version) >> 22)
#define SK_VERSION_MINOR(version) ((uint32_t)(version) >> 12 & 0x3ff)
#define SK_VERSION_PATCH(version) ((uint32_t)(version) & 0xfff)
#define SK_NULL_HANDLE 0

typedef uint32_t SkFlags;
typedef uint32_t SkBool32;

SK_DEFINE_HANDLE(SkInstance)
SK_DEFINE_HANDLE(SkPhysicalDevice)
SK_DEFINE_HANDLE(SkPhysicalComponent)
SK_DEFINE_HANDLE(SkVirtualDevice)
SK_DEFINE_HANDLE(SkVirtualComponent)
SK_DEFINE_HANDLE(SkStream)

#define SK_TRUE 1
#define SK_FALSE 0
#define SK_MAX_EXTENSION_NAME_SIZE 256
#define SK_MAX_DEVICE_NAME_SIZE 256
#define SK_MAX_COMPONENT_NAME_SIZE 256
#define SK_MAX_COMPONENT_DESC_SIZE 256
#define SK_MAX_DRIVER_NAME_SIZE 256
#define SK_MAX_MIXER_NAME_SIZE 256

////////////////////////////////////////////////////////////////////////////////
// Standard Types
////////////////////////////////////////////////////////////////////////////////
typedef enum SkResult {
  SK_SUCCESS = 0,
  SK_INCOMPLETE,
  SK_ERROR_OUT_OF_HOST_MEMORY,
  SK_ERROR_INITIALIZATION_FAILED,
  SK_ERROR_EXTENSION_NOT_PRESENT,
  SK_ERROR_FAILED_QUERYING_DEVICE,
  SK_ERROR_FAILED_RESOLVING_DEVICE,
  SK_ERROR_NOT_IMPLEMENTED
} SkResult;

typedef enum SkAllocationScope {
  SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 0,
  SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
} SkAllocationScope;

typedef enum SkRangeDirection {
  SK_RANGE_DIRECTION_UNKNOWN,
  SK_RANGE_DIRECTION_LESS,
  SK_RANGE_DIRECTION_EQUAL,
  SK_RANGE_DIRECTION_GREATER
} SkRangeDirection;

typedef enum SkStreamType {
  SK_STREAM_TYPE_PLAYBACK = 1<<0,
  SK_STREAM_TYPE_CAPTURE  = 1<<1
} SkStreamType;
typedef SkFlags SkStreamTypes;

typedef void* (SKAPI_PTR *PFN_skAllocationFunction)(
  void*                             pUserData,
  size_t                            size,
  size_t                            alignment,
  SkAllocationScope allocationScope
);

typedef void* (SKAPI_PTR *PFN_skFreeFunction)(
  void*                             pUserData,
  void*                             memory
);

typedef SkFlags SkInstanceCreateFlags;

typedef struct SkRangedValue {
  uint32_t                          value;
  SkRangeDirection                  direction;
} SkRangedValue;

typedef struct SkApplicationInfo {
  const char*                       pApplicationName;
  uint32_t                          applicationVersion;
  const char*                       pEngineName;
  uint32_t                          engineVersion;
  uint32_t                          apiVersion;
} SkApplicationInfo;

typedef struct SkInstanceCreateInfo {
  SkInstanceCreateFlags             flags;
  const SkApplicationInfo*          pApplicationInfo;
  uint32_t                          enabledExtensionCount;
  const char* const*                ppEnabledExtensionNames;
} SkInstanceCreateInfo;

typedef struct SkAllocationCallbacks {
  void*                             pUserData;
  PFN_skAllocationFunction          pfnAllocation;
  PFN_skFreeFunction                pfnFree;
} SkAllocationCallbacks;

typedef struct SkExtensionProperties {
  char                              extensionName[SK_MAX_EXTENSION_NAME_SIZE];
  uint32_t                          specVersion;
} SkExtensionProperties;

typedef struct SkDeviceProperties {
  char                              deviceName[SK_MAX_DEVICE_NAME_SIZE];
  char                              driverName[SK_MAX_DRIVER_NAME_SIZE];
  char                              mixerName[SK_MAX_MIXER_NAME_SIZE];
} SkDeviceProperties;

typedef struct SkComponentProperties {
  char                              componentName[SK_MAX_COMPONENT_NAME_SIZE];
  SkStreamTypes                     supportedStreams;
} SkComponentProperties;

typedef struct SkComponentLimits {
  uint32_t                          maxChannels;
  uint32_t                          minChannels;
  uint32_t                          maxFrameSize;
  uint32_t                          minFrameSize;
  SkRangedValue                     maxBufferTime;
  SkRangedValue                     minBufferTime;
  SkRangedValue                     maxPeriodSize;
  SkRangedValue                     minPeriodSize;
  SkRangedValue                     maxPeriodTime;
  SkRangedValue                     minPeriodTime;
  SkRangedValue                     maxPeriods;
  SkRangedValue                     minPeriods;
  SkRangedValue                     maxRate;
  SkRangedValue                     minRate;
} SkComponentLimits;

////////////////////////////////////////////////////////////////////////////////
// Standard Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skEnumerateInstanceExtensionProperties(
  void*                             reserved,
  uint32_t*                         pExtensionCount,
  SkExtensionProperties*            pExtensionProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  const SkInstanceCreateInfo*       pCreateInfo,
  const SkAllocationCallbacks*      pAllocator,
  SkInstance*                       pInstance
);

SKAPI_ATTR void SKAPI_CALL skDestroyInstance(
  SkInstance                        instance
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkInstance                        instance,
  uint32_t*                         pPhysicalDeviceCount,
  SkPhysicalDevice*                 pPhysicalDevices
);

SKAPI_ATTR void SKAPI_CALL skGetPhysicalDeviceProperties(
  SkPhysicalDevice                  physicalDevice,
  SkDeviceProperties*               pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalComponents(
  SkPhysicalDevice                  physicalDevice,
  uint32_t*                         pPhysicalComponentCount,
  SkPhysicalComponent*              pPhysicalComponents
);

SKAPI_ATTR void SKAPI_CALL skGetPhysicalComponentProperties(
  SkPhysicalComponent               physicalComponent,
  SkComponentProperties*            pProperties
);

SKAPI_ATTR void SKAPI_CALL skGetPhysicalComponentLimits(
  SkPhysicalComponent               physicalComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
  SkInstance                        instance,
  uint32_t*                         pVirtualDeviceCount,
  SkVirtualDevice*                  pVirtualDevices
);

SKAPI_ATTR void SKAPI_CALL skGetVirtualDeviceProperties(
  SkVirtualDevice                   virtualDevice,
  SkDeviceProperties*               pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualComponents(
  SkVirtualDevice                   virtualDevice,
  uint32_t*                         pVirtualComponentCount,
  SkVirtualComponent*               pVirtualComponents
);

SKAPI_ATTR void SKAPI_CALL skGetVirtualComponentProperties(
  SkVirtualComponent                virtualComponent,
  SkComponentProperties*            pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skResolvePhysicalComponent(
  SkVirtualComponent                virtualComponent,
  SkStreamType                      streamType,
  SkPhysicalComponent*              pPhysicalComponent
);

SKAPI_ATTR void SKAPI_CALL skResolvePhysicalDevice(
  SkPhysicalComponent               physicalComponent,
  SkPhysicalDevice*                 pPhysicalDevice
);

////////////////////////////////////////////////////////////////////////////////
// Extension: SK_SEA_device_polling
////////////////////////////////////////////////////////////////////////////////
#define SK_SEA_device_polling 1
#define SK_SEA_DEVICE_POLLING_SPEC_VERSION   1
#define SK_SEA_DEVICE_POLLING_EXTENSION_NAME "SK_SEA_device_polling"

typedef enum SkDeviceEventTypeSEA {
  SK_DEVICE_EVENT_ADDED_SEA,
  SK_DEVICE_EVENT_REMOVED_SEA,
  SK_DEVICE_EVENT_ALL_SEA = 0xffff
} SkDeviceEventTypeSEA;

typedef struct SkDeviceEventSEA {
  SkDeviceEventTypeSEA type;
  SkPhysicalDevice physicalDevice;
} SkDeviceEventSEA;

typedef void (SKAPI_PTR *PFN_skDeviceCallbackSEA)(
  void*                             pUserData,
  SkDeviceEventSEA*                 pEvent
);

SKAPI_ATTR SkResult SKAPI_CALL skRegisterDeviceCallbackSEA(
  SkInstance                        instance,
  SkDeviceEventTypeSEA              eventType,
  PFN_skDeviceCallbackSEA           pfnDeviceCallback,
  void*                             pUserData
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // SELKIE_H
