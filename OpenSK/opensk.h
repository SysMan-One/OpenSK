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
 * OpenSK standard include header.
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

typedef uint32_t SkFlags;
typedef uint32_t SkBool32;
typedef void*    SkObject;

SK_DEFINE_HANDLE(SkInstance);
SK_DEFINE_HANDLE(SkDriver);
SK_DEFINE_HANDLE(SkDevice);
SK_DEFINE_HANDLE(SkEndpoint);
SK_DEFINE_HANDLE(SkPcmStream);
SK_DEFINE_HANDLE(SkMidiStream);
SK_DEFINE_HANDLE(SkVideoStream);

#define skGetStructureType(s) (*((SkStructureType*)o))
#define skGetObjectType(o) (*((SkObjectType*)o))

#define SK_TRUE 1
#define SK_FALSE 0
#define SK_UUID_SIZE 16
#define SK_MAX_NAME_SIZE 256
#define SK_MAX_IDENTIFIER_SIZE 16
#define SK_MAX_DESCRIPTION_SIZE 256

#define SK_OBJECT_PATH_PREFIX "sk:/"
#define SK_OBJECT_PATH_SEPARATOR '/'

////////////////////////////////////////////////////////////////////////////////
// Standard Enumerations
////////////////////////////////////////////////////////////////////////////////

typedef enum SkResult {
  SK_SUCCESS = 0,
  SK_TIMEOUT = 1,
  SK_INCOMPLETE = 2,
  SK_ERROR_SYSTEM_INTERNAL = -1,
  SK_ERROR_OUT_OF_HOST_MEMORY = -2,
  SK_ERROR_INITIALIZATION_FAILED = -3,
  SK_ERROR_LAYER_NOT_PRESENT = -4,
  SK_ERROR_EXTENSION_NOT_PRESENT = -5,
  SK_ERROR_FEATURE_NOT_PRESENT = -6,
  SK_ERROR_INCOMPATIBLE_DRIVER = -7,
  SK_ERROR_MEMORY_MAP_FAILED = -8,
  SK_ERROR_DEVICE_LOST = -9,
  SK_ERROR_INVALID = -10,
  SK_ERROR_NOT_SUPPORTED = -11,
  SK_ERROR_SUSPENDED = -12,
  SK_ERROR_INTERRUPTED = -13,
  SK_ERROR_BUSY = -14,
  SK_ERROR_XRUN = -15,
  SK_RESULT_BEGIN_RANGE = SK_ERROR_XRUN,
  SK_RESULT_END_RANGE = SK_INCOMPLETE,
  SK_RESULT_RANGE_SIZE = (SK_INCOMPLETE - SK_ERROR_XRUN + 1),
  SK_RESULT_MAX_ENUM = 0x7FFFFFFF
} SkResult;

typedef enum SkObjectType {
  SK_OBJECT_TYPE_INVALID = 0,
  SK_OBJECT_TYPE_LAYER = 1,
  SK_OBJECT_TYPE_INSTANCE = 2,
  SK_OBJECT_TYPE_DRIVER = 3,
  SK_OBJECT_TYPE_DEVICE = 4,
  SK_OBJECT_TYPE_ENDPOINT = 5,
  SK_OBJECT_TYPE_PCM_STREAM = 6,
  SK_OBJECT_TYPE_MIDI_STREAM = 7,
  SK_OBJECT_TYPE_VIDEO_STREAM = 8,
  SK_OBJECT_TYPE_BEGIN_RANGE = SK_OBJECT_TYPE_INVALID,
  SK_OBJECT_TYPE_END_RANGE = SK_OBJECT_TYPE_VIDEO_STREAM,
  SK_OBJECT_TYPE_RANGE_SIZE = (SK_OBJECT_TYPE_VIDEO_STREAM - SK_OBJECT_TYPE_INVALID + 1),
  SK_OBJECT_TYPE_MAX_ENUM = 0x7FFFFFFF
} SkObjectType;

