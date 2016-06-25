/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard include header. (Implementation must adhere to this API)
 ******************************************************************************/
#ifndef OPENSK_H
#define OPENSK_H 1

#include "opensk_platform.h"

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

SK_DEFINE_HANDLE(SkObject)
SK_DEFINE_HANDLE(SkInstance)
SK_DEFINE_HANDLE(SkHostApi)
SK_DEFINE_HANDLE(SkDevice)
SK_DEFINE_HANDLE(SkComponent)
SK_DEFINE_HANDLE(SkStream)

typedef int32_t SkBool32;
typedef uint32_t SkFlags;

#define SK_TRUE 1
#define SK_FALSE 0
#define SK_UNKNOWN -1
#define SK_MAX_EXTENSION_NAME_SIZE 256
#define SK_MAX_DEVICE_NAME_SIZE 256
#define SK_MAX_COMPONENT_NAME_SIZE 256
#define SK_MAX_COMPONENT_DESC_SIZE 256
#define SK_MAX_DRIVER_NAME_SIZE 256
#define SK_MAX_MIXER_NAME_SIZE 256
#define SK_MAX_HOST_IDENTIFIER_SIZE 25
#define SK_MAX_DEVICE_IDENTIFIER_SIZE 25
#define SK_MAX_COMPONENT_IDENTIFIER_SIZE 25
#define SK_MAX_HOST_NAME_SIZE 256
#define SK_MAX_HOST_DESC_SIZE 256

////////////////////////////////////////////////////////////////////////////////
// Standard Types
////////////////////////////////////////////////////////////////////////////////
typedef enum SkObjectType {
  SK_OBJECT_TYPE_INVALID = 0,
  SK_OBJECT_TYPE_INSTANCE,
  SK_OBJECT_TYPE_HOST_API,
  SK_OBJECT_TYPE_DEVICE,
  SK_OBJECT_TYPE_COMPONENT,
  SK_OBJECT_TYPE_STREAM
} SkObjectType;

typedef enum SkInstanceCreateFlags {
  SK_INSTANCE_DEFAULT = 0
} SkInstanceCreateFlags;

typedef enum SkResult {
  SK_SUCCESS = 0,
  SK_INCOMPLETE,
  SK_UNSUPPORTED,
  SK_ERROR_OUT_OF_HOST_MEMORY,
  SK_ERROR_INITIALIZATION_FAILED,
  SK_ERROR_EXTENSION_NOT_PRESENT,
  SK_ERROR_FAILED_QUERYING_DEVICE,
  SK_ERROR_FAILED_RESOLVING_DEVICE,
  SK_ERROR_STREAM_REQUEST_FAILED,
  SK_ERROR_STREAM_REQUEST_UNSUPPORTED,
  SK_ERROR_STREAM_BUSY,
  SK_ERROR_FAILED_STREAM_WRITE,
  SK_ERROR_XRUN,
  SK_ERROR_NOT_IMPLEMENTED,
  SK_ERROR_INVALID_OBJECT,
  SK_ERROR_UNKNOWN
} SkResult;

typedef enum SkAllocationScope {
  SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 0,
  SK_SYSTEM_ALLOCATION_SCOPE_HOST_API,
  SK_SYSTEM_ALLOCATION_SCOPE_DEVICE,
  SK_SYSTEM_ALLOCATION_SCOPE_STREAM,
  SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION
} SkAllocationScope;

typedef enum SkHostCapabilities {
  SK_HOST_CAPABILITIES_ALWAYS_PRESENT = 1<<0,
  SK_HOST_CAPABILITIES_PCM = 1<<1,
  SK_HOST_CAPABILITIES_SEQUENCER = 1<<2,
  SK_HOST_CAPABILITIES_VIDEO = 1<<3
} SkHostCapabilities;

typedef enum SkRangeDirection {
  SK_RANGE_DIRECTION_UNKNOWN,
  SK_RANGE_DIRECTION_LESS,
  SK_RANGE_DIRECTION_EQUAL,
  SK_RANGE_DIRECTION_GREATER
} SkRangeDirection;

typedef enum SkStreamType {
  SK_STREAM_TYPE_NONE = 0,
  SK_STREAM_TYPE_PCM_PLAYBACK = 1<<0,
  SK_STREAM_TYPE_PCM_CAPTURE  = 1<<1
} SkStreamType;
#define SK_STREAM_TYPE_BEGIN SK_STREAM_TYPE_PCM_PLAYBACK
#define SK_STREAM_TYPE_END (SK_STREAM_TYPE_PCM_CAPTURE<<1)
#define SK_STREAM_TYPE_PCM_BEGIN SK_STREAM_TYPE_PCM_PLAYBACK
#define SK_STREAM_TYPE_PCM_END (SK_STREAM_TYPE_PCM_CAPTURE<<1)
typedef SkStreamType SkStreamTypes;

