/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard include header. (Implementation must adhere to this API)
 ******************************************************************************/
#ifndef OPENSK_H
#define OPENSK_H 1

// OpenSK
#include <OpenSK/opensk_platform.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Standard Defines
////////////////////////////////////////////////////////////////////////////////

#define SK_MAKE_VERSION(major, minor, patch) (((major) << 22) | ((minor) << 12) | (patch))
#define SK_DEFINE_HANDLE(object) typedef struct object##_T* object
#define SK_API_VERSION_0_0 SK_MAKE_VERSION(0, 0, 0)

#define SK_VERSION_MAJOR(version) ((uint32_t)(version) >> 22)
#define SK_VERSION_MINOR(version) ((uint32_t)(version) >> 12 & 0x3ff)
#define SK_VERSION_PATCH(version) ((uint32_t)(version) & 0xfff)
#define SK_NULL_HANDLE 0

SK_DEFINE_HANDLE(SkObject);
SK_DEFINE_HANDLE(SkInstance);
SK_DEFINE_HANDLE(SkHostApi);
SK_DEFINE_HANDLE(SkDevice);
SK_DEFINE_HANDLE(SkComponent);
SK_DEFINE_HANDLE(SkStream);

typedef int32_t SkBool32;
typedef uint32_t SkFlags;

#define SK_TRUE 1
#define SK_FALSE 0
#define SK_UNKNOWN -1
#define SK_UUID_SIZE 256
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
#define SK_MAX_VENDOR_NAME_SIZE 256
#define SK_MAX_MODEL_NAME_SIZE 256
#define SK_MAX_SERIAL_NAME_SIZE 256

////////////////////////////////////////////////////////////////////////////////
// Standard Enumerations
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
  SK_ERROR_UNKNOWN,
  // Generic System Errors
  SK_ERROR_OUT_OF_HOST_MEMORY,
  SK_ERROR_INITIALIZATION_FAILED,
  SK_ERROR_NOT_IMPLEMENTED,
  SK_ERROR_NOT_FOUND,
  SK_ERROR_INVALID,
  // Device Results
  SK_ERROR_DEVICE_QUERY_FAILED,
  // Stream Results
  SK_ERROR_STREAM_NOT_READY,
  SK_ERROR_STREAM_BUSY,
  SK_ERROR_STREAM_XRUN,
  SK_ERROR_STREAM_INVALID,
  SK_ERROR_STREAM_LOST,
  SK_ERROR_STREAM_REQUEST_FAILED
} SkResult;

typedef enum SkAllocationScope {
  SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 0,
  SK_SYSTEM_ALLOCATION_SCOPE_HOST_API,
  SK_SYSTEM_ALLOCATION_SCOPE_DEVICE,
  SK_SYSTEM_ALLOCATION_SCOPE_STREAM,
  SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION,
  SK_SYSTEM_ALLOCATION_SCOPE_UTILITY
} SkAllocationScope;

typedef enum SkHostCapabilities {
  SK_HOST_CAPABILITIES_ALWAYS_PRESENT = 1<<0,
  SK_HOST_CAPABILITIES_PCM = 1<<1,
  SK_HOST_CAPABILITIES_SEQUENCER = 1<<2,
  SK_HOST_CAPABILITIES_VIDEO = 1<<3
} SkHostCapabilities;

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
  SK_ACCESS_TYPE_MMAP_INTERLEAVED, // TODO: Remove MMAP, turn it into a property.
  SK_ACCESS_TYPE_MMAP_NONINTERLEAVED,
  SK_ACCESS_TYPE_MMAP_COMPLEX
} SkAccessType;

typedef enum SkAccessMode {
  SK_ACCESS_MODE_BLOCK,
  SK_ACCESS_MODE_NONBLOCK
} SkAccessMode;

typedef enum SkStreamFlags {
  SK_STREAM_FLAGS_NONE = 0,
  SK_STREAM_FLAGS_POLL_AVAILABLE = 1<<0,
  SK_STREAM_FLAGS_WAIT_AVAILABLE = 1<<1
} SkStreamFlags;
#define SK_STREAM_FLAGS_BEGIN SK_STREAM_FLAGS_POLL_AVAILABLE
#define SK_STREAM_FLAGS_END (SK_STREAM_FLAGS_WAIT_AVAILABLE<<1)

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