typedef enum SkStructureType {
  SK_STRUCTURE_TYPE_INVALID = 0,
  SK_STRUCTURE_TYPE_INTERNAL = 1,
  SK_STRUCTURE_TYPE_APPLICATION_INFO = 2,
  SK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 3,
  SK_STRUCTURE_TYPE_DRIVER_CREATE_INFO = 4,
  SK_STRUCTURE_TYPE_LAYER_CREATE_INFO = 5,
  SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST = 6,
  SK_STRUCTURE_TYPE_ICD_PCM_STREAM_REQUEST = 7,
  SK_STRUCTURE_TYPE_PCM_STREAM_INFO = 8,
  SK_STRUCTURE_TYPE_ICD_PCM_STREAM_INFO = 9,
  SK_STRUCTURE_TYPE_BEGIN_RANGE = SK_STRUCTURE_TYPE_INVALID,
  SK_STRUCTURE_TYPE_END_RANGE = SK_STRUCTURE_TYPE_ICD_PCM_STREAM_INFO,
  SK_STRUCTURE_TYPE_RANGE_SIZE = (SK_STRUCTURE_TYPE_ICD_PCM_STREAM_INFO - SK_STRUCTURE_TYPE_INVALID + 1),
  SK_STRUCTURE_TYPE_MAX_ENUM = 0x7FFFFFFF
} SkStructureType;

typedef enum SkSystemAllocationScope {
  SK_SYSTEM_ALLOCATION_SCOPE_COMMAND = 0,
  SK_SYSTEM_ALLOCATION_SCOPE_STREAM = 1,
  SK_SYSTEM_ALLOCATION_SCOPE_DRIVER = 2,
  SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 3,
  SK_SYSTEM_ALLOCATION_SCOPE_LOADER = 4,
  SK_SYSTEM_ALLOCATION_SCOPE_BEGIN_RANGE = SK_SYSTEM_ALLOCATION_SCOPE_COMMAND,
  SK_SYSTEM_ALLOCATION_SCOPE_END_RANGE = SK_SYSTEM_ALLOCATION_SCOPE_LOADER,
  SK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE = (SK_SYSTEM_ALLOCATION_SCOPE_LOADER - SK_SYSTEM_ALLOCATION_SCOPE_COMMAND + 1),
  SK_SYSTEM_ALLOCATION_SCOPE_MAX_ENUM = 0x7FFFFFFF
} SkSystemAllocationScope;

typedef enum SkPcmFormat {
  SK_PCM_FORMAT_UNKNOWN = -1,
  SK_PCM_FORMAT_UNDEFINED = 0,
  SK_PCM_FORMAT_S8 = 1,
  SK_PCM_FORMAT_U8 = 2,
  SK_PCM_FORMAT_S16_LE = 3,
  SK_PCM_FORMAT_S16_BE = 4,
  SK_PCM_FORMAT_U16_LE = 5,
  SK_PCM_FORMAT_U16_BE = 6,
  SK_PCM_FORMAT_S24_LE = 7,
  SK_PCM_FORMAT_S24_BE = 8,
  SK_PCM_FORMAT_U24_LE = 9,
  SK_PCM_FORMAT_U24_BE = 10,
  SK_PCM_FORMAT_S32_LE = 11,
  SK_PCM_FORMAT_S32_BE = 12,
  SK_PCM_FORMAT_U32_LE = 13,
  SK_PCM_FORMAT_U32_BE = 14,
  SK_PCM_FORMAT_F32_LE = 15,
  SK_PCM_FORMAT_F32_BE = 16,
  SK_PCM_FORMAT_F64_LE = 17,
  SK_PCM_FORMAT_F64_BE = 18,
  SK_PCM_FORMAT_BEGIN_RANGE = SK_PCM_FORMAT_UNKNOWN,
  SK_PCM_FORMAT_END_RANGE = SK_PCM_FORMAT_F64_BE,
  SK_PCM_FORMAT_RANGE_SIZE = (SK_PCM_FORMAT_F64_BE - SK_PCM_FORMAT_UNKNOWN + 1),
  SK_PCM_FORMAT_MAX_ENUM = 0x7FFFFFFF
} SkPcmFormat;

