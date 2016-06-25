/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard include header. (Implementation must adhere to this API)
 ******************************************************************************/

#include "opensk.h"
#include "dev/assert.h"
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
skGenerateDeviceIdentifier(char *identifier, SkDevice device) {
  uint32_t count;
  SkDevice countDevice;
  SKASSERT(identifier != NULL, "Must have a location to write the identifier to.\n");
  SKASSERT(device != NULL, "Must have a device to generate an identifier for.\n");
  SKASSERT(device->hostApi, "hostAPI must be set on the device to generate an identifier.\n");
  SKASSERT(
    device->properties.isPhysical == SK_TRUE || device->properties.isPhysical == SK_FALSE,
    "The device isPhysical flag must be set to either SK_TRUE or SK_FALSE.\n"
  );

  // Count the number of existing devices
  count = 0;
  countDevice = device->hostApi->pDevices;
  while (countDevice) {
    SKASSERT(countDevice != device, "The identifier must be generated prior to adding the device to the host.\n");
    if (countDevice->properties.isPhysical == device->properties.isPhysical) {
      ++count;
    }
    countDevice = countDevice->pNext;
  }

  return skGenerateObjectIdentifierIMPL(identifier, (device->properties.isPhysical) ? "pd" : "vd", count);
}

SKAPI_ATTR SkResult SKAPI_CALL
skGenerateComponentIdentifier(char *identifier, SkComponent component) {
  uint32_t count;
  SkComponent countComponent;
  SKASSERT(identifier != NULL, "Must have a location to write the identifier to.\n");
  SKASSERT(component != NULL, "Must have a component to generate an identifier for.\n");
  SKASSERT(component->device, "device must be set on the component to generate an identifier.\n");
  SKASSERT(
    strlen(component->device->properties.identifier) > 0,
    "The component's device must have already had it's identifier generated.\n"
  );

  // Count the number of existing components
  count = 0;
  countComponent = component->device->pComponents;
  while (countComponent) {
    SKASSERT(countComponent != component, "The identifier must be generated prior to adding the component to the device.\n");
    ++count;
    countComponent = countComponent->pNext;
  }

  // This can't fail, just assign the device's identifier and then add the count
  strcpy(identifier, component->device->properties.identifier);
  itoa(&identifier[strlen(identifier)], count);
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL
skConstructHostApi(SkInstance instance, size_t hostApiSize, const SkAllocationCallbacks* pAllocator, SkHostApi* pHostApi) {
  SkHostApi hostApi;

  // Allocate the hostApi
  hostApi = SKALLOC(hostApiSize, SK_SYSTEM_ALLOCATION_SCOPE_HOST_API);
  if (!hostApi) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Fill in the HostApi external information
  memset(hostApi, 0, hostApiSize);
  hostApi->objectType = SK_OBJECT_TYPE_HOST_API;
  hostApi->instance = instance;

  // Add to the instance
  hostApi->pNext = instance->hostApi;
  instance->hostApi = hostApi;

  *pHostApi = hostApi;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL
skConstructDevice(SkHostApi hostApi, size_t deviceSize, SkBool32 isPhysical, const SkAllocationCallbacks* pAllocator, SkDevice *pDevice) {
  SkResult result;
  SkDevice device;

  // Allocate the device
  device = SKALLOC(deviceSize, SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
  if (!device) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Fill in the OpenSK external information
  memset(device, 0, deviceSize);
  device->objectType = SK_OBJECT_TYPE_DEVICE;
  device->hostApi = hostApi;
  device->properties.isPhysical = isPhysical;
  result = skGenerateDeviceIdentifier(device->properties.identifier, device);
  if (result != SK_SUCCESS) {
    SKFREE(device);
    return result;
  }

  // Add to the hostApi
  device->pNext = hostApi->pDevices;
  hostApi->pDevices = device;

  *pDevice = device;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL
skConstructComponent(SkDevice device, size_t componentSize, const SkAllocationCallbacks* pAllocator, SkComponent *pComponent) {
  SkResult result;
  SkComponent component;

  // Allocate the component
  component = SKALLOC(componentSize, SK_SYSTEM_ALLOCATION_SCOPE_DEVICE);
  if (!component) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Fill in the OpenSK external information
  memset(component, 0, componentSize);
  component->objectType = SK_OBJECT_TYPE_COMPONENT;
  component->device = device;
  result = skGenerateComponentIdentifier(component->properties.identifier, component);
  if (result != SK_SUCCESS) {
    SKFREE(component);
    return result;
  }

  // Add to the device
  component->pNext = device->pComponents;
  device->pComponents = component;

  *pComponent = component;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skConstructStream(
  SkComponent                   component,
  size_t                        streamSize,
  const SkAllocationCallbacks*  pAllocator,
  SkStream*                     pStream
) {
  SkResult result;
  SkStream stream;

  // Allocate the device
  stream = SKALLOC(streamSize, SK_SYSTEM_ALLOCATION_SCOPE_STREAM);
  if (!stream) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Fill in the OpenSK external information
  memset(stream, 0, streamSize);
  stream->objectType = SK_OBJECT_TYPE_STREAM;
  stream->component = component;

  // Add to the component
  stream->pNext = component->pStreams;
  component->pStreams = stream;

  *pStream = stream;
  return SK_SUCCESS;
}

SKAPI_ATTR SkBool32 SKAPI_CALL skEnumerateFormats(
  SkFormat                      seed,
  SkFormat*                     pIterator,
  SkFormat*                     pValue
) {
  SkFormat state;

  // Handle dynamic formats (must match the machine's endianess)
  if (seed >= SK_FORMAT_DYNAMIC_BEGIN && seed <= SK_FORMAT_DYNAMIC_END) {
    if (*pIterator == SK_FORMAT_INVALID) {
      *pValue = *pIterator = skGetFormatStatic(seed);
      return SK_TRUE;
    }
    *pValue = SK_FORMAT_INVALID;
    return SK_FALSE;
  }

  // Update seed to sub-state
  switch (seed) {
    case SK_FORMAT_FIRST:
      if (*pIterator >= SK_FORMAT_DYNAMIC_BEGIN || *pIterator == SK_FORMAT_INVALID)
        state = SK_FORMAT_FIRST_DYNAMIC;
      else
        state = SK_FORMAT_FIRST_STATIC;
      break;
    case SK_FORMAT_LAST:
      if (*pIterator >= SK_FORMAT_DYNAMIC_BEGIN || *pIterator == SK_FORMAT_INVALID)
        state = SK_FORMAT_LAST_DYNAMIC;
      else
        state = SK_FORMAT_LAST_STATIC;
      break;
    default:
      state = seed;
  }

  // Handle SK_FORMAT_FIRST_DYNAMIC
  if (state == SK_FORMAT_FIRST_DYNAMIC) {
    // Initialize
    if (*pIterator == SK_FORMAT_INVALID) {
      *pIterator = SK_FORMAT_DYNAMIC_BEGIN;
      *pValue = skGetFormatStatic(*pIterator);
      return SK_TRUE;
    }
    // Terminate
    else if (*pIterator >= SK_FORMAT_DYNAMIC_END) {
      if (seed == SK_FORMAT_FIRST) {
        *pIterator = SK_FORMAT_INVALID;
        state = SK_FORMAT_FIRST_STATIC;
      }
    }
    // Enumerate
    else {
      *pValue = skGetFormatStatic(++(*pIterator));
      return SK_TRUE;
    }
  }

  // Handle SK_FORMAT_LAST_DYNAMIC
  if (state == SK_FORMAT_LAST_DYNAMIC) {
    // Initialize
    if (*pIterator == SK_FORMAT_INVALID) {
      *pIterator = SK_FORMAT_DYNAMIC_END;
      *pValue = skGetFormatStatic(*pIterator);
      return SK_TRUE;
    }
    // Terminate
    else if (*pIterator <= SK_FORMAT_DYNAMIC_BEGIN) {
      if (seed == SK_FORMAT_LAST) {
        *pIterator = SK_FORMAT_INVALID;
        state = SK_FORMAT_LAST_STATIC;
      }
    }
    // Enumerate
    else {
      *pValue = skGetFormatStatic(--(*pIterator));
      return SK_TRUE;
    }
  }

  // Handle SK_FORMAT_FIRST_STATIC
  if (state == SK_FORMAT_FIRST_STATIC) {
    // Initialize
    if (*pIterator == SK_FORMAT_INVALID) {
      *pValue = *pIterator = SK_FORMAT_STATIC_BEGIN;
      return SK_TRUE;
    }
    // Terminate
    else if (*pIterator >= SK_FORMAT_STATIC_END) {
      *pValue = SK_FORMAT_INVALID;
      return SK_FALSE;
    }
    // Enumerate
    else {
      *pValue = ++(*pIterator);
      return SK_TRUE;
    }
  }

  // Handle SK_FORMAT_FIRST_STATIC
  if (state == SK_FORMAT_LAST_STATIC) {
    // Initialize
    if (*pIterator == SK_FORMAT_INVALID) {
      *pValue = *pIterator = SK_FORMAT_STATIC_END;
      return SK_TRUE;
    }
      // Terminate
    else if (*pIterator <= SK_FORMAT_STATIC_BEGIN) {
      *pValue = SK_FORMAT_INVALID;
      return SK_FALSE;
    }
      // Enumerate
    else {
      *pValue = --(*pIterator);
      return SK_TRUE;
    }
  }

  // Handle when the type is explicitly defined
  if (state != SK_FORMAT_INVALID) {
    if (*pIterator == SK_FORMAT_INVALID) {
      *pValue = *pIterator = seed;
      return SK_TRUE;
    }
  }

  *pValue = SK_FORMAT_INVALID;
  return SK_FALSE;
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
    opResult = (*hostApiInitializer)(instance, &hostApi, pAllocator);
    if (opResult != SK_SUCCESS) {
      result = SK_INCOMPLETE;
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

static void
searchDeviceList(char const* path, SkDevice device, SkObject *result) {
  SkComponent  component;
  while (device) {
    if (pathcmp(device->properties.identifier, path)) {
      (*result) = (SkObject)device;
      break;
    }
    component = device->pComponents;
    while (component) {
      if (pathcmp(component->properties.identifier, path)) {
        break;
      }
      component = component->pNext;
    }
    if (component) {
      (*result) = (SkObject)component;
      break;
    }
    device = device->pNext;
  }
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
  SkHostApiProperties properties;

  // Check for valid sk path prefix (no-op)
  if (strncmp(path, "sk://", 5) == 0) {
    path = ADVANCE(5);
  }

  // Check if the path should resolve to the instance
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

  // Refresh the device list (always happens on object requests)
  result = skScanDevices(hostApi);
  if (result != SK_SUCCESS && result != SK_INCOMPLETE) {
    return SK_NULL_HANDLE;
  }

  // Check if the path should resolve to the host
  path = ADVANCE(strlen(properties.identifier));
  if (CHECK_END()) {
    return (SkObject)hostApi;
  }

  // Check for physical devices and components
  deviceOrComponent = NULL;
  searchDeviceList(path, hostApi->pDevices, &deviceOrComponent);
  if (deviceOrComponent != SK_NULL_HANDLE) {
    return deviceOrComponent;
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

SKAPI_ATTR SkResult SKAPI_CALL skScanDevices(
  SkHostApi                         hostApi
) {
  uint32_t physicalDevices;
  uint32_t physicalComponents;
  uint32_t virtualDevices;
  uint32_t virtualComponents;
  SkDevice device;
  SkComponent component;
  SkResult result;

  // Initialize Counts
  physicalDevices = 0;
  physicalComponents = 0;
  virtualDevices = 0;
  virtualComponents = 0;

  // Count Devices and Components
  result = hostApi->impl.SkScanDevices(hostApi, hostApi->instance->pAllocator);
  if (result == SK_SUCCESS) {
    device = hostApi->pDevices;
    while (device) {
      if (device->properties.isPhysical) {
        ++physicalDevices;
      }
      else {
        ++virtualDevices;
      }
      component = device->pComponents;
      while (component) {
        if (device->properties.isPhysical) {
          ++physicalComponents;
        }
        else {
          ++virtualComponents;
        }
        component = component->pNext;
      }
      device = device->pNext;
    }
  }

  // Assign counts
  hostApi->properties.physicalDevices = physicalDevices;
  hostApi->properties.physicalComponents = physicalComponents;
  hostApi->properties.virtualDevices = virtualDevices;
  hostApi->properties.virtualComponents = virtualComponents;

  return result;
}

static SkResult
enumerateDeviceList(
  SkDevice                          device,
  SkBool32                          isPhysical,
  uint32_t*                         pDeviceCount,
  SkDevice*                         pDevices
) {
  uint32_t count = 0;

  // Copy physical devices into output variables
  while (device) {
    if (device->properties.isPhysical == isPhysical || isPhysical == SK_UNKNOWN) {
      if (pDevices) {
        if (count >= *pDeviceCount) break;
        *pDevices = device;
        ++pDevices;
      }
      ++count;
    }
    device = device->pNext;
  }
  *pDeviceCount = count;

  // Right now this cannot fail...
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pPhysicalDeviceCount,
  SkDevice*                         pPhysicalDevices
) {
  return enumerateDeviceList(hostApi->pDevices, SK_TRUE, pPhysicalDeviceCount, pPhysicalDevices);
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pVirtualDeviceCount,
  SkDevice*                         pVirtualDevices
) {
  return enumerateDeviceList(hostApi->pDevices, SK_FALSE, pVirtualDeviceCount, pVirtualDevices);
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pDeviceCount,
  SkDevice*                         pDevices
) {
  return enumerateDeviceList(hostApi->pDevices, SK_UNKNOWN, pDeviceCount, pDevices);
}

SKAPI_ATTR void SKAPI_CALL skGetDeviceProperties(
  SkDevice                          device,
  SkDeviceProperties*               pProperties
) {
  *pProperties = device->properties;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateComponents(
  SkDevice                          device,
  uint32_t*                         pComponentCount,
  SkComponent *                     pComponents
) {
  uint32_t count = 0;
  SkComponent component;

  // Copy physical devices into output variables
  component = device->pComponents;
  while (component) {
    if (pComponents) {
      if (count >= *pComponentCount) break;
      *pComponents = component;
      ++pComponents;
    }
    ++count;
    component = component->pNext;
  }
  *pComponentCount = count;

  // Right now this cannot fail...
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skGetComponentProperties(
  SkComponent                       component,
  SkComponentProperties*            pProperties
) {
  *pProperties = component->properties;
}


SKAPI_ATTR SkResult SKAPI_CALL skGetComponentLimits(
  SkComponent                       component,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
) {
  return component->device->hostApi->impl.SkGetComponentLimits(
    component, streamType, pLimits
  );
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestStream(
  SkComponent                       component,
  SkStreamRequest*                  pStreamRequest,
  SkStream*                         pStream
) {
  return component->device->hostApi->impl.SkRequestStream(
      component, pStreamRequest, pStream, component->device->hostApi->instance->pAllocator
  );
}

SKAPI_ATTR void SKAPI_CALL skGetStreamInfo(
  SkStream                          stream,
  SkStreamInfo*                     pStreamInfo
) {
  *pStreamInfo = stream->streamInfo;
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteInterleaved(
  SkStream                          stream,
  void const*                       pBuffer,
  uint32_t                          samples
) {
  SKASSERT(stream->streamInfo.streamType == SK_STREAM_TYPE_PCM_PLAYBACK, "Attempted to write to a non-write or non-PCM stream.");
  return stream->component->device->hostApi->impl.SkStreamWriteInterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteNoninterleaved(
  SkStream                          stream,
  void const* const*                pBuffer,
  uint32_t                          samples
) {
  SKASSERT(stream->streamInfo.streamType == SK_STREAM_TYPE_PCM_PLAYBACK, "Attempted to write to a non-write or non-PCM stream.");
  return stream->component->device->hostApi->impl.SkStreamWriteNoninterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadInterleaved(
  SkStream                          stream,
  void*                             pBuffer,
  uint32_t                          samples
) {
  SKASSERT(stream->streamInfo.streamType == SK_STREAM_TYPE_PCM_CAPTURE, "Attempted to read from a non-capture or non-PCM stream.");
  return stream->component->device->hostApi->impl.SkStreamReadInterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadNoninterleaved(
  SkStream                          stream,
  void**                            pBuffer,
  uint32_t                          samples
) {
  SKASSERT(stream->streamInfo.streamType == SK_STREAM_TYPE_PCM_CAPTURE, "Attempted to read from a non-capture or non-PCM stream.");
  return stream->component->device->hostApi->impl.SkStreamReadNoninterleaved(
    stream,
    pBuffer,
    samples
  );
}

SKAPI_ATTR void SKAPI_CALL skDestroyStream(
  SkStream                          stream,
  SkBool32                          drain
) {
  stream->component->device->hostApi->impl.SkDestroyStream(
    stream,
    drain
  );
}

////////////////////////////////////////////////////////////////////////////////
// Stringize Functions
////////////////////////////////////////////////////////////////////////////////
#define PRINT_CASE(e) case e: return #e
SKAPI_ATTR char const* SKAPI_CALL skGetObjectTypeString(
  SkObjectType                      objectType
) {
  switch (objectType) {
    PRINT_CASE(SK_OBJECT_TYPE_INVALID);
    PRINT_CASE(SK_OBJECT_TYPE_INSTANCE);
    PRINT_CASE(SK_OBJECT_TYPE_HOST_API);
    PRINT_CASE(SK_OBJECT_TYPE_DEVICE);
    PRINT_CASE(SK_OBJECT_TYPE_COMPONENT);
    PRINT_CASE(SK_OBJECT_TYPE_STREAM);
  }
  return NULL;
}

SKAPI_ATTR char const* SKAPI_CALL skGetStreamTypeString(
  SkStreamType                      streamType
) {
  switch (streamType) {
    PRINT_CASE(SK_STREAM_TYPE_NONE);
    PRINT_CASE(SK_STREAM_TYPE_PCM_PLAYBACK);
    PRINT_CASE(SK_STREAM_TYPE_PCM_CAPTURE);
  }
  SKERROR("Invalid or unsupported value (%d)", (int)streamType);
}

SKAPI_ATTR char const* SKAPI_CALL skGetAccessModeString(
  SkAccessMode                      accessMode
) {
  switch (accessMode) {
    PRINT_CASE(SK_ACCESS_MODE_BLOCK);
    PRINT_CASE(SK_ACCESS_MODE_NONBLOCK);
  }
  SKERROR("Invalid or unsupported value (%d)", (int)accessMode);
}

SKAPI_ATTR char const* SKAPI_CALL skGetAccessTypeString(
  SkAccessType                      accessType
) {
  switch (accessType) {
    PRINT_CASE(SK_ACCESS_TYPE_ANY);
    PRINT_CASE(SK_ACCESS_TYPE_INTERLEAVED);
    PRINT_CASE(SK_ACCESS_TYPE_NONINTERLEAVED);
    PRINT_CASE(SK_ACCESS_TYPE_MMAP_INTERLEAVED);
    PRINT_CASE(SK_ACCESS_TYPE_MMAP_NONINTERLEAVED);
    PRINT_CASE(SK_ACCESS_TYPE_MMAP_COMPLEX);
  }
  SKERROR("Invalid or unsupported value (%d)", (int)accessType);
}

SKAPI_ATTR char const* SKAPI_CALL skGetFormatString(
  SkFormat                          format
) {
  switch (format) {
    PRINT_CASE(SK_FORMAT_INVALID);
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
    PRINT_CASE(SK_FORMAT_FIRST);
    PRINT_CASE(SK_FORMAT_FIRST_DYNAMIC);
    PRINT_CASE(SK_FORMAT_FIRST_STATIC);
    PRINT_CASE(SK_FORMAT_LAST);
    PRINT_CASE(SK_FORMAT_LAST_DYNAMIC);
    PRINT_CASE(SK_FORMAT_LAST_STATIC);
  }
  SKERROR("Invalid or unsupported value (%d)", (int)format);
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