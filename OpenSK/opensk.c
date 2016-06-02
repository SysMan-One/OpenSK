/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard include header. (Implementation must adhere to this API)
 ******************************************************************************/

#include "opensk.h"
#include "dev/hosts.h"
#include "dev/macros.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
// Internal Types
////////////////////////////////////////////////////////////////////////////////
void* SkDefaultAllocationFunction(
  void*               pUserData,
  size_t              size,
  size_t              alignment,
  SkAllocationScope   scope
) {
  (void)scope;
  (void)pUserData;
  return aligned_alloc(alignment, size);
}

void* SkDefaultFreeFunction(
  void*               pUserData,
  void*               memory
) {
  (void)pUserData;
  free(memory);
}

SkAllocationCallbacks SkDefaultAllocationCallbacks = {
  NULL,
  &SkDefaultAllocationFunction,
  &SkDefaultFreeFunction
};

#define BIND_HOSTAPI(name) &skHostApiInit_##name
static const PFN_skHostApiInit hostApiInitializers[] = {
#ifdef HOST_ALSA
  BIND_HOSTAPI(ALSA),
#endif
  NULL
};
#undef BIND_HOSTAPI

static union { uint32_t u32; unsigned char u8[4]; } intEndianCheck = { (uint32_t)0x01234567 };
static int
skIsIntegerBigEndianIMPL() {
  return intEndianCheck.u8[3] == 0x01;
}

static union { float f32; unsigned char u8[4]; } floatEndianCheck = { (float)0x01234567 };
static int
skIsFloatBigEndianIMPL() {
  return floatEndianCheck.u8[3] == 0x01;
}
////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////
static int
pathcmp(char const* lhs, char const* rhs) {
  while (*lhs && *rhs && *lhs == *rhs && *lhs != '/') {
    ++lhs; ++rhs;
  }
  return ((*lhs == '\0' || *rhs == '/') && (*rhs == '\0' || *rhs == '/')) ? 1 : 0;
}

#define ptrdiff(lhs, rhs) (((unsigned char*)lhs) - ((unsigned char*)rhs))
static int
_lltoa(char *begin, long long value) {
  char tmp;
  long long prev;
  char *end = begin;
  size_t bytes;

  // Write the integer to the destination
  do {
    prev = value;
    value /= 10;
    *end++ = "9876543210123456789"[9 + prev - value * 10];
  } while (value != 0);

  // Handle negatives
  if (prev < 0) {
    *end++ = '-';
  }

  // Apply the null-termination
  *end = '\0';
  bytes = ptrdiff(end--, begin);

  // Reverse the integer
  while (begin < end) {
    tmp = *begin;
    *begin++ = *end;
    *end-- = tmp;
  }

  return (int)bytes;
}

static int
itoa(char *dest, int value) {
  return _lltoa(dest, value);
}

static int
ltoa(char *dest, long value) {
  return _lltoa(dest, value);
}

static int
lltoa(char *dest, long long value) {
  return _lltoa(dest, value);
}