typedef enum SkChannel {
  SK_CHANNEL_UNKNOWN = 0,
  SK_CHANNEL_NOT_APPLICABLE = 1,
  SK_CHANNEL_MONO = 2,
  SK_CHANNEL_FRONT_LEFT = 3,
  SK_CHANNEL_FRONT_RIGHT = 4,
  SK_CHANNEL_FRONT_CENTER = 5,
  SK_CHANNEL_LOW_FREQUENCY = 6,
  SK_CHANNEL_SIDE_LEFT = 7,
  SK_CHANNEL_SIDE_RIGHT = 8,
  SK_CHANNEL_BACK_LEFT = 9,
  SK_CHANNEL_BACK_RIGHT = 10,
  SK_CHANNEL_BACK_CENTER = 11,
  SK_CHANNEL_BEGIN_RANGE = SK_CHANNEL_UNKNOWN,
  SK_CHANNEL_END_RANGE = SK_CHANNEL_BACK_CENTER,
  SK_CHANNEL_RANGE_SIZE = (SK_CHANNEL_BACK_CENTER - SK_CHANNEL_UNKNOWN + 1),
  SK_CHANNEL_MAX_ENUM = 0x7FFFFFFF
} SkChannel;

typedef enum SkDeviceType {
  SK_DEVICE_TYPE_UNKNOWN = 0,
  SK_DEVICE_TYPE_OTHER = 1,
  SK_DEVICE_TYPE_INTERNAL = 2,
  SK_DEVICE_TYPE_MICROPHONE = 3,
  SK_DEVICE_TYPE_SPEAKER = 4,
  SK_DEVICE_TYPE_HEADPHONE = 5,
  SK_DEVICE_TYPE_WEBCAM = 6,
  SK_DEVICE_TYPE_HEADSET = 7,
  SK_DEVICE_TYPE_HANDSET = 8,
  SK_DEVICE_TYPE_BEGIN_RANGE = SK_DEVICE_TYPE_UNKNOWN,
  SK_DEVICE_TYPE_END_RANGE = SK_DEVICE_TYPE_HANDSET,
  SK_DEVICE_TYPE_RANGE_SIZE = (SK_DEVICE_TYPE_HANDSET - SK_DEVICE_TYPE_UNKNOWN + 1),
  SK_DEVICE_TYPE_MAX_ENUM = 0x7FFFFFFF
} SkDeviceType;

typedef enum SkInstanceCreateFlagBits {
  SK_INSTANCE_CREATE_NO_IMPLICIT_OBJECTS_BIT = 0x00000001,
  SK_INSTANCE_CREATE_NO_SYSTEM_OBJECTS_BIT = 0x00000002,
  SK_INSTANCE_CREATE_FLAG_BITS_MASK = 0x00000003,
  SK_INSTANCE_CREATE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} SkInstanceCreateFlagBits;
typedef SkFlags SkInstanceCreateFlags;

typedef enum SkStreamFlagBits {
  SK_STREAM_PCM_READ_BIT = 0x00000001,
  SK_STREAM_PCM_WRITE_BIT = 0x00000002,
  SK_STREAM_PCM_MASK = 0x00000003,
  SK_STREAM_MIDI_READ_BIT = 0x00000004,
  SK_STREAM_MIDI_WRITE_BIT = 0x00000008,
  SK_STREAM_MIDI_MASK = 0x0000000C,
  SK_STREAM_VIDEO_READ_BIT = 0x00000010,
  SK_STREAM_VIDEO_WRITE_BIT = 0x00000020,
  SK_STREAM_VIDEO_MASK = 0x00000030,
  SK_STREAM_FLAG_BITS_MASK = 0x0000003F,
  SK_STREAM_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} SkStreamFlagBits;
typedef SkFlags SkStreamFlags;

typedef enum SkAccessFlagBits {
  SK_ACCESS_BLOCKING_BIT = 0x00000001,
  SK_ACCESS_INTERLEAVED_BIT = 0x00000002,
  SK_ACCESS_MEMORY_MAPPED_BIT = 0x00000004,
  SK_ACCESS_FLAG_BITS_MASK = 0x00000007,
  SK_ACCESS_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
} SkAccessFlagBits;
typedef SkFlags SkAccessFlags;