typedef enum SkTimeUnits {
  SK_TIME_UNITS_UNKNOWN,
  SK_TIME_UNITS_PLANCKTIME,
  SK_TIME_UNITS_PLANCKTIME_E3,
  SK_TIME_UNITS_PLANCKTIME_E6,
  SK_TIME_UNITS_PLANCKTIME_E9,
  SK_TIME_UNITS_PLANCKTIME_E12,
  SK_TIME_UNITS_PLANCKTIME_E15,
  SK_TIME_UNITS_PLANCKTIME_E18,
  SK_TIME_UNITS_YOCTOSECONDS,
  SK_TIME_UNITS_ZEPTOSECONDS,
  SK_TIME_UNITS_ATTOSECONDS,
  SK_TIME_UNITS_FEMTOSECONDS,
  SK_TIME_UNITS_PICOSECONDS,
  SK_TIME_UNITS_NANOSECONDS,
  SK_TIME_UNITS_MICROSECONDS,
  SK_TIME_UNITS_MILLISECONDS,
  SK_TIME_UNITS_SECONDS,
  SK_TIME_UNITS_KILOSECONDS,
  SK_TIME_UNITS_MEGASECONDS,
  SK_TIME_UNITS_GIGASECONDS,
  SK_TIME_UNITS_TERASECONDS,
  SK_TIME_UNITS_PETASECONDS,
  SK_TIME_UNITS_EXASECONDS,
  SK_TIME_UNITS_ZETTASECONDS,
  SK_TIME_UNITS_YOTTASECONDS
} SkTimeUnits;

typedef enum SkTransportType {
  SK_TRANSPORT_TYPE_UNKNOWN,
  SK_TRANSPORT_TYPE_UNDEFINED,
  SK_TRANSPORT_TYPE_PCI,
  SK_TRANSPORT_TYPE_USB,
  SK_TRANSPORT_TYPE_FIREWIRE,
  SK_TRANSPORT_TYPE_BLUETOOTH,
  SK_TRANSPORT_TYPE_THUNDERBOLT
} SkTransportType;

typedef enum SkFormFactor {
  SK_FORM_FACTOR_UNKNOWN,
  SK_FORM_FACTOR_UNDEFINED,
  SK_FORM_FACTOR_INTERNAL,
  SK_FORM_FACTOR_MICROPHONE,
  SK_FORM_FACTOR_SPEAKER,
  SK_FORM_FACTOR_HEADPHONE,
  SK_FORM_FACTOR_WEBCAM,
  SK_FORM_FACTOR_HEADSET,
  SK_FORM_FACTOR_HANDSET,
  SK_FORM_FACTOR_TELEVISION
} SkFormFactor;

////////////////////////////////////////////////////////////////////////////////
// Standard Function Pointers
////////////////////////////////////////////////////////////////////////////////

typedef void* (SKAPI_PTR *PFN_skAllocationFunction)(
  void*                                 pUserData,
  size_t                                size,
  size_t                                alignment,
  SkAllocationScope allocationScope
);

typedef void* (SKAPI_PTR *PFN_skFreeFunction)(
  void*                                 pUserData,
  void*                                 memory
);

////////////////////////////////////////////////////////////////////////////////
// Standard Structures
////////////////////////////////////////////////////////////////////////////////

#if defined(UINT64_MAX)
typedef uint64_t SkTimeQuantum;
#define SK_TIME_UNITS_MIN SK_TIME_UNITS_ATTOSECONDS
#define SK_TIME_UNITS_MAX SK_TIME_UNITS_PETASECONDS
#define SK_TIME_QUANTUM_MOD 1000000000000000000u
#define SK_TIME_QUANTUM_MAX (SK_TIME_QUANTUM_MOD-1)
#define SK_TIME_QUANTUM_BITS 64
#elif defined(UINT32_MAX)
typedef uint32_t SkTimeQuantum;
#define SK_TIME_UNITS_MIN SK_TIME_UNITS_NANOSECONDS
#define SK_TIME_UNITS_MAX SK_TIME_UNITS_MEGASECONDS
#define SK_TIME_QUANTUM_MOD 1000000000u
#define SK_TIME_QUANTUM_MAX (SK_TIME_QUANTUM_MOD-1)
#define SK_TIME_QUANTUM_BITS 32
#else
# error "Unsupported machine configuration - must at least support 32-bit numbers!"
#endif
#define SK_TIME_PERIOD_HI 1
#define SK_TIME_PERIOD_LO 0