typedef enum SkAccessType {
  SK_ACCESS_TYPE_ANY,
  SK_ACCESS_TYPE_INTERLEAVED,
  SK_ACCESS_TYPE_NONINTERLEAVED,
  SK_ACCESS_TYPE_MMAP_INTERLEAVED,
  SK_ACCESS_TYPE_MMAP_NONINTERLEAVED,
  SK_ACCESS_TYPE_MMAP_COMPLEX
} SkAccessType;

typedef enum SkAccessMode {
  SK_ACCESS_MODE_BLOCK,
  SK_ACCESS_MODE_NONBLOCK
} SkAccessMode;

typedef enum SkFormat {
  SK_FORMAT_INVALID = -1,
  SK_FORMAT_ANY = 0,
  SK_FORMAT_S8,
  SK_FORMAT_U8,
  SK_FORMAT_S16_LE,
  SK_FORMAT_S16_BE,
  SK_FORMAT_U16_LE,
  SK_FORMAT_U16_BE,
  SK_FORMAT_S24_LE,
  SK_FORMAT_S24_BE,
  SK_FORMAT_U24_LE,
  SK_FORMAT_U24_BE,
  SK_FORMAT_S32_LE,
  SK_FORMAT_S32_BE,
  SK_FORMAT_U32_LE,
  SK_FORMAT_U32_BE,
  SK_FORMAT_FLOAT_LE,
  SK_FORMAT_FLOAT_BE,
  SK_FORMAT_FLOAT64_LE,
  SK_FORMAT_FLOAT64_BE,

  // Will select machine-endian
  SK_FORMAT_S16,
  SK_FORMAT_U16,
  SK_FORMAT_S24,
  SK_FORMAT_U24,
  SK_FORMAT_S32,
  SK_FORMAT_U32,
  SK_FORMAT_FLOAT,
  SK_FORMAT_FLOAT64,

  // Will select dynamically
  SK_FORMAT_FIRST,
  SK_FORMAT_FIRST_DYNAMIC,
  SK_FORMAT_FIRST_STATIC,
  SK_FORMAT_LAST,
  SK_FORMAT_LAST_DYNAMIC,
  SK_FORMAT_LAST_STATIC
} SkFormat;
#define SK_FORMAT_BEGIN SK_FORMAT_ANY
#define SK_FORMAT_END SK_FORMAT_FLOAT64
#define SK_FORMAT_STATIC_BEGIN SK_FORMAT_S8
#define SK_FORMAT_STATIC_END SK_FORMAT_FLOAT64_BE
#define SK_FORMAT_DYNAMIC_BEGIN SK_FORMAT_S16
#define SK_FORMAT_DYNAMIC_END SK_FORMAT_FLOAT64
#define SK_FORMAT_MAX (SK_FORMAT_FLOAT64 + 1)

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
SKAPI_ATTR SkAllocationCallbacks SkDefaultAllocationCallbacks;

typedef struct SkHostApiProperties {
  char                              identifier[SK_MAX_HOST_IDENTIFIER_SIZE];
  char                              hostName[SK_MAX_HOST_NAME_SIZE];
  char                              hostNameFull[SK_MAX_HOST_NAME_SIZE];
  char                              description[SK_MAX_HOST_DESC_SIZE];
  SkHostCapabilities                capabilities;
  uint32_t                          physicalDevices;
  uint32_t                          physicalComponents;
  uint32_t                          virtualDevices;
  uint32_t                          virtualComponents;
} SkHostApiProperties;

typedef struct SkExtensionProperties {
  char                              extensionName[SK_MAX_EXTENSION_NAME_SIZE];
  uint32_t                          specVersion;
} SkExtensionProperties;

typedef struct SkDeviceProperties {
  char                              identifier[SK_MAX_DEVICE_IDENTIFIER_SIZE];
  char                              deviceName[SK_MAX_DEVICE_NAME_SIZE];
  char                              driverName[SK_MAX_DRIVER_NAME_SIZE];
  char                              mixerName[SK_MAX_MIXER_NAME_SIZE];
  uint32_t                          componentCount;
  SkBool32                          isPhysical;
} SkDeviceProperties;

typedef struct SkComponentProperties {
  char                              identifier[SK_MAX_COMPONENT_IDENTIFIER_SIZE];
  char                              componentName[SK_MAX_COMPONENT_NAME_SIZE];
  char                              componentDescription[SK_MAX_COMPONENT_DESC_SIZE];
  SkStreamTypes                     supportedStreams;
  SkBool32                          isPhysical;
} SkComponentProperties;

typedef struct SkComponentLimits {
  uint32_t                          maxChannels;
  uint32_t                          minChannels;
  uint64_t                          maxFrameSize;
  uint64_t                          minFrameSize;
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
  SkBool32                          supportedFormats[SK_FORMAT_MAX];
} SkComponentLimits;

typedef struct SkPcmStreamRequest {
  SkStreamType                      streamType;
  SkAccessType                      accessType;
  SkAccessMode                      accessMode;
  SkFormat                          formatType;
  uint32_t                          channels;
  uint32_t                          periods;
  uint32_t                          periodSize;
  uint32_t                          periodTime;
  uint32_t                          sampleRate;
  uint32_t                          bufferSize;
  uint32_t                          bufferTime;
} SkPcmStreamRequest;