#define SK_ACCESS_NONBLOCKING 0x0
#define SK_ACCESS_BLOCKING SK_ACCESS_BLOCKING_BIT
#define SK_ACCESS_NONINTERLEAVED 0x0
#define SK_ACCESS_INTERLEAVED SK_ACCESS_INTERLEAVED_BIT
#define SK_ACCESS_BUFFERED 0x0
#define SK_ACCESS_MEMORY_MAPPED SK_ACCESS_MEMORY_MAPPED_BIT

////////////////////////////////////////////////////////////////////////////////
// Standard Function Pointers
////////////////////////////////////////////////////////////////////////////////

typedef void (SKAPI_PTR *PFN_skVoidFunction)(void);
typedef void* (SKAPI_PTR *PFN_skAllocationFunction)(void* pUserData, size_t size, size_t alignment, SkSystemAllocationScope allocationScope);
typedef void* (SKAPI_PTR *PFN_skReallocationFunction)(void* pUserData, void* pOriginal, size_t size, size_t alignment, SkSystemAllocationScope allocationScope);
typedef void (SKAPI_PTR *PFN_skFreeFunction)(void* pUserData, void* memory);
typedef PFN_skVoidFunction (SKAPI_PTR *PFN_skGetInstanceProcAddr)(SkInstance instance, char const* pName);
typedef PFN_skVoidFunction (SKAPI_PTR *PFN_skGetDriverProcAddr)(SkDriver driver, char const* pName);
typedef PFN_skVoidFunction (SKAPI_PTR *PFN_skGetPcmStreamProcAddr)(SkPcmStream stream, char const* pName);

////////////////////////////////////////////////////////////////////////////////
// Standard Structures
////////////////////////////////////////////////////////////////////////////////

typedef struct SkApplicationInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  char const*                           pApplicationName;
  uint32_t                              applicationVersion;
  char const*                           pEngineName;
  uint32_t                              engineVersion;
  uint32_t                              apiVersion;
} SkApplicationInfo;

typedef struct SkInstanceCreateInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  SkInstanceCreateFlags                 flags;
  SkApplicationInfo const*              pApplicationInfo;
  uint32_t                              enabledLayerCount;
  char const* const*                    ppEnabledLayerNames;
  uint32_t                              enabledExtensionCount;
  char const* const*                    ppEnabledExtensionNames;
} SkInstanceCreateInfo;

typedef struct SkAllocationCallbacks {
  void*                                 pUserData;
  PFN_skAllocationFunction              pfnAllocation;
  PFN_skReallocationFunction            pfnReallocation;
  PFN_skFreeFunction                    pfnFree;
} SkAllocationCallbacks;

typedef struct SkLayerProperties {
  uint32_t                              apiVersion;
  uint32_t                              implVersion;
  uint8_t                               layerUuid[SK_UUID_SIZE];
  char                                  layerName[SK_MAX_NAME_SIZE];
  char                                  displayName[SK_MAX_NAME_SIZE];
  char                                  description[SK_MAX_DESCRIPTION_SIZE];
} SkLayerProperties;

typedef struct SkLayerCreateInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  SkLayerProperties                     properties;
  PFN_skGetInstanceProcAddr             pfnGetInstanceProcAddr;
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr;
  PFN_skGetPcmStreamProcAddr            pfnGetPcmStreamProcAddr;
} SkLayerCreateInfo;

typedef struct SkDriverFeatures {
  SkStreamFlags                         supportedStreams;
  SkAccessFlags                         supportedAccessModes;
  SkBool32                              defaultEndpoint;
} SkDriverFeatures;

typedef struct SkDriverProperties {
  uint32_t                              apiVersion;
  uint32_t                              implVersion;
  uint8_t                               driverUuid[SK_UUID_SIZE];
  char                                  identifier[SK_MAX_IDENTIFIER_SIZE];
  char                                  driverName[SK_MAX_NAME_SIZE];
  char                                  displayName[SK_MAX_NAME_SIZE];
  char                                  description[SK_MAX_DESCRIPTION_SIZE];
} SkDriverProperties;