typedef SkTimeQuantum SkTimePeriod[2];

typedef struct SkApplicationInfo {
  char const*                           pApplicationName;
  uint32_t                              applicationVersion;
  char const*                           pEngineName;
  uint32_t                              engineVersion;
  uint32_t                              apiVersion;
} SkApplicationInfo;

typedef struct SkInstanceCreateInfo {
  SkInstanceCreateFlags                 flags;
  SkApplicationInfo const*              pApplicationInfo;
  uint32_t                              enabledExtensionCount;
  char const* const*                    ppEnabledExtensionNames;
} SkInstanceCreateInfo;

typedef struct SkAllocationCallbacks {
  void*                                 pUserData;
  PFN_skAllocationFunction              pfnAllocation;
  PFN_skFreeFunction                    pfnFree;
} SkAllocationCallbacks;

typedef struct SkHostApiProperties {
  char                                  identifier[SK_MAX_HOST_IDENTIFIER_SIZE];
  char                                  hostName[SK_MAX_HOST_NAME_SIZE];
  char                                  hostNameFull[SK_MAX_HOST_NAME_SIZE];
  char                                  description[SK_MAX_HOST_DESC_SIZE];
  SkHostCapabilities                    capabilities;
  uint32_t                              physicalDevices;
  uint32_t                              physicalComponents;
  uint32_t                              virtualDevices;
  uint32_t                              virtualComponents;
} SkHostApiProperties;

typedef struct SkExtensionProperties {
  char                                  extensionName[SK_MAX_EXTENSION_NAME_SIZE];
  uint32_t                              specVersion;
} SkExtensionProperties;

typedef struct SkDeviceProperties {
  // Device Properties
  char                                  identifier[SK_MAX_DEVICE_IDENTIFIER_SIZE];  // SK Canonical Device Identifier (ex. pda)
  char                                  deviceName[SK_MAX_DEVICE_NAME_SIZE];        // Pretty Device Name (ex. HDA Nvidia)
  char                                  driverName[SK_MAX_DRIVER_NAME_SIZE];        // Pretty Driver Name (ex. USB-Audio)
  char                                  mixerName[SK_MAX_MIXER_NAME_SIZE];          // Pretty Mixer Name (ex. USB Mixer)
  uint32_t                              componentCount;                             // Number of components this device owns
  SkBool32                              isPhysical;                                 // Whether or not the device (and it's components) are physical
  // Hardware Properties
  char                                  deviceUUID[SK_UUID_SIZE];                   // The UUID for this device under this transport
  char                                  vendorUUID[SK_UUID_SIZE];                   // The UUID for the vendor
  char                                  vendorName[SK_MAX_VENDOR_NAME_SIZE];        // The name of the vendor
  char                                  modelUUID[SK_UUID_SIZE];                    // The UUID for the model
  char                                  modelName[SK_MAX_MODEL_NAME_SIZE];          // The name of the model
  char                                  serialUUID[SK_UUID_SIZE];                   // The UUID for the serial
  char                                  serialName[SK_MAX_SERIAL_NAME_SIZE];        // The name of the serial
  SkTransportType                       transportType;                              // The transport type
  SkFormFactor                          formFactor;                                 // The form factor
} SkDeviceProperties;

typedef struct SkComponentProperties {
  char                                  identifier[SK_MAX_COMPONENT_IDENTIFIER_SIZE];
  char                                  componentName[SK_MAX_COMPONENT_NAME_SIZE];
  char                                  componentDescription[SK_MAX_COMPONENT_DESC_SIZE];
  SkStreamTypes                         supportedStreams;
  SkBool32                              isPhysical;
} SkComponentProperties;

