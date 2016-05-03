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
SK_DEFINE_HANDLE(SkPhysicalSubcomponent)
SK_DEFINE_HANDLE(SkDevice)

#define SK_TRUE 1
#define SK_FALSE 0
#define SK_MAX_EXTENSION_NAME_SIZE 256
#define SK_MAX_PHYSICAL_DEVICE_NAME_SIZE 256
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
  SK_ERROR_FAILED_ENUMERATING_PHYSICAL_DEVICES
} SkResult;

typedef enum SkAllocationScope {
  SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 0,
  SK_SYSTEM_ALLOCATION_SCOPE_DEVICE
} SkAllocationScope;

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

typedef struct SkPhysicalDeviceProperties {
  SkBool32                          available;
  char                              deviceName[SK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
  char                              driverName[SK_MAX_DRIVER_NAME_SIZE];
  char                              mixerName[SK_MAX_MIXER_NAME_SIZE];
} SkPhysicalDeviceProperties;

typedef struct SkExtensionProperties {
  char                              extensionName[SK_MAX_EXTENSION_NAME_SIZE];
  uint32_t                          specVersion;
} SkExtensionProperties;

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
    SkPhysicalDeviceProperties*       pProperties
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