typedef struct SkDriverCreateInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  SkDriverProperties                    properties;
  PFN_skGetDriverProcAddr               pfnGetDriverProcAddr;
} SkDriverCreateInfo;

typedef struct SkDeviceFeatures {
  SkStreamFlags                         supportedStreams;
} SkDeviceFeatures;

typedef struct SkDeviceProperties {
  uint32_t                              vendorID;
  uint32_t                              deviceID;
  SkDeviceType                          deviceType;
  char                                  deviceName[SK_MAX_NAME_SIZE];
  char                                  driverName[SK_MAX_NAME_SIZE];
  char                                  mixerName[SK_MAX_NAME_SIZE];
  uint8_t                               deviceUuid[SK_UUID_SIZE];
} SkDeviceProperties;

typedef struct SkEndpointFeatures {
  SkStreamFlags                         supportedStreams;
} SkEndpointFeatures;

typedef struct SkEndpointProperties {
  char                                  endpointName[SK_MAX_NAME_SIZE];
  char                                  displayName[SK_MAX_NAME_SIZE];
  uint8_t                               endpointUuid[SK_UUID_SIZE];
} SkEndpointProperties;

typedef struct SkPcmStreamRequest {
  SkStructureType                       sType;
  void const*                           pNext;
  SkStreamFlagBits                      streamType;
  SkAccessFlags                         accessFlags;
  SkPcmFormat                           formatType;
  uint32_t                              sampleRate;
  uint32_t                              channels;
  uint32_t                              bufferSamples;
} SkPcmStreamRequest;

typedef struct SkPcmStreamInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  SkStreamFlagBits                      streamType;
  SkAccessFlags                         accessFlags;
  SkPcmFormat                           formatType;
  uint32_t                              formatBits;
  uint32_t                              sampleRate;
  uint32_t                              sampleBits;
  uint32_t                              offsetBits;
  uint32_t                              channels;
  uint32_t                              frameBits;
  uint32_t                              periodSamples;
  uint32_t                              periodBits;
  uint32_t                              bufferSamples;
  uint32_t                              bufferBits;
} SkPcmStreamInfo;

typedef struct SkMidiStreamInfo SkMidiStreamInfo;
typedef struct SkMidiStreamInfo SkMidiStreamRequest;

typedef struct SkVideoStreamInfo SkVideoStreamInfo;
typedef struct SkVideoStreamInfo SkVideoStreamRequest;

////////////////////////////////////////////////////////////////////////////////
// Public API (Core 1.0)
////////////////////////////////////////////////////////////////////////////////

// Instance
typedef SkResult (SKAPI_PTR *PFN_skCreateInstance)(SkInstanceCreateInfo const* pCreateInfo, SkAllocationCallbacks const* pAllocator, SkInstance* pInstance);
typedef SkResult (SKAPI_PTR *PFN_skDestroyInstance)(SkInstance instance, SkAllocationCallbacks const* pAllocator);
typedef SkResult (SKAPI_PTR *PFN_skEnumerateLayerProperties)(uint32_t* pLayerCount, SkLayerProperties* pProperties);
typedef SkObject (SKAPI_PTR *PFN_skResolveParent)(SkObject object);
typedef SkObject (SKAPI_PTR *PFN_skResolveObject)(SkObject object, char const* path);
typedef SkResult (SKAPI_PTR *PFN_skEnumerateInstanceDrivers)(SkInstance instance, uint32_t* pDriverCount, SkDriver* pDriver);