typedef struct SkComponentLimits {
  uint32_t                              maxChannels;
  uint32_t                              minChannels;
  uint64_t                              maxFrameSize;
  uint64_t                              minFrameSize;
  SkTimePeriod                          maxBufferTime;
  SkTimePeriod                          minBufferTime;
  uint64_t                              maxPeriodSize;
  uint64_t                              minPeriodSize;
  SkTimePeriod                          maxPeriodTime;
  SkTimePeriod                          minPeriodTime;
  uint64_t                              maxPeriods;
  uint64_t                              minPeriods;
  uint64_t                              maxRate;
  uint64_t                              minRate;
  SkBool32                              supportedFormats[SK_FORMAT_MAX];
} SkComponentLimits;

typedef struct SkPcmStreamInfo {
  SkStreamType                          streamType;   // Should be one of the PCM stream types
  SkAccessMode                          accessMode;   // How the stream should be accessed (block/non-block)
  SkAccessType                          accessType;   // Configuration for reading/writing data
  SkStreamFlags                         streamFlags;  // Configuration for misc. runtime options.
  SkFormat                              formatType;   // The underlying type of the PCM stream
  uint32_t                              formatBits;   // Number of bits in the format
  uint32_t                              channels;     // Number of channels in a sample
  uint32_t                              sampleRate;   // Number of samples per second
  uint32_t                              sampleBits;   // Number of bits in a sample
  SkTimePeriod                          sampleTime;   // Calculated time each sample represents
  uint32_t                              periods;      // Number of periods
  uint32_t                              periodSamples;// Number of samples in a period
  uint32_t                              periodBits;   // Number of bits in a period
  SkTimePeriod                          periodTime;   // Calculated time each period represents
  uint32_t                              bufferSamples;// Number of samples in the back-buffer
  uint32_t                              bufferBits;   // Number of bits in the back-buffer
  SkTimePeriod                          bufferTime;   // Calculated time the back-buffer represents
} SkPcmStreamInfo;

typedef union SkStreamInfo {
  SkStreamType                          streamType;
  SkPcmStreamInfo                       pcm;
} SkStreamInfo;

////////////////////////////////////////////////////////////////////////////////
// Standard Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  SkAllocationCallbacks const*          pAllocator,
  SkInstanceCreateInfo const*           pCreateInfo,
  SkInstance*                           pInstance
);

SKAPI_ATTR void SKAPI_CALL skDestroyInstance(
  SkInstance                            instance
);

SKAPI_ATTR SkObject SKAPI_CALL skRequestObject(
  SkInstance                            instance,
  char const*                           path
);