typedef union SkStreamRequest {
  SkStreamType                      streamType;
  SkPcmStreamRequest                pcm;
} SkStreamRequest;

typedef struct SkPcmStreamInfo {
  SkStreamType                      streamType;
  SkAccessMode                      accessMode;
  SkAccessType                      accessType;
  SkFormat                          formatType;
  uint32_t                          formatBits;   // Number of bits in the format
  uint32_t                          periods;      // Number of periods
  uint32_t                          channels;     // Number of channels in a sample
  uint32_t                          sampleBits;   // Number of bits in a sample
  uint32_t                          sampleTime;   // Time in microseconds of a sample
  uint32_t                          periodSamples;// Number of utils in a period
  uint32_t                          periodBits;   // Number of bits in a period
  uint32_t                          periodTime;   // Time in microseconds of a period
  uint32_t                          bufferSamples;// Number of utils in the back-buffer
  uint32_t                          bufferBits;   // Number of bits in the back-buffer
  uint32_t                          bufferTime;   // Time in microseconds in the back-buffer
  uint32_t                          sampleRate;   // Number of samples per second
  uint32_t                          latency;
} SkPcmStreamInfo;

typedef union SkStreamInfo {
  SkStreamType                      streamType;
  SkPcmStreamInfo                   pcm;
} SkStreamInfo;

////////////////////////////////////////////////////////////////////////////////
// Standard Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  const SkInstanceCreateInfo*       pCreateInfo,
  const SkAllocationCallbacks*      pAllocator,
  SkInstance*                       pInstance
);

SKAPI_ATTR void SKAPI_CALL skDestroyInstance(
  SkInstance                        instance
);

SKAPI_ATTR SkObject SKAPI_CALL skRequestObject(
  SkInstance                        instance,
  char const*                       path
);

SKAPI_ATTR SkObjectType SKAPI_CALL skGetObjectType(
  SkObject                          object
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateHostApi(
  SkInstance                        instance,
  uint32_t*                         pHostApiCount,
  SkHostApi*                        pHostApi
);

SKAPI_ATTR SkResult SKAPI_CALL skScanDevices(
  SkHostApi                         hostApi
);

SKAPI_ATTR void SKAPI_CALL skGetHostApiProperties(
  SkHostApi                         hostApi,
  SkHostApiProperties*              pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pPhysicalDeviceCount,
  SkDevice*                         pPhysicalDevices
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pVirtualDeviceCount,
  SkDevice*                         pVirtualDevices
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDevices(
  SkHostApi                         hostApi,
  uint32_t*                         pDeviceCount,
  SkDevice*                         pDevices
);

SKAPI_ATTR void SKAPI_CALL skGetDeviceProperties(
  SkDevice                          device,
  SkDeviceProperties*               pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateComponents(
  SkDevice                          device,
  uint32_t*                         pComponentCount,
  SkComponent*                      pComponents
);

SKAPI_ATTR void SKAPI_CALL skGetComponentProperties(
  SkComponent                       component,
  SkComponentProperties*            pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skGetComponentLimits(
  SkComponent                       component,
  SkStreamType                      streamType,
  SkComponentLimits*                pLimits
);

SKAPI_ATTR SkResult SKAPI_CALL skRequestStream(
  SkComponent                       component,
  SkStreamRequest*                  pStreamRequest,
  SkStream*                         pStream
);

SKAPI_ATTR void SKAPI_CALL skGetStreamInfo(
  SkStream                          stream,
  SkStreamInfo*                     pStreamInfo
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteInterleaved(
  SkStream                          stream,
  void const*                       pBuffer,
  uint32_t                          samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteNoninterleaved(
  SkStream                          stream,
  void const* const*                pBuffer,
  uint32_t                          samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadInterleaved(
  SkStream                          stream,
  void*                             pBuffer,
  uint32_t                          samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadNoninterleaved(
  SkStream                          stream,
  void**                            pBuffer,
  uint32_t                          samples
);

SKAPI_ATTR void SKAPI_CALL skDestroyStream(
  SkStream                          stream,
  SkBool32                          drain
);

////////////////////////////////////////////////////////////////////////////////
// Stringize Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR char const* SKAPI_CALL skGetObjectTypeString(
  SkObjectType                      objectType
);

SKAPI_ATTR char const* SKAPI_CALL skGetStreamTypeString(
  SkStreamType                      streamType
);

SKAPI_ATTR char const* SKAPI_CALL skGetFormatString(
  SkFormat                          format
);

SKAPI_ATTR char const* SKAPI_CALL skGetAccessModeString(
  SkAccessMode                      accessMode
);

SKAPI_ATTR char const* SKAPI_CALL skGetAccessTypeString(
  SkAccessType                      accessType
);

SKAPI_ATTR SkFormat SKAPI_CALL skGetFormatStatic(
  SkFormat                          format
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_H