// Drivers
typedef void (SKAPI_PTR *PFN_skGetDriverProperties)(SkDriver driver, SkDriverProperties* pProperties);
typedef void (SKAPI_PTR *PFN_skGetDriverFeatures)(SkDriver driver, SkDriverFeatures* pFeatures);
typedef SkResult (SKAPI_PTR *PFN_skEnumerateDriverDevices)(SkDriver driver, uint32_t* pDeviceCount, SkDevice* pDevices);
typedef SkResult (SKAPI_PTR *PFN_skEnumerateDriverEndpoints)(SkDriver driver, uint32_t* pEndpointCount, SkEndpoint* pEndpoints);
typedef SkResult (SKAPI_PTR *PFN_skQueryDeviceFeatures)(SkDevice device, SkDeviceFeatures* pFeatures);
typedef SkResult (SKAPI_PTR *PFN_skQueryDeviceProperties)(SkDevice device, SkDeviceProperties* pProperties);
typedef SkResult (SKAPI_PTR *PFN_skEnumerateDeviceEndpoints)(SkDevice device, uint32_t* pEndpointCount, SkEndpoint* pEndpoints);
typedef SkResult (SKAPI_PTR *PFN_skQueryEndpointFeatures)(SkEndpoint endpoint, SkEndpointFeatures *pFeatures);
typedef SkResult (SKAPI_PTR *PFN_skQueryEndpointProperties)(SkEndpoint endpoint, SkEndpointProperties *pProperties);
typedef SkResult (SKAPI_PTR *PFN_skRequestPcmStream)(SkEndpoint endpoint, SkPcmStreamRequest const* pRequest, SkPcmStream* pStream);

// PCM Streams
typedef SkResult (SKAPI_PTR *PFN_skClosePcmStream)(SkPcmStream stream, SkBool32 drain);
typedef SkResult (SKAPI_PTR *PFN_skGetPcmStreamInfo)(SkPcmStream stream, SkStreamFlagBits streamType, SkPcmStreamInfo* pStreamInfo);
typedef SkResult (SKAPI_PTR *PFN_skGetPcmStreamChannelMap)(SkPcmStream stream, SkStreamFlagBits streamType, SkChannel* pChannelMap);
typedef SkResult (SKAPI_PTR *PFN_skSetPcmStreamChannelMap)(SkPcmStream stream, SkStreamFlagBits streamType, SkChannel const* channelMap);
typedef SkResult (SKAPI_PTR *PFN_skStartPcmStream)(SkPcmStream stream);
typedef SkResult (SKAPI_PTR *PFN_skStopPcmStream)(SkPcmStream stream, SkBool32 drain);
typedef SkResult (SKAPI_PTR *PFN_skPausePcmStream)(SkPcmStream stream, SkBool32 pause);
typedef SkResult (SKAPI_PTR *PFN_skRecoverPcmStream)(SkPcmStream stream);
typedef SkResult (SKAPI_PTR *PFN_skWaitPcmStream)(SkPcmStream stream, SkStreamFlagBits streamType, int32_t timeout);
typedef SkResult (SKAPI_PTR *PFN_skAvailPcmStreamSamples)(SkPcmStream stream, SkStreamFlagBits streamType, uint32_t* pSamples);
typedef SkResult (SKAPI_PTR *PFN_skWritePcmStreamInterleaved)(SkPcmStream stream, void const* pBuffer, uint32_t samples);
typedef SkResult (SKAPI_PTR *PFN_skWritePcmStreamNoninterleaved)(SkPcmStream stream, void** pBuffer, uint32_t samples);
typedef SkResult (SKAPI_PTR *PFN_skReadPcmStreamInterleaved)(SkPcmStream stream, void* pBuffer, uint32_t samples);
typedef SkResult (SKAPI_PTR *PFN_skReadPcmStreamNoninterleaved)(SkPcmStream stream, void** pBuffer, uint32_t samples);

#ifndef   SK_NO_PROTOTYPES