SKAPI_ATTR SkObjectType SKAPI_CALL skGetObjectType(
  SkObject                              object
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateHostApi(
  SkInstance                            instance,
  uint32_t*                             pHostApiCount,
  SkHostApi*                            pHostApi
);

SKAPI_ATTR SkResult SKAPI_CALL skScanDevices(
  SkHostApi                             hostApi
);

SKAPI_ATTR void SKAPI_CALL skGetHostApiProperties(
  SkHostApi                             hostApi,
  SkHostApiProperties*                  pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumeratePhysicalDevices(
  SkHostApi                             hostApi,
  uint32_t*                             pPhysicalDeviceCount,
  SkDevice*                             pPhysicalDevices
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateVirtualDevices(
  SkHostApi                             hostApi,
  uint32_t*                             pVirtualDeviceCount,
  SkDevice*                             pVirtualDevices
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDevices(
  SkHostApi                             hostApi,
  uint32_t*                             pDeviceCount,
  SkDevice*                             pDevices
);

SKAPI_ATTR void SKAPI_CALL skGetDeviceProperties(
  SkDevice                              device,
  SkDeviceProperties*                   pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateComponents(
  SkDevice                              device,
  uint32_t*                             pComponentCount,
  SkComponent*                          pComponents
);

SKAPI_ATTR void SKAPI_CALL skGetComponentProperties(
  SkComponent                           component,
  SkComponentProperties*                pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skGetComponentLimits(
  SkComponent                           component,
  SkStreamType                          streamType,
  SkComponentLimits*                    pLimits
);

SKAPI_ATTR SkResult SKAPI_CALL skRequestStream(
  SkComponent                           component,
  SkStreamInfo*                         pStreamRequest,
  SkStream*                             pStream
);

SKAPI_ATTR void SKAPI_CALL skGetStreamInfo(
  SkStream                              stream,
  SkStreamInfo*                         pStreamInfo
);

SKAPI_ATTR void* SKAPI_CALL skGetStreamHandle(
  SkStream                              stream
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteInterleaved(
  SkStream                              stream,
  void const*                           pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamWriteNoninterleaved(
  SkStream                              stream,
  void const* const*                    pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadInterleaved(
  SkStream                              stream,
  void*                                 pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skStreamReadNoninterleaved(
  SkStream                              stream,
  void**                                pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR void SKAPI_CALL skDestroyStream(
  SkStream                              stream,
  SkBool32                              drain
);

////////////////////////////////////////////////////////////////////////////////
// Timer Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkTimeQuantum SKAPI_CALL skTimeQuantumConvert(
  SkTimeQuantum                         timeQuantum,
  SkTimeUnits                           fromTimeUnits,
  SkTimeUnits                           toTimeUnits
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodClear(
  SkTimePeriod                          timePeriod
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodSet(
  SkTimePeriod                          result,
  SkTimePeriod                          original
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodSetQuantum(
  SkTimePeriod                          timePeriod,
  SkTimeQuantum                         timeQuantum,
  SkTimeUnits                           timeUnits
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodAdd(
  SkTimePeriod                          resultTimePeriod,
  SkTimePeriod                          leftTimePeriod,
  SkTimePeriod                          rightTimePeriod
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodScaleAdd(
  SkTimePeriod                          resultTimePeriod,
  SkTimeQuantum                         leftScalar,
  SkTimePeriod                          leftTimePeriod,
  SkTimePeriod                          rightTimePeriod
);

SKAPI_ATTR void SKAPI_CALL skTimePeriodSubtract(
  SkTimePeriod                          resultTimePeriod,
  SkTimePeriod                          leftTimePeriod,
  SkTimePeriod                          rightTimePeriod
);

SKAPI_ATTR SkBool32 SKAPI_CALL skTimePeriodLess(
  SkTimePeriod                          leftTimePeriod,
  SkTimePeriod                          rightTimePeriod
);

SKAPI_ATTR SkBool32 SKAPI_CALL skTimePeriodLessEqual(
  SkTimePeriod                          leftTimePeriod,
  SkTimePeriod                          rightTimePeriod
);

SKAPI_ATTR SkBool32 SKAPI_CALL skTimePeriodIsZero(
  SkTimePeriod                          timePeriod
);

SKAPI_ATTR float SKAPI_CALL skTimePeriodToFloat(
  SkTimePeriod                          timePeriod,
  SkTimeUnits                           timeUnits
);

SKAPI_ATTR SkTimeQuantum SKAPI_CALL skTimePeriodToQuantum(
  SkTimePeriod                          timePeriod,
  SkTimeUnits                           timeUnits
);

////////////////////////////////////////////////////////////////////////////////
// Stringize Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR char const* SKAPI_CALL skGetResultString(
  SkResult                              result
);

SKAPI_ATTR char const* SKAPI_CALL skGetObjectTypeString(
  SkObjectType                          objectType
);

SKAPI_ATTR char const* SKAPI_CALL skGetStreamTypeString(
  SkStreamType                          streamType
);

SKAPI_ATTR char const* SKAPI_CALL skGetStreamFlagString(
  SkStreamFlags                         streamFlag
);

SKAPI_ATTR char const* SKAPI_CALL skGetFormatString(
  SkFormat                              format
);

SKAPI_ATTR char const* SKAPI_CALL skGetAccessModeString(
  SkAccessMode                          accessMode
);

SKAPI_ATTR char const* SKAPI_CALL skGetAccessTypeString(
  SkAccessType                          accessType
);

SKAPI_ATTR SkFormat SKAPI_CALL skGetFormatStatic(
  SkFormat                              format
);

SKAPI_ATTR char const* SKAPI_CALL skGetTimeUnitsString(
  SkTimeUnits                           timeUnits
);

SKAPI_ATTR char const* SKAPI_CALL skGetTimeUnitsSymbolString(
  SkTimeUnits                           timeUnits
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_H