#define ALPHABET_CHARACTERS 26
static SkResult
skGenerateObjectIdentifierIMPL(char *buffer, char const *prefix, uint32_t number) {
  uint32_t tmpA, tmpB;
  uint32_t length = (uint32_t)strlen(prefix);
  if (number < ALPHABET_CHARACTERS) {
    memcpy(buffer, prefix, length);
    buffer[length] = (char)('a' + number);
    length += 1;
  }
  else if (number < ALPHABET_CHARACTERS * ALPHABET_CHARACTERS) {
    memcpy(buffer, prefix, length);
    buffer[length + 0] = (char)('a' + (number / ALPHABET_CHARACTERS) - 1);
    buffer[length + 1] = (char)('a' + (number % ALPHABET_CHARACTERS));
    length += 2;
  }
  else if (number < ALPHABET_CHARACTERS * ALPHABET_CHARACTERS * ALPHABET_CHARACTERS) {
    memcpy(buffer, prefix, length);
    tmpA = number / (ALPHABET_CHARACTERS * ALPHABET_CHARACTERS);
    tmpB = number - (tmpA * ALPHABET_CHARACTERS * ALPHABET_CHARACTERS);
    buffer[length + 0] = (char)('a' + tmpA - 1);
    buffer[length + 1] = (char)('a' + tmpB / (ALPHABET_CHARACTERS) - 1);
    buffer[length + 2] = (char)('a' + ((number - tmpB) % ALPHABET_CHARACTERS));
    length += 3;
  }
  else {
    // Anything past three identifiers isn't supported
    return SK_ERROR_INITIALIZATION_FAILED;
  }
  buffer[length + 1] = '\0';
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL
skGenerateObjectIdentifier(char *identifier, SkObject object, uint32_t number) {
  switch (skGetObjectType(object)) {
    case SK_OBJECT_TYPE_PHYSICAL_DEVICE:
      return skGenerateObjectIdentifierIMPL(identifier, "pd", number);
    case SK_OBJECT_TYPE_VIRTUAL_DEVICE:
      return skGenerateObjectIdentifierIMPL(identifier, "vd", number);
    case SK_OBJECT_TYPE_PHYSICAL_COMPONENT:
      strcpy(identifier, ((SkPhysicalComponent)object)->physicalDevice->properties.identifier);
      itoa(&identifier[strlen(identifier)], number);
      break;
    case SK_OBJECT_TYPE_VIRTUAL_COMPONENT:
      strcpy(identifier, ((SkVirtualComponent)object)->virtualDevice->properties.identifier);
      itoa(&identifier[strlen(identifier)], number);
      break;
    default:
      return SK_ERROR_INITIALIZATION_FAILED;
  }
  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  const SkInstanceCreateInfo*       pCreateInfo,
  const SkAllocationCallbacks*      pAllocator,
  SkInstance*                       pInstance
) {
  SkResult result = SK_SUCCESS;
  SkResult opResult;
  SkHostApi hostApi;
  const PFN_skHostApiInit* hostApiInitializer;
  if (!pAllocator) pAllocator = &SkDefaultAllocationCallbacks;

  // Allocate all of the required fields of SkInstance_T
  SkInstance instance = SKALLOC(sizeof(SkInstance_T), SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
  if (!instance) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  memset(instance, 0, sizeof(SkInstance_T));

  // Initialize the Instance's public data
  instance->objectType = SK_OBJECT_TYPE_INSTANCE;
  instance->pAllocator = pAllocator;
  instance->applicationInfo = *pCreateInfo->pApplicationInfo;
  instance->instanceFlags = pCreateInfo->flags;

  // Initialize all compiled and supported SkHostApi information
  hostApiInitializer = hostApiInitializers;
  while (*hostApiInitializer) {
    opResult = (*hostApiInitializer)(&hostApi, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
    else {
      hostApi->instance = instance;
      hostApi->pNext = instance->hostApi;
      instance->hostApi = hostApi;
      if (instance->instanceFlags & SK_INSTANCE_REFRESH_ON_CREATE) {
        skRefreshDeviceList(hostApi);
      }
    }
    ++hostApiInitializer;
  }

  *pInstance = instance;
  return result;
}

SKAPI_ATTR void SKAPI_CALL skDestroyInstance(
  SkInstance                        instance
) {
  SkHostApi hostApi;
  SkHostApi nextHostApi;

  // Delete the Host API internals.
  hostApi = instance->hostApi;
  while (hostApi) {
    nextHostApi = hostApi->pNext;
    hostApi->impl.SkHostApiFree(hostApi, instance->pAllocator);
    hostApi = nextHostApi;
  }
  instance->hostApi = SK_NULL_HANDLE;

  _SKFREE(instance->pAllocator, instance);
}

#define ADVANCE(n) ((path[n] == '/') ? &path[n + 1] : &path[n])
#define CHECK_END() (*path == '\0')
SKAPI_ATTR SkObject SKAPI_CALL skRequestObject(
  SkInstance                        instance,
  char const*                       path
) {
  SkResult result;
  SkHostApi hostApi;
  SkObject deviceOrComponent;
  SkPhysicalDevice physicalDevice;
  SkPhysicalComponent physicalComponent;
  SkVirtualDevice virtualDevice;
  SkVirtualComponent virtualComponent;
  SkHostApiProperties properties;

  // Check for valid sk path prefix
  if (strncmp(path, "sk://", 5) != 0) {
    return SK_NULL_HANDLE;
  }

  // Check if the path should resolve to the instance
  path = ADVANCE(5);
  if (CHECK_END()) {
    return (SkObject)instance;
  }

  // Search the list of host APIs
  hostApi = instance->hostApi;
  while (hostApi) {
    skGetHostApiProperties(hostApi, &properties);
    if (pathcmp(properties.identifier, path)) {
      break;
    }
    hostApi = hostApi->pNext;
  }

  // Check that the host API is valid
  if (!hostApi) {
    return SK_NULL_HANDLE;
  }

  // Check if the path should resolve to the host
  path = ADVANCE(strlen(properties.identifier));
  if (CHECK_END()) {
    return (SkObject)hostApi;
  }

  deviceOrComponent = NULL;
  {
    // Check for physical devices and components
    result = hostApi->impl.SkRefreshPhysicalDevices(hostApi, hostApi->instance->pAllocator);
    if (result == SK_SUCCESS || result == SK_INCOMPLETE) {
      physicalDevice = hostApi->physicalDevices;
      while (physicalDevice) {
        if (pathcmp(physicalDevice->properties.identifier, path)) {
          deviceOrComponent = (SkObject)physicalDevice;
          break;
        }
        physicalComponent = physicalDevice->physicalComponents;
        while (physicalComponent) {
          if (pathcmp(physicalComponent->properties.identifier, path)) {
            break;
          }
          physicalComponent = physicalComponent->pNext;
        }
        if (physicalComponent) {
          deviceOrComponent = (SkObject)physicalComponent;
          break;
        }
        physicalDevice = physicalDevice->pNext;
      }
      if (deviceOrComponent) {
        return deviceOrComponent;
      }
    }

    // Check for virtual devices and components
    result = hostApi->impl.SkRefreshVirtualDevices(hostApi, hostApi->instance->pAllocator);
    if (result == SK_SUCCESS || result == SK_INCOMPLETE) {
      virtualDevice = hostApi->virtualDevices;
      while (virtualDevice) {
        if (pathcmp(virtualDevice->properties.identifier, path)) {
          deviceOrComponent = (SkObject)virtualDevice;
          break;
        }
        virtualComponent = virtualDevice->virtualComponents;
        while (virtualComponent) {
          if (pathcmp(virtualComponent->properties.identifier, path)) {
            break;
          }
          virtualComponent = virtualComponent->pNext;
        }
        if (virtualComponent) {
          deviceOrComponent = (SkObject)virtualComponent;
          break;
        }
        virtualDevice = virtualDevice->pNext;
      }
      if (deviceOrComponent) {
        return deviceOrComponent;
      }
    }
  }

  // No valid identifier found
  return SK_NULL_HANDLE;
}
#undef CHECK_END
#undef ADVANCE

SKAPI_ATTR SkObjectType SKAPI_CALL skGetObjectType(
  SkObject                          object
) {
  return *((SkObjectType*)object);
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateHostApi(
  SkInstance                        instance,
  uint32_t*                         pHostApiCount,
  SkHostApi*                        pHostApi
) {
  uint32_t count = 0;
  SkHostApi hostApi = instance->hostApi;
  while (hostApi) {
    if (pHostApi) {
      if (count >= *pHostApiCount) break;
      *pHostApi = hostApi;
      ++pHostApi;
    }
    ++count;
    hostApi = hostApi->pNext;
  }
  *pHostApiCount = count;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skGetHostApiProperties(
  SkHostApi                         hostApi,
  SkHostApiProperties*              pProperties
) {
  *pProperties = hostApi->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skRefreshPhysicalDeviceList(
  SkHostApi                         hostApi
) {
  return hostApi->impl.SkRefreshPhysicalDevices(hostApi, hostApi->instance->pAllocator);
}

SKAPI_ATTR SkResult SKAPI_CALL skRefreshVirtualDeviceList(
  SkHostApi                         hostApi
) {
  return hostApi->impl.SkRefreshVirtualDevices(hostApi, hostApi->instance->pAllocator);
}

SKAPI_ATTR SkResult SKAPI_CALL skRefreshDeviceList(
  SkHostApi                         hostApi
) {
  SkResult opResult;
  SkResult result = SK_SUCCESS;
  opResult = skRefreshPhysicalDeviceList(hostApi);
  if (opResult != SK_SUCCESS) {
    result = opResult;
  }
  opResult = skRefreshVirtualDeviceList(hostApi);
  if (opResult != SK_SUCCESS) {
    result = opResult;
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pPhysicalDeviceCount,
  SkPhysicalDevice*                 pPhysicalDevices
) {
  uint32_t count = 0;
  SkResult result = SK_SUCCESS;
  SkPhysicalDevice physicalDevice;

  // Refresh device list (if flag is set)
  if (hostApi->instance->instanceFlags & SK_INSTANCE_REFRESH_ON_ENUMERATE) {
    result = skRefreshPhysicalDeviceList(hostApi);
    if (result != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }

  // Copy physical devices into output variables
  physicalDevice = hostApi->physicalDevices;
  while (physicalDevice) {
    if (pPhysicalDevices) {
      if (count >= *pPhysicalDeviceCount) break;
      *pPhysicalDevices = physicalDevice;
      ++pPhysicalDevices;
    }
    ++count;
    physicalDevice = physicalDevice->pNext;
  }
  *pPhysicalDeviceCount = count;

  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pVirtualDeviceCount,
  SkVirtualDevice*                  pVirtualDevices
) {
  uint32_t count = 0;
  SkResult result = SK_SUCCESS;
  SkVirtualDevice virtualDevice;

  // Refresh device list
  if (hostApi->instance->instanceFlags & SK_INSTANCE_REFRESH_ON_ENUMERATE) {
    result = skRefreshVirtualDeviceList(hostApi);
    if (result != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }

  // Copy physical devices into output variables
  virtualDevice = hostApi->virtualDevices;
  while (virtualDevice) {
    if (pVirtualDevices) {
      if (count >= *pVirtualDeviceCount) break;
      *pVirtualDevices = virtualDevice;
      ++pVirtualDevices;
    }
    ++count;
    virtualDevice = virtualDevice->pNext;
  }
  *pVirtualDeviceCount = count;

  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pDeviceCount,
  SkObject*                         pDevices
) {
  SkResult result = SK_SUCCESS;
  SkResult opResult;
  uint32_t physicalDevice;
  uint32_t virtualDevice;

  if (!pDevices) {
    opResult = skEnumeratePhysicalDevices(hostApi, &physicalDevice, NULL);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
    opResult = skEnumerateVirtualDevices(hostApi, &virtualDevice, NULL);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }
  else {
    physicalDevice = *pDeviceCount;
    opResult = skEnumeratePhysicalDevices(hostApi, &physicalDevice, (SkPhysicalDevice*)pDevices);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
    virtualDevice = *pDeviceCount - physicalDevice;
    opResult = skEnumerateVirtualDevices(hostApi, &virtualDevice, (SkVirtualDevice*)&pDevices[physicalDevice]);
    if (opResult != SK_SUCCESS) {
      result = opResult;
    }
  }
  *pDeviceCount = physicalDevice + virtualDevice;

  return result;
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalDeviceProperties(
  SkPhysicalDevice                  physicalDevice,
  SkDeviceProperties*               pProperties
) {
  *pProperties = physicalDevice->properties;
}

SKAPI_ATTR void SKAPI_CALL skGetVirtualDeviceProperties(
  SkVirtualDevice                   virtualDevice,
  SkDeviceProperties*               pProperties
) {
  *pProperties = virtualDevice->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skGetDeviceProperties(
  SkObject                          device,
  SkDeviceProperties*               pProperties
) {
  switch (skGetObjectType((SkObject)device)) {
    case SK_OBJECT_TYPE_PHYSICAL_DEVICE:
      skGetPhysicalDeviceProperties((SkPhysicalDevice)device, pProperties);
      break;
    case SK_OBJECT_TYPE_VIRTUAL_DEVICE:
      skGetVirtualDeviceProperties((SkVirtualDevice)device, pProperties);
      break;
    default:
      return SK_ERROR_INVALID_OBJECT;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalComponents(
  SkPhysicalDevice                  physicalDevice,
  uint32_t*                         pPhysicalComponentCount,
  SkPhysicalComponent*              pPhysicalComponents
) {
  uint32_t count = 0;
  SkResult result = SK_SUCCESS;
  SkPhysicalComponent physicalComponent;

  // Refresh device list
  if (physicalDevice->hostApi->instance->instanceFlags & SK_INSTANCE_REFRESH_ON_COMPONENT_ENUMERATE) {
    result = physicalDevice->hostApi->impl.SkRefreshPhysicalComponents(physicalDevice, physicalDevice->hostApi->instance->pAllocator);
    if (result != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }

  // Copy physical devices into output variables
  physicalComponent = physicalDevice->physicalComponents;
  while (physicalComponent) {
    if (pPhysicalComponents) {
      if (count >= *pPhysicalComponentCount) break;
      *pPhysicalComponents = physicalComponent;
      ++pPhysicalComponents;
    }
    ++count;
    physicalComponent = physicalComponent->pNext;
  }
  *pPhysicalComponentCount = count;

  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualComponents(
  SkVirtualDevice                   virtualDevice,
  uint32_t*                         pVirtualComponentCount,
  SkVirtualComponent*               pVirtualComponents
) {
  uint32_t count = 0;
  SkResult result = SK_SUCCESS;
  SkVirtualComponent virtualComponent;

  // Refresh device list
  if (virtualDevice->hostApi->instance->instanceFlags & SK_INSTANCE_REFRESH_ON_COMPONENT_ENUMERATE) {
    result = virtualDevice->hostApi->impl.SkRefreshVirtualComponents(virtualDevice, virtualDevice->hostApi->instance->pAllocator);
    if (result != SK_SUCCESS) {
      result = SK_INCOMPLETE;
    }
  }

  // Copy physical devices into output variables
  virtualComponent = virtualDevice->virtualComponents;
  while (virtualComponent) {
    if (pVirtualComponents) {
      if (count >= *pVirtualComponentCount) break;
      *pVirtualComponents = virtualComponent;
      ++pVirtualComponents;
    }
    ++count;
    virtualComponent = virtualComponent->pNext;
  }
  *pVirtualComponentCount = count;

  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateComponents(
  SkObject                          device,
  uint32_t*                         pComponentCount,
  SkObject*                         pComponents
) {
  switch (skGetObjectType(device)) {
    case SK_OBJECT_TYPE_PHYSICAL_DEVICE:
      return skEnumeratePhysicalComponents((SkPhysicalDevice)device, pComponentCount, (SkPhysicalComponent*)pComponents);
    case SK_OBJECT_TYPE_VIRTUAL_DEVICE:
      return skEnumerateVirtualComponents((SkVirtualDevice)device, pComponentCount, (SkVirtualComponent*)pComponents);
    default:
      return SK_ERROR_INVALID_OBJECT;
  }
}

SKAPI_ATTR void SKAPI_CALL skGetPhysicalComponentProperties(
  SkPhysicalComponent               physicalComponent,
  SkComponentProperties*            pProperties
) {
  *pProperties = physicalComponent->properties;
}


SKAPI_ATTR void SKAPI_CALL skGetVirtualComponentProperties(
  SkVirtualComponent                virtualComponent,
  SkComponentProperties*            pProperties
) {
  *pProperties = virtualComponent->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skGetComponentProperties(
  SkObject                          component,
  SkComponentProperties*            pProperties
) {
  switch (skGetObjectType(component)) {
    case SK_OBJECT_TYPE_PHYSICAL_COMPONENT:
      skGetPhysicalComponentProperties((SkPhysicalComponent)component, pProperties);
      break;
    case SK_OBJECT_TYPE_VIRTUAL_COMPONENT:
      skGetVirtualComponentProperties((SkVirtualComponent)component, pProperties);
      break;
    default:
      return SK_ERROR_INVALID_OBJECT;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skGetPhysicalComponentLimits(
  SkPhysicalComponent               physicalComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  return physicalComponent->physicalDevice->hostApi->impl.SkGetPhysicalComponentLimits(
    physicalComponent, streamType, pLimits
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skGetVirtualComponentLimits(
  SkVirtualComponent                virtualComponent,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  return virtualComponent->virtualDevice->hostApi->impl.SkGetVirtualComponentLimits(
      virtualComponent, streamType, pLimits
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skGetComponentLimits(
  SkObject                          component,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  switch (skGetObjectType(component)) {
    case SK_OBJECT_TYPE_PHYSICAL_COMPONENT:
      return skGetPhysicalComponentLimits((SkPhysicalComponent)component, streamType, pLimits);
    case SK_OBJECT_TYPE_VIRTUAL_COMPONENT:
      return skGetVirtualComponentLimits((SkVirtualComponent)component, streamType, pLimits);
    default:
      return SK_ERROR_INVALID_OBJECT;
  }
}

#define PRINT_CASE(e) case e: return #e
SKAPI_ATTR char const* SKAPI_CALL skGetStreamTypeString(
  SkStreamType                      streamType
) {
  switch (streamType) {
    PRINT_CASE(SK_STREAM_TYPE_PCM_PLAYBACK);
    PRINT_CASE(SK_STREAM_TYPE_PCM_CAPTURE);
    default: return NULL;
  }
}

SKAPI_ATTR char const* SKAPI_CALL skGetFormatString(
  SkFormat                          format
) {
  switch (format) {
    PRINT_CASE(SK_FORMAT_UNKNOWN);
    PRINT_CASE(SK_FORMAT_ANY);
    PRINT_CASE(SK_FORMAT_S8);
    PRINT_CASE(SK_FORMAT_U8);
    PRINT_CASE(SK_FORMAT_S16_LE);
    PRINT_CASE(SK_FORMAT_S16_BE);
    PRINT_CASE(SK_FORMAT_U16_LE);
    PRINT_CASE(SK_FORMAT_U16_BE);
    PRINT_CASE(SK_FORMAT_S24_LE);
    PRINT_CASE(SK_FORMAT_S24_BE);
    PRINT_CASE(SK_FORMAT_U24_LE);
    PRINT_CASE(SK_FORMAT_U24_BE);
    PRINT_CASE(SK_FORMAT_S32_LE);
    PRINT_CASE(SK_FORMAT_S32_BE);
    PRINT_CASE(SK_FORMAT_U32_LE);
    PRINT_CASE(SK_FORMAT_U32_BE);
    PRINT_CASE(SK_FORMAT_FLOAT_LE);
    PRINT_CASE(SK_FORMAT_FLOAT_BE);
    PRINT_CASE(SK_FORMAT_FLOAT64_LE);
    PRINT_CASE(SK_FORMAT_FLOAT64_BE);
    PRINT_CASE(SK_FORMAT_S16);
    PRINT_CASE(SK_FORMAT_U16);
    PRINT_CASE(SK_FORMAT_S24);
    PRINT_CASE(SK_FORMAT_U24);
    PRINT_CASE(SK_FORMAT_S32);
    PRINT_CASE(SK_FORMAT_U32);
    PRINT_CASE(SK_FORMAT_FLOAT);
    PRINT_CASE(SK_FORMAT_FLOAT64);
    default: return NULL;
  }
}

SKAPI_ATTR SkFormat SKAPI_CALL skGetFormatStatic(
  SkFormat                          format
) {
  switch (format) {
    case SK_FORMAT_S16:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_S16_BE : SK_FORMAT_S16_LE;
    case SK_FORMAT_U16:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_U16_BE : SK_FORMAT_U16_LE;
    case SK_FORMAT_S24:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_S24_BE : SK_FORMAT_S24_LE;
    case SK_FORMAT_U24:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_U24_BE : SK_FORMAT_U24_LE;
    case SK_FORMAT_S32:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_S32_BE : SK_FORMAT_S32_LE;
    case SK_FORMAT_U32:
      return (skIsIntegerBigEndianIMPL()) ? SK_FORMAT_U32_BE : SK_FORMAT_U32_LE;
    case SK_FORMAT_FLOAT:
      return (skIsFloatBigEndianIMPL()) ? SK_FORMAT_FLOAT_BE : SK_FORMAT_FLOAT_LE;
    case SK_FORMAT_FLOAT64:
      return (skIsFloatBigEndianIMPL()) ? SK_FORMAT_FLOAT64_BE : SK_FORMAT_FLOAT64_LE;
    default:
      return format;
  }
}