// Instance

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetInstanceProcAddr(
  SkInstance                            instance,
  char const*                           pName
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLayerProperties(
  uint32_t*                             pLayerCount,
  SkLayerProperties*                    pProperties
);

SKAPI_ATTR SkObject SKAPI_CALL skResolveParent(
  SkObject                              object
);

SKAPI_ATTR SkObject SKAPI_CALL skResolveObject(
  SkObject                              object,
  char const*                           pPath
);

SKAPI_ATTR SkResult SKAPI_CALL skCreateInstance(
  SkInstanceCreateInfo const*           pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkInstance*                           pInstance
);

SKAPI_ATTR SkResult SKAPI_CALL skDestroyInstance(
  SkInstance                            instance,
  SkAllocationCallbacks const*          pAllocator
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateInstanceDrivers(
  SkInstance                            instance,
  uint32_t*                             pDriverCount,
  SkDriver*                             pDriver
);

// Drivers

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr(
  SkDriver                              driver,
  char const*                           pName
);

SKAPI_ATTR void SKAPI_CALL skGetDriverProperties(
  SkDriver                              driver,
  SkDriverProperties*                   pProperties
);

SKAPI_ATTR void SKAPI_CALL skGetDriverFeatures(
  SkDriver                              driver,
  SkDriverFeatures*                     pFeatures
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDriverEndpoints(
  SkDriver                              driver,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDriverDevices(
  SkDriver                              driver,
  uint32_t*                             pDeviceCount,
  SkDevice*                             pDevices
);

SKAPI_ATTR SkResult SKAPI_CALL skQueryDeviceProperties(
  SkDevice                              device,
  SkDeviceProperties*                   pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skQueryDeviceFeatures(
  SkDevice                              device,
  SkDeviceFeatures*                     pFeatures
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateDeviceEndpoints(
  SkDevice                              device,
  uint32_t*                             pEndpointCount,
  SkEndpoint*                           pEndpoints
);

SKAPI_ATTR SkResult SKAPI_CALL skQueryEndpointFeatures(
  SkEndpoint                            endpoint,
  SkEndpointFeatures*                   pFeatures
);

SKAPI_ATTR SkResult SKAPI_CALL skQueryEndpointProperties(
  SkEndpoint                            endpoint,
  SkEndpointProperties*                 pProperties
);

SKAPI_ATTR SkResult SKAPI_CALL skRequestPcmStream(
  SkEndpoint                            endpoint,
  SkPcmStreamRequest const*             pStreamRequest,
  SkPcmStream*                          pStream
);

// PCM Stream

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr(
  SkPcmStream                           stream,
  char const*                           pName
);

SKAPI_ATTR SkResult SKAPI_CALL skClosePcmStream(
  SkPcmStream                           stream,
  SkBool32                              drain
);

SKAPI_ATTR SkResult SKAPI_CALL skGetPcmStreamInfo(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkPcmStreamInfo*                      pStreamInfo
);

SKAPI_ATTR SkResult SKAPI_CALL skGetPcmStreamChannelMap(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel*                            pChannelMap
);

SKAPI_ATTR SkResult SKAPI_CALL skSetPcmStreamChannelMap(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  SkChannel const*                      pChannelMap
);

SKAPI_ATTR SkResult SKAPI_CALL skStartPcmStream(
  SkPcmStream                           stream
);

SKAPI_ATTR SkResult SKAPI_CALL skStopPcmStream(
  SkPcmStream                           stream,
  SkBool32                              drain
);

SKAPI_ATTR SkResult SKAPI_CALL skPausePcmStream(
  SkPcmStream                           stream,
  SkBool32                              pause
);

SKAPI_ATTR SkResult SKAPI_CALL skRecoverPcmStream(
  SkPcmStream                           stream
);

SKAPI_ATTR SkResult SKAPI_CALL skWaitPcmStream(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  int32_t                               timeout
);

SKAPI_ATTR SkResult SKAPI_CALL skAvailPcmStreamSamples(
  SkPcmStream                           stream,
  SkStreamFlagBits                      streamType,
  uint32_t*                             pSamples
);

SKAPI_ATTR int64_t SKAPI_CALL skWritePcmStreamInterleaved(
  SkPcmStream                           stream,
  void const*                           pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skWritePcmStreamNoninterleaved(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skReadPcmStreamInterleaved(
  SkPcmStream                           stream,
  void*                                 pBuffer,
  uint32_t                              samples
);

SKAPI_ATTR int64_t SKAPI_CALL skReadPcmStreamNoninterleaved(
  SkPcmStream                           stream,
  void**                                pBuffer,
  uint32_t                              samples
);

#endif // SK_NO_PROTOTYPES

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_H
