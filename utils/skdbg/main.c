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
 * A developer utility for whoever implements OpenSK features. Checks allocation
 * routines, runs basic tests, extracts debug information. General debugging.
 ******************************************************************************/
#define _USE_MATH_DEFINES

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/ext/sk_loader.h>
#include <OpenSK/plt/platform.h>
#include <OpenSK/utl/allocators.h>
#include <OpenSK/utl/macros.h>
#include <OpenSK/utl/string.h>

#ifdef    OPENSK_ALSA
#include <OpenSK/icd/alsa.h>
#endif // OPENSK_ALSA

// Supported Manifests:
#include <OpenSK/man/manifest.h>
#include <OpenSK/man/manifest_1_0_0.h>

// C99
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <OpenSK/dev/json.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/ext/sk_driver.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

typedef enum VerificationStep {
  VS_MANIFEST_VERIFY,
  VS_MANIFEST_ENUMERATE,
  VS_MANIFEST_CREATE,
  VS_LOADER_INITIALIZE,
  VS_INSTANCE_CREATE,
  VS_INSTANCE_VALIDATE,
  VS_INSTANCE_RESOLVE_DRIVERS,
  VS_INSTANCE_ENUMERATE_DRIVERS,
  VS_DRIVER_FEATURES,
  VS_DRIVER_PROPERTIES,
  VS_DRIVER_ENUMERATE_ENDPOINTS,
  VS_DRIVER_ENUMERATE_DEVICES,
  VS_DRIVER_ENDPOINTS_VALIDATE,
  VS_DRIVER_DEVICES_VALIDATE,
  VS_DEVICE_PROPERTIES,
  VS_DEVICE_FEATURES,
  VS_DEVICE_ENUMERATE_ENDPOINTS,
  VS_DEVICE_ENDPOINTS_VALIDATE,
  VS_ENDPOINT_FEATURES,
  VS_ENDPOINT_PROPERTIES,
  VS_ENDPOINT_VALIDATE,
  VS_MAX_COUNT
} VerificationStep;

typedef enum FileType {
  FT_MANIFEST
} FileType;

typedef enum AllocatorType {
  AT_DEBUG
} AllocatorType;

struct SkExecUTL;
typedef void (SKAPI_PTR *PFN_skSectionCallback)(char const* name, struct SkExecUTL* exec);

typedef struct DebugFile {
  char const*                           pFilename;
  FileType                              fileType;
} DebugFile;

typedef struct SkExecUTL {
  SkAllocationCallbacks const*          pSystemAllocator;
  SkAllocationCallbacks*                pUserAllocator;
  AllocatorType                         allocatorType;
  SkApplicationInfo                     applicationInfo;
  DebugFile*                            files;
  uint32_t                              fileCount;
  PFN_skSectionCallback                 sectionBegin;
  PFN_skSectionCallback                 sectionEnd;
  uint32_t                              verbose;
  SkBool32                              verify[VS_MAX_COUNT];
} SkExecUTL;

#define ONCE for (SkBool32 _once_ = SK_TRUE; _once_; _once_ = SK_FALSE)

static char const* requiredLayers[] = {
  "SK_LAYER_OPENSK_VALIDATION"
};

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static float
sine(float t) {
  return (float)sin(2.0f * M_PI * t);
}

static int
check(SkExecUTL* exec, VerificationStep vs) {
  return (exec->verify[vs]);
}

#define CASE(v) case v: return #v
static char const*
verification_string(VerificationStep verification) {
  switch (verification) {
    CASE(VS_MANIFEST_VERIFY);
    CASE(VS_MANIFEST_ENUMERATE);
    CASE(VS_MANIFEST_CREATE);
    CASE(VS_LOADER_INITIALIZE);
    CASE(VS_INSTANCE_CREATE);
    CASE(VS_INSTANCE_VALIDATE);
    CASE(VS_INSTANCE_RESOLVE_DRIVERS);
    CASE(VS_INSTANCE_ENUMERATE_DRIVERS);
    CASE(VS_DRIVER_FEATURES);
    CASE(VS_DRIVER_PROPERTIES);
    CASE(VS_DRIVER_ENUMERATE_ENDPOINTS);
    CASE(VS_DRIVER_ENUMERATE_DEVICES);
    CASE(VS_DRIVER_ENDPOINTS_VALIDATE);
    CASE(VS_DRIVER_DEVICES_VALIDATE);
    CASE(VS_DEVICE_PROPERTIES);
    CASE(VS_DEVICE_FEATURES);
    CASE(VS_DEVICE_ENUMERATE_ENDPOINTS);
    CASE(VS_DEVICE_ENDPOINTS_VALIDATE);
    CASE(VS_ENDPOINT_FEATURES);
    CASE(VS_ENDPOINT_PROPERTIES);
    CASE(VS_ENDPOINT_VALIDATE);
    case VS_MAX_COUNT:
      return NULL;
  }
  return NULL;
}
#undef CASE

static void SKAPI_CALL
begin_section(char const* sectionName, SkExecUTL* exec) {
  if (exec->verbose > 1) {
    printf("%s...\n", sectionName);
  }
}

static void SKAPI_CALL
end_section(char const* sectionName, SkExecUTL* exec) {
  if (exec->verbose > 1) {
    if (exec->pUserAllocator) {
      switch (exec->allocatorType) {
        case AT_DEBUG:
          skPrintDebugAllocatorStatisticsUTL(exec->pUserAllocator);
          break;
      }
    }
    printf("%s Done!\n\n", sectionName);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////
static void
print_usage() {
  printf(
    "Usage: skdbg [options] <manifests...>\n"
    "\n"
    "Debugs a particular manifest to make sure that it is valid.\n"
    "  This utility is meant for OpenSK developers to test their changes.\n"
    "\n"
    "Options:\n"
    "  -h, --help         Prints this help documentation.\n"
    "  -s, --steps        Enables the provided steps (specified by name or number).\n"
    "  -S, --list-steps   List all of the available validation steps.\n"
    "  -i, --ignore       Ignore the provided steps (specified by name or number).\n"
    "  -l, --list-layers  List all of the layers which are found on the host.\n"
    "  -d, --list-drivers List all of the drivers which are found on the host.\n"
    "  -v, --verbose      Print more detailed information about what is being tested.\n"
    "  -V, --verbose=high Print all of the allocation information along with detailed info.\n"
    "\n"
    "OpenSK is copyright Trent Reed 2016 - All rights reserved.\n"
    "Full documentation can be found online at <http://www.opensk.org/>.\n"
  );
}


#define CHECK_NOSUB(func) do {\
pfnVoidFunction = skGetPcmStreamProcAddr(stream, #func);\
if (!pfnVoidFunction) {\
  SKERR("PCM Stream is missing the function: " #func "\n");\
  ++err;\
}} while (0)
#define CHECK(func) do {\
pfnVoidFunction = skGetPcmStreamProcAddr(stream, #func);\
if (!pfnVoidFunction || pfnVoidFunction == (PFN_skVoidFunction)&func) {\
  SKERR("PCM Stream is missing the function: " #func "\n");\
  ++err;\
}} while (0)
static int
validate_pcm_0_0(SkExecUTL* exec, SkPcmStream stream) {
  int err;
  PFN_skVoidFunction pfnVoidFunction;

  err = 0;
  exec->sectionBegin("Validate PCM Exports (0.0.0)", exec);
  {
    CHECK_NOSUB(skCreatePcmStream);
    CHECK_NOSUB(skDestroyPcmStream);
    CHECK(skGetPcmStreamProcAddr);
    CHECK(skClosePcmStream);
    CHECK(skGetPcmStreamInfo);
    CHECK(skGetPcmStreamChannelMap);
    CHECK(skSetPcmStreamChannelMap);
    CHECK(skStartPcmStream);
    CHECK(skStopPcmStream);
    CHECK(skRecoverPcmStream);
    CHECK(skWaitPcmStream);
    CHECK(skPausePcmStream);
    CHECK(skAvailPcmStreamSamples);
    CHECK(skReadPcmStreamInterleaved);
    CHECK(skReadPcmStreamNoninterleaved);
    CHECK(skWritePcmStreamInterleaved);
    CHECK(skWritePcmStreamNoninterleaved);
  }
  exec->sectionEnd("Validate PCM Exports (0.0.0)", exec);

  return err;
}
#undef CHECK
#undef CHECK_NOSUB

static int
validate_endpoint_pcm_write(SkExecUTL* exec, SkDriverProperties const* pDriverProperties, SkEndpointProperties const* pProperties, SkEndpoint endpoint) {
  int err;
  uint32_t idx;
  uint32_t ddx;
  char* sample;
  char* waveform;
  float currFreq;
  float addrFreq;
  int64_t played;
  SkResult result;
  uint32_t remaining;
  int16_t currSample;
  SkPcmStream stream;
  SkPcmStreamInfo info;
  SkPcmStreamRequest request;

  // Configure the request (use a common request format with high latency)
  printf(".   Validating PCM Write (%s)\n", pProperties->displayName);
  memset(&request, 0, sizeof(SkPcmStreamRequest));
  request.sType = SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST;
  request.accessFlags = SK_ACCESS_INTERLEAVED | SK_ACCESS_BLOCKING;
  request.streamType = SK_STREAM_PCM_WRITE_BIT;
  request.formatType = SK_PCM_FORMAT_S16_LE;
  request.channels = 2;
  request.sampleRate = 44100;
  request.bufferSamples = 4096;

  // Request the PCM stream for writing the sine wave to.
  result = skRequestPcmStream(endpoint, &request, &stream);
  if (result != SK_SUCCESS) {
    switch (result) {
      case SK_ERROR_BUSY:
        // Endpoint is at-capacity, this is not a problem, and is expected.
        return 0;
      case SK_ERROR_INVALID:
        fprintf(stderr, "The provided stream request is invalid and cannot be supported by the driver.\n");
        break;
      case SK_ERROR_NOT_SUPPORTED:
        fprintf(stderr, "The requested playback stream does not support the requested configuration.\n");
        return 0;
      case SK_ERROR_OUT_OF_HOST_MEMORY:
        fprintf(stderr, "The driver failed to allocate the memory necessary to process the request.\n");
        break;
      case SK_ERROR_SYSTEM_INTERNAL:
        fprintf(stderr, "The requested playback stream could not be acquired for unknown reasons.\n");
        break;
      default:
        fprintf(stderr, "Unexpected result returned from skRequestPcmStream()! Contact a developer!\n");
        break;
    }
    return 1;
  }
  result = skGetPcmStreamInfo(stream, SK_STREAM_PCM_WRITE_BIT, &info);
  if (result != SK_SUCCESS) {
    fprintf(stderr, "Failed to acquire PCM stream info - aborting.\n");
    skClosePcmStream(stream, SK_TRUE);
    return 1;
  }
  if (exec->verbose) {
    printf("info.sType: %u\n", info.sType);
    printf("info.pNext: %p\n", info.pNext);
    printf("info.streamType: %u\n", info.streamType);
    printf("info.accessFlags: %u\n", info.accessFlags);
    printf("info.formatType: %u\n", info.formatType);
    printf("info.formatBits: %u\n", info.formatBits);
    printf("info.sampleRate: %u\n", info.sampleRate);
    printf("info.sampleBits: %u\n", info.sampleBits);
    printf("info.channels: %u\n", info.channels);
    printf("info.frameBits: %u\n", info.frameBits);
    printf("info.periodSamples: %u\n", info.periodSamples);
    printf("info.periodBits: %u\n", info.periodBits);
    printf("info.bufferSamples: %u\n", info.bufferSamples);
    printf("info.bufferBits: %u\n", info.bufferBits);
  }

  // Validate the functions present on this stream.
  switch (pDriverProperties->apiVersion) {
    case SK_API_VERSION_0_0:
      err = validate_pcm_0_0(exec, stream);
      if (err != 0) {
        return err;
      }
      break;
    default:
      SKERR("Invalid OpenSK API version detected - continuing may be erroneous!\n");
      return 1;
  }

  // Allocate a buffer for the sine waveform.
  waveform = skAllocate(
    exec->pSystemAllocator,
    info.frameBits * info.sampleRate / 8,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!waveform) {
    SKERR("Failed to allocate enough space for %u samples.\n", info.sampleRate);
    skClosePcmStream(stream, SK_TRUE);
    return 1;
  }

  // Generate the sine wave and prepare to play a one second wave.
  currFreq = 0.0f;
  sample = waveform;
  addrFreq = 261.63f / info.sampleRate;
  for (idx = 0; idx < info.sampleRate; ++idx) {
    currFreq += addrFreq;
    currFreq -= (float)floor(currFreq);
    currSample = (int16_t)(sine(currFreq) * 0.05f * INT16_MAX);
    for (ddx = 0; ddx < info.channels; ++ddx) {
      *((int16_t*)sample) = currSample;
      sample += info.sampleBits / 8;
    }
  }

  // Populate the PCM stream with the waveform
  sample = waveform;
  remaining = info.sampleRate;
  while (remaining) {
    played = skWritePcmStreamInterleaved(stream, sample, remaining);
    if (played < 0) {
      switch (played) {
        case SK_ERROR_BUSY:
          // Not accepting more samples - utl in non-blocking mode.
          break;
        case SK_ERROR_XRUN:
          fprintf(stderr, "Underrun - were not able to provide samples fast-enough!\n");
          break;
        case SK_ERROR_SUSPENDED:
          fprintf(stderr, "Suspended!\n");
          break;
        default:
          // Nothing to do otherwise, we should quit.
          return -2;
      }
      played = skRecoverPcmStream(stream);
      if (played != SK_SUCCESS) {
        SKERR("Failed recovering - aborting.\n");
        skFree(exec->pSystemAllocator, waveform);
        skClosePcmStream(stream, SK_TRUE);
        return 1;
      }
    }
    remaining -= (uint32_t)played;
    sample += played * info.frameBits / 8;
  }

  skFree(exec->pSystemAllocator, waveform);
  skClosePcmStream(stream, SK_TRUE);
  return 0;
}

static int
validate_endpoint_pcm_read(SkExecUTL* exec, SkDriverProperties const* pDriverProperties, SkEndpointProperties const* pProperties, SkEndpoint endpoint) {
  int err;
  char* sample;
  char* waveform;
  int64_t recorded;
  SkResult result;
  uint32_t remaining;
  SkPcmStream stream;
  SkPcmStreamInfo info;
  SkPcmStreamRequest request;

  // Configure the request (use a common request format with high latency)
  printf(".   Validating PCM Read (%s)\n", pProperties->displayName);
  memset(&request, 0, sizeof(SkPcmStreamRequest));
  request.sType = SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST;
  request.accessFlags = SK_ACCESS_INTERLEAVED | SK_ACCESS_BLOCKING;
  request.streamType = SK_STREAM_PCM_READ_BIT;
  request.formatType = SK_PCM_FORMAT_S16_LE;
  request.channels = 2;
  request.sampleRate = 44100;
  request.bufferSamples = 4096;

  // Request the PCM stream for reading the wavefrom from.
  result = skRequestPcmStream(endpoint, &request, &stream);
  if (result != SK_SUCCESS) {
    switch (result) {
      case SK_ERROR_BUSY:
        // Endpoint is at-capacity, this is not a problem, and is expected.
        return 0;
      case SK_ERROR_INVALID:
        fprintf(stderr, "The provided stream request is invalid and cannot be supported by the driver.\n");
        break;
      case SK_ERROR_NOT_SUPPORTED:
        fprintf(stderr, "The requested playback stream does not support the requested configuration.\n");
        return 0;
      case SK_ERROR_OUT_OF_HOST_MEMORY:
        fprintf(stderr, "The driver failed to allocate the memory necessary to process the request.\n");
        break;
      case SK_ERROR_SYSTEM_INTERNAL:
        fprintf(stderr, "The requested playback stream could not be acquired for unknown reasons.\n");
        break;
      default:
        fprintf(stderr, "Unexpected result returned from skRequestPcmStream()! Contact a developer!\n");
        break;
    }
    return 1;
  }
  result = skGetPcmStreamInfo(stream, SK_STREAM_PCM_READ_BIT, &info);
  if (result != SK_SUCCESS) {
    fprintf(stderr, "Failed to acquire PCM stream info - aborting.\n");
    skClosePcmStream(stream, SK_TRUE);
    return 1;
  }
  if (exec->verbose) {
    printf("info.sType: %u\n", info.sType);
    printf("info.pNext: %p\n", info.pNext);
    printf("info.streamType: %u\n", info.streamType);
    printf("info.accessFlags: %u\n", info.accessFlags);
    printf("info.formatType: %u\n", info.formatType);
    printf("info.formatBits: %u\n", info.formatBits);
    printf("info.sampleRate: %u\n", info.sampleRate);
    printf("info.sampleBits: %u\n", info.sampleBits);
    printf("info.channels: %u\n", info.channels);
    printf("info.frameBits: %u\n", info.frameBits);
    printf("info.periodSamples: %u\n", info.periodSamples);
    printf("info.periodBits: %u\n", info.periodBits);
    printf("info.bufferSamples: %u\n", info.bufferSamples);
    printf("info.bufferBits: %u\n", info.bufferBits);
  }

  // Validate the functions present on this stream.
  switch (pDriverProperties->apiVersion) {
    case SK_API_VERSION_0_0:
      err = validate_pcm_0_0(exec, stream);
      if (err != 0) {
        return err;
      }
      break;
    default:
      SKERR("Invalid OpenSK API version detected - continuing may be erroneous!\n");
      return 1;
  }

  // Allocate a buffer for the sine waveform.
  waveform = skAllocate(
    exec->pSystemAllocator,
    info.frameBits * info.sampleRate / 8,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!waveform) {
    SKERR("Failed to allocate enough space for %u samples.\n", info.sampleRate);
    skClosePcmStream(stream, SK_TRUE);
    return 1;
  }

  // Populate the PCM stream with the waveform
  sample = waveform;
  remaining = info.sampleRate;
  while (remaining) {
    recorded = skReadPcmStreamInterleaved(stream, sample, remaining);
    if (recorded < 0) {
      switch (recorded) {
        case SK_ERROR_BUSY:
          // Not accepting more samples - utl in non-blocking mode.
          break;
        case SK_ERROR_XRUN:
          fprintf(stderr, "Underrun - were not able to provide samples fast-enough!\n");
          break;
        case SK_ERROR_SUSPENDED:
          fprintf(stderr, "Suspended!\n");
          break;
        default:
          // Nothing to do otherwise, we should quit.
          return -2;
      }
      recorded = skRecoverPcmStream(stream);
      if (recorded != SK_SUCCESS) {
        SKERR("Failed recovering - aborting.\n");
        skFree(exec->pSystemAllocator, waveform);
        skClosePcmStream(stream, SK_TRUE);
        return 1;
      }
    }
    remaining -= (uint32_t)recorded;
    sample += recorded * info.frameBits / 8;
  }

  skFree(exec->pSystemAllocator, waveform);
  skClosePcmStream(stream, SK_TRUE);
  return 0;
}

static int
validate_endpoint_features(SkExecUTL* exec, SkDriverProperties const* pDriverProperties, SkEndpoint endpoint, SkEndpointProperties const* pProperties, SkEndpointFeatures* features) {
  int err;

  err = 0;
  if (features->supportedStreams & SK_STREAM_PCM_WRITE_BIT) {
    err += validate_endpoint_pcm_write(exec, pDriverProperties, pProperties, endpoint);
  }
  if (features->supportedStreams & SK_STREAM_PCM_READ_BIT) {
    err += validate_endpoint_pcm_read(exec, pDriverProperties, pProperties, endpoint);
  }

  return err;
}

static int
validate_endpoint(SkExecUTL* exec, SkDriverProperties const* pDriverProperties, SkObject parent, SkEndpoint endpoint) {
  int err;
  SkResult result;
  SkBool32 foundFeatures;
  SkEndpointFeatures features;
  SkEndpointProperties properties;

  err = 0;
  foundFeatures = SK_FALSE;

  if (skGetObjectType(endpoint) != SK_OBJECT_TYPE_ENDPOINT) {
    SKERR("The endpoint's oType field is not set to SK_OBJECT_TYPE_ENDPOINT!\n");
    ++err;
  }
  if (skResolveParent(endpoint) != parent) {
    SKERR("The endpoint's pParent field is not set to the expected object!\n");
    ++err;
  }

  // Check the device properties
  if (check(exec, VS_ENDPOINT_PROPERTIES)) {
    exec->sectionBegin("Endpoint Properties", exec);
    ONCE {
      result = skQueryEndpointProperties(endpoint, &properties);
      if (result != SK_SUCCESS) {
        SKERR("Failed to query for endpoint properties!\n");
        ++err;
        break;
      }
      if (exec->verbose) {
        printf("Display Name: %s\n", properties.displayName);
        printf("Endpoint Name: %s\n", properties.endpointName);
        printf("Endpoint UUID: ");
        skPrintUuidUTL(properties.endpointUuid);
        printf("\n");
      }
    }
    exec->sectionEnd("Endpoint Properties", exec);
  }

  // Check the endpoint features
  if (check(exec, VS_ENDPOINT_FEATURES)) {
    exec->sectionBegin("Endpoint Features", exec);
    ONCE {
      result = skQueryEndpointFeatures(endpoint, &features);
      if (result != SK_SUCCESS) {
        SKERR("Failed to query for endpoint features!\n");
        ++err;
        break;
      }
      foundFeatures = SK_TRUE;
    }
    exec->sectionEnd("Endpoint Features", exec);
  }

  // Check the endpoint supported streams
  if (check(exec, VS_ENDPOINT_VALIDATE)) {
    exec->sectionBegin("Endpoint Validate", exec);
    ONCE {
      if (!foundFeatures) {
        SKWRN("Attempted to validate device features, but features were not queried properly.\n");
        break;
      }
      err = validate_endpoint_features(exec, pDriverProperties, endpoint, &properties, &features);
    }
    exec->sectionEnd("Endpoint Validate", exec);
  }

  return err;
}

static int
validate_device(SkExecUTL* exec, SkDriverProperties const* pDriverProperties, SkDriver driver, SkDevice device) {
  int err;
  uint32_t idx;
  SkResult result;
  uint32_t endpointCount;
  SkEndpoint* pEndpoints;
  SkDeviceFeatures features;
  SkDeviceProperties properties;

  err = 0;
  if (skGetObjectType(device) != SK_OBJECT_TYPE_DEVICE) {
    SKERR("The device's oType field is not set to SK_OBJECT_TYPE_DEVICE");
    ++err;
  }
  if (skResolveParent(device) != driver) {
    SKERR("The device's pParent field is not set to the expected parent.\n");
    ++err;
  }

  // Check the device properties
  if (check(exec, VS_DEVICE_PROPERTIES)) {
    exec->sectionBegin("Device Properties", exec);
    ONCE {
      result = skQueryDeviceProperties(device, &properties);
      if (result != SK_SUCCESS) {
        SKERR("Failed to query for device properties!\n");
        ++err;
        break;
      }
      if (exec->verbose) {
        printf("Device ID: %u\n", properties.deviceID);
        printf("Device Vendor: %u\n", properties.vendorID);
        printf("Device Name: %s\n", properties.deviceName);
        printf("Device Type: %u\n", properties.deviceType);
        printf("Device Driver Name: %s\n", properties.driverName);
        printf("Device Mixer Name: %s\n", properties.mixerName);
        printf("Device UUID: ");
        skPrintUuidUTL(properties.deviceUuid);
        printf("\n");
      }
    }
    exec->sectionEnd("Device Properties", exec);
  }

  // Check the device features
  if (check(exec, VS_DEVICE_FEATURES)) {
    exec->sectionBegin("Device Features", exec);
    ONCE {
      result = skQueryDeviceFeatures(device, &features);
      if (result != SK_SUCCESS) {
        SKERR("Failed to query for device features!\n");
        ++err;
        break;
      }
    }
    exec->sectionEnd("Device Features", exec);
  }

  // Enumerate the device endpoints
  pEndpoints = NULL;
  endpointCount = 0;
  if (check(exec, VS_DEVICE_ENUMERATE_ENDPOINTS)) {
    exec->sectionBegin("Device Endpoints", exec);
    ONCE {
      result = skEnumerateDeviceEndpoints(device, &endpointCount, NULL);
      if (result != SK_SUCCESS) {
        SKERR("Failed to count device endpoints!\n");
        ++err;
        break;
      }
      pEndpoints = skAllocate(
        exec->pSystemAllocator,
        sizeof(SkEndpoint) * endpointCount,
        1,
        SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
      );
      if (!pEndpoints) {
        SKERR("Failed to allocate enough space for %u endpoints!\n", endpointCount);
        ++err;
        break;
      }
      result = skEnumerateDeviceEndpoints(device, &endpointCount, pEndpoints);
      if (result < SK_SUCCESS) {
        skFree(exec->pSystemAllocator, pEndpoints);
        pEndpoints = NULL;
        SKERR("Failed to enumerate device endpoints!\n");
        ++err;
        break;
      }
    }
    exec->sectionEnd("Device Endpoints", exec);
  }

  // Validate device endpoints
  if (check(exec, VS_DEVICE_ENDPOINTS_VALIDATE)) {
    exec->sectionBegin("Device Endpoints Validate", exec);
    ONCE {
      if (!check(exec, VS_DEVICE_ENUMERATE_ENDPOINTS)) {
        SKWRN("Cannot validate device endpoints and not enumerate them - ignoring.\n");
        break;
      }
      if (!pEndpoints) {
        SKWRN("Planned to validate device endpoints, but device endpoints failed to enumerate.\n");
        break;
      }
      printf("... Validating %u device endpoints\n", endpointCount);
      for (idx = 0; idx < endpointCount; ++idx) {
        err += validate_endpoint(exec, pDriverProperties, device, pEndpoints[idx]);
      }
    }
    exec->sectionEnd("Device Endpoints Validate", exec);
  }

  skFree(exec->pSystemAllocator, pEndpoints);
  return err;
}

#define CHECK_NOSUB(func) do {\
pfnVoidFunction = skGetDriverProcAddr(driver, #func);\
if (!pfnVoidFunction) {\
  SKERR("Driver is missing the function: " #func "\n");\
  ++err;\
}} while (0)
#define CHECK(func) do {\
pfnVoidFunction = skGetDriverProcAddr(driver, #func);\
if (!pfnVoidFunction || pfnVoidFunction == (PFN_skVoidFunction)&func) {\
  SKERR("Driver is missing the function: " #func "\n");\
  ++err;\
}} while (0)
static int
validate_driver_0_0(SkExecUTL* exec, SkDriver driver) {
  int err;
  PFN_skVoidFunction pfnVoidFunction;

  err = 0;
  exec->sectionBegin("Validate Driver Exports (0.0.0)", exec);
  {
    // Required:
    CHECK_NOSUB(skCreateDriver);
    CHECK_NOSUB(skDestroyDriver);
    CHECK(skGetDriverProcAddr);
    CHECK(skGetDriverProperties);
    CHECK(skGetDriverFeatures);
    CHECK(skEnumerateDriverEndpoints);
    CHECK(skEnumerateDriverDevices);
    CHECK(skQueryDeviceProperties);
    CHECK(skQueryDeviceFeatures);
    CHECK(skEnumerateDeviceEndpoints);
    CHECK(skQueryEndpointFeatures);
    CHECK(skQueryEndpointProperties);
    CHECK(skRequestPcmStream);
  }
  exec->sectionEnd("Validate Driver Exports (0.0.0)", exec);

  return err;
}
#undef CHECK
#undef CHECK_NOSUB

#ifdef    OPENSK_ALSA
static int
validate_driver_alsa(SkExecUTL* exec, SkDriver driver) {
  SkResult result;

  // Configure a sample PCM stream request with commonly-supported parameters.
  // This is just to test if the API validly conforms to ICD requests.
  exec->sectionBegin("ALSA PCM", exec);
  {
    SkAlsaPcmStreamRequest request;
    memset(&request, 0, sizeof(SkAlsaPcmStreamRequest));
    request.sType = SK_STRUCTURE_TYPE_ICD_PCM_STREAM_REQUEST;
    request.hw.streamType = SND_PCM_STREAM_PLAYBACK;
    request.hw.accessMode = SND_PCM_ACCESS_RW_INTERLEAVED;
    request.hw.blockingMode = SND_PCM_BLOCK;
    request.hw.formatType = SND_PCM_FORMAT_S16_LE;
    request.hw.channels = 2;
    request.hw.rate = 44100;

    SkEndpoint endpoint;
    endpoint = skResolveObject(driver, "default");
    if (!endpoint || skGetObjectType(endpoint) != SK_OBJECT_TYPE_ENDPOINT) {
      SKERR("No default endpoint in the ALSA driver (not expected)!");
      return 1;
    }

    SkPcmStream stream;
    result = skRequestPcmStream(endpoint, (SkPcmStreamRequest*)&request, &stream);
    if (result != SK_SUCCESS) {
      SKERR("Unable to create a simple default PCM stream.");
      return 1;
    }

    result = skClosePcmStream(stream, SK_FALSE);
    if (result != SK_SUCCESS) {
      SKERR("Unable to close a simple default PCM stream.");
      return 1;
    }
  }
  exec->sectionEnd("ALSA PCM", exec);

  return 0;
}
#endif // OPENSK_ALSA

static int
validate_driver(SkExecUTL* exec, SkDriverCreateInfo* pCreateInfo, SkInstance instance) {
  int err;
  uint32_t idx;
  SkResult result;
  SkDriver driver;
  uint32_t driverCount;
  SkDevice* pDevices;
  SkDriver* pDriver;
  uint32_t deviceCount;
  SkEndpoint* pEndpoints;
  uint32_t endpointCount;
  SkDriver foundDriver;
  SkDriverFeatures features;
  SkDriverProperties properties;

  // Accumulate errors using this variable.
  err = 0;
  driver = SK_NULL_HANDLE;
  memset(&properties, 0, sizeof(SkDriverProperties));

  // Attempt to get the driver via. the resolve function.
  if (check(exec, VS_INSTANCE_RESOLVE_DRIVERS)) {
    exec->sectionBegin("Resolve Object", exec);
    ONCE {
      driver = skResolveObject(instance, pCreateInfo->properties.identifier);
      if (!driver) {
        SKERR("Unable to find the driver '%s' after constructing an instance.\n", pCreateInfo->properties.identifier);
        ++err;
        break;
      }
      if (skGetObjectType(driver) != SK_OBJECT_TYPE_DRIVER) {
        SKERR("Resolved object is not of-type SK_OBJECT_TYPE_DRIVER.\n");
        ++err;
        break;
      }
    }
    exec->sectionEnd("Resolve Object", exec);
  }

  // Attempt to get the driver via. the enumerate function.
  if (check(exec, VS_INSTANCE_ENUMERATE_DRIVERS)) {
    exec->sectionBegin("Enumerate Driver", exec);
    ONCE {
      result = skEnumerateInstanceDrivers(instance, &driverCount, NULL);
      if (result != SK_SUCCESS) {
        SKERR("Failed to enumerate drivers on the provided instance.\n");
        ++err;
        break;
      }
      if (!driverCount) {
        SKERR("Failed to find any drivers within the provided instance.\n");
        ++err;
        break;
      }
      pDriver = skAllocate(
        exec->pSystemAllocator,
        sizeof(SkDriver) * driverCount,
        1,
        SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
      );
      if (!pDriver) {
        SKERR("Failed to allocate enough space for the driver api.\n");
        ++err;
        break;
      }
      result = skEnumerateInstanceDrivers(instance, &driverCount, pDriver);
      if (result < SK_SUCCESS) {
        SKERR("Failed to enumerate drivers on the provided instance.\n");
        skFree(exec->pSystemAllocator, pDriver);
        ++err;
        break;
      }
      foundDriver = SK_NULL_HANDLE;
      for (idx = 0; idx < driverCount; ++idx) {
        skGetDriverProperties(pDriver[idx], &properties);
        if (strcmp(properties.identifier, pCreateInfo->properties.identifier) == 0) {
          foundDriver = pDriver[idx];
          break;
        }
      }
      skFree(exec->pSystemAllocator, pDriver);
      if (!foundDriver) {
        SKERR("Failed to find the constructed driver within the provided instance.\n");
        ++err;
        break;
      }
      else if (driver && foundDriver != driver) {
        SKERR("The driver found via. skResolveObject() is different than the one found via. skEnumerateInstanceDriver() - aborting.\n");
        return err;
      }
      else {
        driver = foundDriver;
      }
    }
    exec->sectionBegin("Enumerate Driver", exec);
  }

  // See if we found the driver via. either of the methods above.
  if (!driver) {
    SKERR("Cannot continue testing the driver - aborting.\n");
    return err;
  }
  if (skGetObjectType(driver) != SK_OBJECT_TYPE_DRIVER) {
    SKERR("The driver's oType is not set to SK_OBJECT_TYPE_DRIVER!\n");
    ++err;
  }
  if (skResolveParent(driver) != instance) {
    SKERR("The driver's parent is not set to the constructed instance!\n");
    ++err;
  }

  // Check the properties of the driver and make sure they match the manifest.
  if (check(exec, VS_DRIVER_PROPERTIES)) {
    exec->sectionBegin("Driver Properties", exec);
    {
      skGetDriverProperties(driver, &properties);
      if (strcmp(properties.identifier, pCreateInfo->properties.identifier)) {
        SKERR("Invalid driver properties (identifier) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (strcmp(properties.driverName, pCreateInfo->properties.driverName)) {
        SKERR("Invalid driver properties (driverName) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (strcmp(properties.displayName, pCreateInfo->properties.displayName)) {
        SKERR("Invalid driver properties (displayName) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (strcmp(properties.description, pCreateInfo->properties.description)) {
        SKERR("Invalid driver properties (description) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (properties.apiVersion != pCreateInfo->properties.apiVersion) {
        SKERR("Invalid driver properties (apiVersion) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (properties.implVersion != pCreateInfo->properties.implVersion) {
        SKERR("Invalid driver properties (implVersion) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (memcmp(properties.driverUuid, pCreateInfo->properties.driverUuid, SK_UUID_SIZE)) {
        SKERR("Invalid driver properties (driverUuid) - mismatch between manifest and logic.\n");
        ++err;
      }
    }
    exec->sectionEnd("Driver Properties", exec);
  }

  // Check the features of the driver and make sure they are sensible.
  pEndpoints = NULL;
  endpointCount = 0;
  if (check(exec, VS_DRIVER_FEATURES)) {
    exec->sectionBegin("Driver Features", exec);
    {
      skGetDriverFeatures(driver, &features);
      if (features.defaultEndpoint != SK_TRUE && features.defaultEndpoint != SK_FALSE) {
        SKERR("Invalid driver features (defaultEndpoint) - please use SK_TRUE or SK_FALSE.\n");
        ++err;
      }
      if (features.supportedAccessModes & ~SK_ACCESS_FLAG_BITS_MASK) {
        SKERR("Invalid driver features (supportedAccessModes) - please use the bit definitions provided.\n");
        ++err;
      }
      if (features.supportedStreams & ~SK_STREAM_FLAG_BITS_MASK) {
        SKERR("Invalid driver features (supportedStreams) - please use the bit definitions provided.\n");
        ++err;
      }
    }
    exec->sectionEnd("Driver Features", exec);
  }

  // Validate the functions present on this driver
  switch (properties.apiVersion) {
    case SK_API_VERSION_0_0:
      err += validate_driver_0_0(exec, driver);
      break;
    default:
      SKERR("Invalid OpenSK API version detected - continuing may be erroneous!\n");
      ++err;
      break;
  }

  // Do driver-specific validation
#ifdef    OPENSK_ALSA
  if (strcmp(properties.driverName, SK_DRIVER_OPENSK_ALSA) == 0) {
    err += validate_driver_alsa(exec, driver);
  }
#endif // OPENSK_ALSA

  // Check the driver to see if you can query the endpoints available.
  if (check(exec, VS_DRIVER_ENUMERATE_ENDPOINTS)) {
    exec->sectionBegin("Driver Endpoints", exec);
    ONCE {
      result = skEnumerateDriverEndpoints(driver, &endpointCount, NULL);
      if (result != SK_SUCCESS) {
        SKERR("Failed to count driver endpoints!\n");
        ++err;
        break;
      }
      pEndpoints = skAllocate(
        exec->pSystemAllocator,
        sizeof(SkEndpoint) * endpointCount,
        1,
        SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
      );
      if (!pEndpoints) {
        SKERR("Failed to allocate enough space for %u endpoints.\n", endpointCount);
        ++err;
        break;
      }
      result = skEnumerateDriverEndpoints(driver, &endpointCount, pEndpoints);
      if (result != SK_SUCCESS) {
        skFree(exec->pSystemAllocator, pEndpoints);
        pEndpoints = NULL;
        SKERR("Failed to enumerate driver endpoints!\n");
        ++err;
        break;
      }
    }
    exec->sectionEnd("Driver Endpoints", exec);
  }

  // Check the driver to see if you can query the devices available.
  pDevices = NULL;
  deviceCount = 0;
  if (check(exec, VS_DRIVER_ENUMERATE_DEVICES)) {
    exec->sectionBegin("Driver Devices", exec);
    ONCE {
      result = skEnumerateDriverDevices(driver, &deviceCount, NULL);
      if (result != SK_SUCCESS) {
        SKERR("Failed to count driver devices!\n");
        ++err;
        break;
      }
      pDevices = skAllocate(
        exec->pSystemAllocator,
        sizeof(SkEndpoint) * deviceCount,
        1,
        SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
      );
      if (!pDevices) {
        SKERR("Failed to allocate enough space for %u devices.\n", deviceCount);
        ++err;
        break;
      }
      result = skEnumerateDriverDevices(driver, &deviceCount, pDevices);
      if (result != SK_SUCCESS) {
        skFree(exec->pSystemAllocator, pDevices);
        pDevices = NULL;
        SKERR("Failed to enumerate driver devices!\n");
        ++err;
        break;
      }
    }
    exec->sectionEnd("Driver Devices", exec);
  }

  // Validate the driver endpoints
  if (check(exec, VS_DRIVER_ENDPOINTS_VALIDATE)) {
    exec->sectionBegin("Driver Endpoint Validate", exec);
    ONCE {
      if (!check(exec, VS_DRIVER_ENUMERATE_ENDPOINTS)) {
        SKWRN("Cannot validate endpoints and not enumerate them - ignoring.\n");
        break;
      }
      if (!pEndpoints) {
        SKWRN("Planned to validate endpoints, but endpoints failed to enumerate.\n");
        break;
      }
      printf("--- Validating %u driver endpoints\n", endpointCount);
      for (idx = 0; idx < endpointCount; ++idx) {
        err += validate_endpoint(exec, &properties, driver, pEndpoints[idx]);
      }
    }
    exec->sectionEnd("Driver Endpoint Validate", exec);
  }

  // Validate the driver devices
  if (check(exec, VS_DRIVER_DEVICES_VALIDATE)) {
    exec->sectionBegin("Driver Device Validate", exec);
    ONCE {
      if (!check(exec, VS_DRIVER_ENUMERATE_DEVICES)) {
        SKWRN("Cannot validate devices and not enumerate them - ignoring.\n");
        break;
      }
      if (!pDevices) {
        SKWRN("Planned to validate devices, but devices failed to enumerate.\n");
        break;
      }
      printf("--- Validating %u driver devices\n", deviceCount);
      for (idx = 0; idx < deviceCount; ++idx) {
        err += validate_device(exec, &properties, driver, pDevices[idx]);
      }
    }
    exec->sectionEnd("Driver Device Validate", exec);
  }

  skFree(exec->pSystemAllocator, pEndpoints);
  skFree(exec->pSystemAllocator, pDevices);
  return err;
}

static int
process_driver(SkExecUTL* exec, SkManifestMAN manifest, SkDriverProperties* pProperties) {
  int err;
  SkResult result;
  SkInstance instance;
  uint32_t memoryIssues;
  SkInternalResult iresult;
  SkInstanceCreateInfo createInfo;
  SkDriverCreateInfo driverCreateInfo;
  SkDebugAllocatorCreateInfoUTL debugAllocatorCreateInfo;

  printf("*** Validating %s (%s)\n", pProperties->displayName, pProperties->driverName);

  // Initialize the driver create-info structure.
  iresult = skInitializeManifestDriverCreateInfoMAN(
    manifest,
    pProperties->driverName,
    &driverCreateInfo
  );
  if (iresult != SKI_SUCCESS) {
    return 1;
  }

  // Create the requested type of allocator
  exec->sectionBegin("Allocator Initialize", exec);
  {
    switch (exec->allocatorType) {
      case AT_DEBUG:
        debugAllocatorCreateInfo.underrunBufferSize = 32;
        debugAllocatorCreateInfo.overrunBufferSize = 32;
        result = skCreateDebugAllocatorUTL(
          &debugAllocatorCreateInfo,
          exec->pSystemAllocator,
          &exec->pUserAllocator
        );
        break;
      default:
        SKWRN("Unknown allocator type - using the default allocator.\n");
        exec->pUserAllocator = NULL;
        result = SK_SUCCESS;
        break;
    }
    if (result != SK_SUCCESS) {
      SKERR("Failed to create the requested allocator - aborting.\n");
      return 1;
    }
  }
  exec->sectionEnd("Allocator Initialize", exec);

  // Initialize the loader with the provided allocator.
  if (check(exec, VS_LOADER_INITIALIZE)) {
    exec->sectionBegin("Loader Initialize", exec);
    {
      result = skSetLoaderAllocator(exec->pUserAllocator);
      if (result != SK_SUCCESS) {
        SKERR("Failed to set the loader's allocator - aborting.\n");
        return 1;
      }
      result = skInitializeLoader();
      if (result != SK_SUCCESS) {
        SKERR("Failed to initialize the loader - aborting.\n");
        return 1;
      }
    }
    exec->sectionEnd("Loader Initialize", exec);
  }

  // At this point, we can start aggregating errors.
  err = 0;

  // Create the instance to use for debugging the driver.
  // NOTE: For debugging purposes, we only create explicit drivers.
  if (check(exec, VS_INSTANCE_CREATE)) {
    exec->sectionBegin("Instance Create", exec);
    {
      memset(&createInfo, 0, sizeof(SkInstanceCreateInfo));
      createInfo.sType = SK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      createInfo.pApplicationInfo = &exec->applicationInfo;
      createInfo.flags = SK_INSTANCE_CREATE_NO_IMPLICIT_OBJECTS_BIT;
      createInfo.pNext = &driverCreateInfo;
      createInfo.enabledLayerCount = sizeof(requiredLayers) / sizeof(requiredLayers[0]);
      createInfo.ppEnabledLayerNames = requiredLayers;
      result = skCreateInstance(&createInfo, exec->pUserAllocator, &instance);
      if (result != SK_SUCCESS) {
        err += 1;
      }
    }
    exec->sectionEnd("Instance Create", exec);

    // Only run the rest of the validation checks if constructed properly.
    if (result == SK_SUCCESS) {
      if (check(exec, VS_INSTANCE_VALIDATE)) {
        exec->sectionBegin("Instance Validate", exec);
        {
          if (skGetObjectType(instance) != SK_OBJECT_TYPE_INSTANCE) {
            SKERR("The instance object's oType is not set to SK_OBJECT_TYPE_INSTANCE!\n");
            ++err;
          }
          if (skResolveParent(instance) != NULL) {
            SKERR("The instance object's pParent field appears to have a parent - it should not!\n");
            ++err;
          }
          err += validate_driver(exec, &driverCreateInfo, instance);
        }
        exec->sectionEnd("Instance Validate", exec);
      }

      // Destroy the instance so that we destruct the driver
      exec->sectionBegin("Instance Destroy", exec);
      {
        skDestroyInstance(instance, exec->pUserAllocator);
      }
      exec->sectionEnd("Instance Destroy", exec);
    }
  }

  // Deinitialize the loader so that it can be used again later.
  exec->sectionBegin("Loader Deinitialize", exec);
  {
    skDeinitializeLoader();
  }
  exec->sectionEnd("Loader Deinitialize", exec);

  // Destroy the allocator that was created.
  exec->sectionBegin("Allocator Destroy", exec);
  {
    switch (exec->allocatorType) {
      case AT_DEBUG:
        memoryIssues = skDestroyDebugAllocatorUTL(
          exec->pUserAllocator,
          exec->pSystemAllocator
        );
        if (memoryIssues) {
          SKERR("Memory Issues detected - please fix them!\n");
          err += memoryIssues;
        }
        break;
      default:
        break;
    }
    exec->pUserAllocator = NULL;
  }
  exec->sectionEnd("Allocator Destroy", exec);

  return err;
}

static int
find_properties(SkLayerCreateInfo const* pCreateInfo, SkLayerProperties* pProperties) {
  int err;
  PFN_skVoidFunction pfnVoidFunction;
  PFN_skGetLayerProperties pfnGetLayerProperties;

  err = 0;
  pfnGetLayerProperties = NULL;
  if (pCreateInfo->pfnGetInstanceProcAddr) {
    pfnVoidFunction = pCreateInfo->pfnGetInstanceProcAddr(NULL, "skGetLayerProperties");
    if (!pfnGetLayerProperties) {
      pfnGetLayerProperties = (PFN_skGetLayerProperties)pfnVoidFunction;
    }
    if (!pfnVoidFunction) {
      SKERR("skGetInstanceProcAddr was defined, but it doesn't return skGetLayerProperties.\n");
      ++err;
    }
  }
  if (pCreateInfo->pfnGetDriverProcAddr) {
    pfnVoidFunction = pCreateInfo->pfnGetDriverProcAddr(NULL, "skGetLayerProperties");
    if (!pfnGetLayerProperties) {
      pfnGetLayerProperties = (PFN_skGetLayerProperties)pfnVoidFunction;
    }
    if (!pfnVoidFunction) {
      SKERR("skGetDriverProcAddr was defined, but it doesn't return skGetLayerProperties.\n");
      ++err;
    }
  }
  if (pCreateInfo->pfnGetPcmStreamProcAddr) {
    pfnVoidFunction = pCreateInfo->pfnGetPcmStreamProcAddr(NULL, "skGetLayerProperties");
    if (!pfnGetLayerProperties) {
      pfnGetLayerProperties = (PFN_skGetLayerProperties)pfnVoidFunction;
    }
    if (!pfnVoidFunction) {
      SKERR("skGetPcmStreamProcAddr was defined, but it doesn't return skGetLayerProperties.\n");
      ++err;
    }
  }

  // Grab the properties for the layer.
  if (pfnGetLayerProperties) {
    pfnGetLayerProperties(pProperties);
    return err;
  }
  return -err;
}

static int
process_layer(SkExecUTL* exec, SkManifestMAN manifest, SkLayerProperties* pProperties) {
  int err;
  SkInternalResult result;
  SkLayerCreateInfo createInfo;
  SkLayerProperties properties;
  SkBool32 supportsSomeLayering;

  printf("*** Validating %s (%s)\n", pProperties->displayName, pProperties->layerName);
  result = skInitializeManifestLayerCreateInfoMAN(
    manifest, pProperties->layerName, &createInfo
  );
  if (result != SKI_SUCCESS) {
    switch (result) {
      case SKI_ERROR_MANIFEST_LAYER_NOT_FOUND:
        SKERR("Failed to find the shared library to load the layer from - aborting.\n");
        break;
      case SKI_ERROR_MANIFEST_INVALID:
        SKERR("One or more of the required functions were not exported in the layer - aborting.\n");
        break;
      default:
        SKERR("An unexpected error occurred when initializing the layer creation info - aborting.\n");
        break;
    }
    return 1;
  }

  // Scan layer properties and make sure manifest information matches.
  exec->sectionBegin("Layer Properties", exec);
  {
    err = find_properties(&createInfo, &properties);
    if (err < 0) {
      err = -err;
      SKERR("Failed to fill the properties structure, skipping manifest metadata verification.\n");
    }
    else {
      if (strcmp(createInfo.properties.layerName, properties.layerName)) {
        SKERR("Invalid layer properties (layerName) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (strcmp(createInfo.properties.displayName, properties.displayName)) {
        SKERR("Invalid layer properties (displayName) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (strcmp(createInfo.properties.description, properties.description)) {
        SKERR("Invalid layer properties (description) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (memcmp(createInfo.properties.layerUuid, properties.layerUuid, SK_UUID_SIZE)) {
        SKERR("Invalid layer properties (layerUuid) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (createInfo.properties.apiVersion != properties.apiVersion) {
        SKERR("Invalid layer properties (apiVersion) - mismatch between manifest and logic.\n");
        ++err;
      }
      if (createInfo.properties.implVersion != properties.implVersion) {
        SKERR("Invalid layer properties (implVersion) - mismatch between manifest and logic.\n");
        ++err;
      }
    }
  }
  exec->sectionEnd("Layer Properties", exec);

  // Validate the different layer types - check that the creation function exists.
  supportsSomeLayering = SK_FALSE;
  exec->sectionBegin("Layer Overlays", exec);
  {
    if (createInfo.pfnGetInstanceProcAddr) {
      if (!createInfo.pfnGetInstanceProcAddr(NULL, "skCreateInstance")) {
        SKERR("Layer claims to have instance proc addresses, but it doesn't create an instance layer.\n");
        ++err;
      }
      else {
        supportsSomeLayering = SK_TRUE;
        printf("--- Layer supports Instance layering.\n");
      }
    }
    if (createInfo.pfnGetDriverProcAddr) {
      if (!createInfo.pfnGetDriverProcAddr(NULL, "skCreateDriver")) {
        SKERR("Layer claims to have driver proc addresses, but it doesn't create an instance layer.\n");
        ++err;
      }
      else {
        supportsSomeLayering = SK_TRUE;
        printf("--- Layer supports Driver layering.\n");
      }
    }
    if (createInfo.pfnGetPcmStreamProcAddr) {
      if (!createInfo.pfnGetPcmStreamProcAddr(NULL, "skCreatePcmStream")) {
        SKERR("Layer claims to have PCM proc addresses, but it doesn't create an instance layer.\n");
        ++err;
      }
      else {
        supportsSomeLayering = SK_TRUE;
        printf("--- Layer supports PCM layering.\n");
      }
    }
    if (!supportsSomeLayering) {
      SKWRN("The layer does not appear to support layering for any subset of the API.\n");
    }
  }
  exec->sectionEnd("Layer Overlays", exec);

  // Validate the functions present on this driver
  switch (properties.apiVersion) {
    case SK_API_VERSION_0_0:
      break;
    default:
      SKERR("Invalid OpenSK API version detected - continuing may be erroneous!\n");
      ++err;
      break;
  }

  return err;
}

static int
process_drivers(SkExecUTL* exec, SkManifestMAN manifest) {
  int err;
  uint32_t idx;
  uint32_t driverCount;
  SkInternalResult result;
  SkDriverProperties* pProperties;

  // Attempt to enumerate the manifest's drivers
  result = skEnumerateManifestDriverPropertiesMAN(
    manifest,
    SK_MANIFEST_ENUMERATE_ALL_BIT,
    &driverCount,
    NULL
  );
  if (result != SKI_SUCCESS) {
    return 1;
  }

  // Construct a temporary buffer for the drivers
  pProperties = skAllocate(
    exec->pSystemAllocator,
    sizeof(SkDriverProperties) * driverCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pProperties) {
    return 1;
  }

  // Enumerate the manifest's drivers for information
  result = skEnumerateManifestDriverPropertiesMAN(
    manifest,
    SK_MANIFEST_ENUMERATE_ALL_BIT,
    &driverCount,
    pProperties
  );
  if (result < SKI_SUCCESS) {
    skFree(exec->pSystemAllocator, pProperties);
    return 1;
  }

  // Process all of the drivers listed within the manifest
  err = 0;
  if (check(exec, VS_MANIFEST_CREATE)) {
    for (idx = 0; idx < driverCount; ++idx) {
      err += process_driver(exec, manifest, &pProperties[idx]);
    }
  }

  skFree(exec->pSystemAllocator, pProperties);
  return err;
}

static int
process_layers(SkExecUTL* exec, SkManifestMAN manifest) {
  int err;
  uint32_t idx;
  uint32_t driverCount;
  SkInternalResult result;
  SkLayerProperties* pProperties;

  // Attempt to enumerate the manifest's drivers
  result = skEnumerateManifestLayerPropertiesMAN(
    manifest,
    SK_MANIFEST_ENUMERATE_ALL_BIT,
    &driverCount,
    NULL
  );
  if (result != SKI_SUCCESS) {
    return 1;
  }

  // Construct a temporary buffer for the drivers
  pProperties = skAllocate(
    exec->pSystemAllocator,
    sizeof(SkLayerProperties) * driverCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pProperties) {
    return 1;
  }

  // Enumerate the manifest's drivers for information
  result = skEnumerateManifestLayerPropertiesMAN(
    manifest,
    SK_MANIFEST_ENUMERATE_ALL_BIT,
    &driverCount,
    pProperties
  );
  if (result < SKI_SUCCESS) {
    skFree(exec->pSystemAllocator, pProperties);
    return 1;
  }

  // Process all of the drivers listed within the manifest
  err = 0;
  if (check(exec, VS_MANIFEST_CREATE)) {
    for (idx = 0; idx < driverCount; ++idx) {
      err += process_layer(exec, manifest, &pProperties[idx]);
    }
  }

  skFree(exec->pSystemAllocator, pProperties);
  return err;
}

#define FORMAT_MANIFEST_VERSION(v) "  \"" v "\"\n"
static int
process_manifest(SkExecUTL* exec, DebugFile* pFile) {
  int err;
  SkManifestMAN manifest;
  SkInternalResult result;
  SkManifestPropertiesMAN properties;
  SkManifestCreateInfoMAN createInfo;

  printf("=== Validating manifest: '%s'\n", pFile->pFilename);

  // Fill the query out for processing.
  memset(&createInfo, 0, sizeof(SkManifestCreateInfoMAN));
  createInfo.pFilepath = pFile->pFilename;

  // Attempt to read the manifest file - respond to the internal error codes.
  exec->sectionBegin("Loading Manifest", exec);
  {
    result = skCreateManifestMAN(
      &createInfo,
      exec->pSystemAllocator,
      &manifest
    );
    switch (result) {
      case SKI_ERROR_JSON_INVALID:
        SKERR("Unable to parse the manifest file provided - invalid JSON!\n");
        return 1;
      case SKI_ERROR_MANIFEST_INVALID:
        SKERR("Manifest file is missing the required '%s' property.\n", SK_MANIFEST_VERSION_PROPERTY_NAME);
        return 1;
      case SKI_ERROR_MANIFEST_UNSUPPORTED:
        SKERR("The manifest version string does not exactly match a supported versions:\n");
        fprintf(
          stderr,
          FORMAT_MANIFEST_VERSION(SK_MANIFEST_VERSION_1_0_0)
          "\n"
        );
        return 1;
      case SKI_SUCCESS:
        // Expected
        break;
      default:
        SKERR("An unexpected error code returned from skQueryManifestMAN() - aborting!\n");
        return 1;
    }
    skGetManifestPropertiesMAN(manifest, &properties);
  }
  exec->sectionEnd("Loading Manifest", exec);

  // At this point, we will be validating the contents of the manifest
  err = 0;

  // See if any of the drivers/layers within the manifest are invalid.
  if (check(exec, VS_MANIFEST_VERIFY)) {
    if (properties.definedDriverCount != properties.validDriverCount) {
      SKWRN("One or more of the drivers defined within the manifest is invalid.\n");
      ++err;
    }
    if (properties.definedLayerCount != properties.validLayerCount) {
      SKWRN("One or more of the layers defined within the manifest is invalid.\n");
      ++err;
    }
  }

  // Check that there is anything to debug
  if (check(exec, VS_MANIFEST_ENUMERATE)) {
    if (properties.validDriverCount == 0 && properties.validLayerCount == 0) {
      skDestroyManifestMAN(manifest, exec->pSystemAllocator);
      SKERR("A manifest was provided and parsed, but no valid modules were found.\n");
      return 1;
    }

    // Debug the provided manifest objects.
    if (properties.validDriverCount) {
      err += process_drivers(exec, manifest);
    }
    if (properties.validLayerCount) {
      err += process_layers(exec, manifest);
    }
  }

  skDestroyManifestMAN(manifest, exec->pSystemAllocator);
  return err;
}
#undef FORMAT_MANIFEST_VERSION

static int
add_file(SkExecUTL* exec, char const* pFilename) {
  ++exec->fileCount;
  exec->files = realloc(exec->files, exec->fileCount);
  if (!exec->files) {
    return -1;
  }
  memset(&exec->files[exec->fileCount - 1], 0, sizeof(DebugFile));
  exec->files[exec->fileCount - 1].pFilename = pFilename;
  return 0;
}

static SkBool32
is_layer_implicit(SkLayerProperties const* pProperties, SkLayerProperties const* pImplicitProperties, uint32_t implicitCount) {
  uint32_t idx;
  for (idx = 0; idx < implicitCount; ++idx) {
    if (strcmp(pProperties->layerName, pImplicitProperties[idx].layerName) == 0) {
      return SK_TRUE;
    }
  }
  return SK_FALSE;
}

static int
list_steps() {
  VerificationStep verification;

  printf("*** List of all verification steps\n");
  verification = (VerificationStep)0;
  while (verification != VS_MAX_COUNT) {
    printf("  %s (%u)\n", verification_string(verification), (unsigned)verification);
    ++verification;
  }

  return 0;
}

static int
list_drivers() {
  uint32_t idx;
  uint32_t count;
  SkResult result;
  SkDriverProperties* pProperties;

  // Count all of the drivers on the system
  result = skEnumerateLoaderDriverProperties(
    SK_LOADER_ENUMERATE_ALL_BIT,
    &count,
    NULL
  );
  if (result != SK_SUCCESS) {
    SKERR("Failed to count all of the driver properties.\n");
    return -1;
  }

  // If there are no drivers whatsoever, print that and exit.
  if (!count) {
    printf("There are no valid drivers installed on this system.\n");
    return 0;
  }

  // Allocate space for the drivers
  pProperties = malloc(sizeof(SkDriverProperties) * count);
  if (!pProperties) {
    SKERR("Failed to allocate space for %u drivers...\n", count);
    return -1;
  }

  // Grab all of the driver properties.
  result = skEnumerateLoaderDriverProperties(
    SK_LOADER_ENUMERATE_ALL_BIT,
    &count,
    pProperties
  );
  if (result < SK_SUCCESS) {
    SKERR("Failed to enumerate all of the driver properties.\n");
    return -1;
  }

  // Print all of the drivers on the system.
  printf("*** List of all installed drivers\n");
  for (idx = 0; idx < count; ++idx) {
    printf("%s: %s\n", pProperties[idx].driverName, pProperties[idx].displayName);
    printf("  %s\n", pProperties[idx].description);
    printf("  ");
    skPrintUuidUTL(pProperties[idx].driverUuid);
  }
  printf("\n");

  return 0;
}

static int
list_layers() {
  uint32_t idx;
  SkResult result;
  SkBool32 isImplicit;
  uint32_t totalCount;
  uint32_t implicitCount;
  SkLayerProperties* pProperties;
  SkLayerProperties* pImplicitProperties;

  // Count all of the layers regardless of implicitness
  result = skEnumerateLoaderLayerProperties(
    SK_LOADER_ENUMERATE_ALL_BIT,
    &totalCount,
    NULL
  );
  if (result != SK_SUCCESS) {
    SKERR("Failed to count all of the layer properties.\n");
    return -1;
  }

  // If there are no layers whatsoever, print that and exit.
  if (!totalCount) {
    printf("There are no valid layers installed on this system.\n");
    return 0;
  }

  // Count all of the implicit layers specifically
  result = skEnumerateLoaderLayerProperties(
    SK_LOADER_ENUMERATE_IMPLICIT_BIT,
    &implicitCount,
    NULL
  );
  if (result != SK_SUCCESS) {
    SKERR("Failed to count the implicit layer properties.\n");
    return -1;
  }

  // Allocate space for the layers
  pProperties = malloc(sizeof(SkLayerProperties) * (totalCount + implicitCount));
  if (!pProperties) {
    SKERR("Failed to allocate space for %u layers...\n", totalCount);
    return -1;
  }
  pImplicitProperties = &pProperties[totalCount];

  // Grab all of the layers regardless of implicitness
  result = skEnumerateLoaderLayerProperties(SK_FALSE, &totalCount, pProperties);
  if (result < SK_SUCCESS) {
    SKERR("Failed to enumerate all of the layer properties.\n");
    return -1;
  }

  // Grab all of the implicit layers specifically
  result = skEnumerateLoaderLayerProperties(SK_TRUE, &implicitCount, pImplicitProperties);
  if (result < SK_SUCCESS) {
    SKERR("Failed to enumerate the implicit layer properties.\n");
    return -1;
  }

  // Print all of the layers on the system
  printf("*** List of all installed layers\n");
  for (idx = 0; idx < totalCount; ++idx) {
    isImplicit = is_layer_implicit(&pProperties[idx], pImplicitProperties, implicitCount);
    printf("%s: %s", pProperties[idx].layerName, pProperties[idx].displayName);
    if (isImplicit) {
      printf(" [IMPLICIT]");
    }
    printf("\n  %s\n", pProperties[idx].description);
    printf("  ");
    skPrintUuidUTL(pProperties[idx].layerUuid);
  }
  printf("\n");

  return 0;
}

int
main(int argc, char const* argv[]) {
  int err;
  FILE* file;
  uint32_t idx;
  SkExecUTL exec;
  SkResult result;
  unsigned uValue;
  char const* param;
  VerificationStep vs;
  SkBool32 customVerify;
  DebugFile* pDebugFile;
  SkDebugAllocatorCreateInfoUTL allocatorCreateInfo;

  // Initialize execution steps
  memset(&exec, 0, sizeof(SkExecUTL));
  for (idx = 0; idx < VS_MAX_COUNT; ++idx) {
    exec.verify[idx] = SK_TRUE;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  customVerify = SK_FALSE;
  for (idx = 1; idx < (uint32_t)argc; ++idx) {
    param = argv[idx];
    if (skCheckParamUTL(param, "-h", "--help")) {
      print_usage();
      return 0;
    }
    else if (skCheckParamUTL(param, "-s", "--steps")) {
      param = argv[++idx];

      // Check to see if we should disable any verification
      if (!customVerify) {
        for (idx = 0; idx < VS_MAX_COUNT; ++idx) {
          exec.verify[idx] = SK_FALSE;
        }
      }

      // First, check if we are enabling steps by number
      if (sscanf(param, "%u", &uValue)) {
        if (uValue >= VS_MAX_COUNT) {
          SKERR("Unable to find the verification step '%u'\n", uValue);
          return -1;
        }
        customVerify = SK_TRUE;
        exec.verify[uValue] = SK_TRUE;
      }

      // Otherwise, check if we are enabling a specific step
      else {
        for (vs = (VerificationStep)0; vs != VS_MAX_COUNT; ++vs) {
          if (strcmp(param, verification_string(vs)) == 0) {
            break;
          }
        }
        if (vs >= VS_MAX_COUNT) {
          SKERR("Unable to find the verification step '%s'\n", param);
          return -1;
        }
        customVerify = SK_TRUE;
        exec.verify[vs] = SK_TRUE;
      }

    }
    else if (skCheckParamUTL(param, "-i", "--ignore")) {
      param = argv[++idx];

      // First, check if we are enabling steps by number
      if (sscanf(param, "%u", &uValue)) {
        if (uValue >= VS_MAX_COUNT) {
          SKERR("Unable to find the verification step '%u'\n", uValue);
          return -1;
        }
        customVerify = SK_TRUE;
        exec.verify[uValue] = SK_FALSE;
      }

      // Otherwise, check if we are enabling a specific step
      else {
        for (vs = (VerificationStep)0; vs != VS_MAX_COUNT; ++vs) {
          if (strcmp(param, verification_string(vs)) == 0) {
            break;
          }
        }
        if (vs >= VS_MAX_COUNT) {
          SKERR("Unable to find the verification step '%s'\n", param);
          return -1;
        }
        customVerify = SK_TRUE;
        exec.verify[vs] = SK_FALSE;
      }

    }
    else if (skCheckParamUTL(param, "-S", "--list-steps")) {
      return list_steps();
    }
    else if (skCheckParamUTL(param, "-v", "--verbose=low", "--verbose")) {
      exec.verbose = 1;
    }
    else if (skCheckParamUTL(param, "-V", "--verbose=high")) {
      exec.verbose = 2;
    }
    else if (skCheckParamUTL(param, "-l", "--list-layers")) {
      return list_layers();
    }
    else if (skCheckParamUTL(param, "-d", "--list-drivers")) {
      return list_drivers();
    }
    else if (param[0] == '-') {
      SKERR("Invalid flag provided to skdbg: %s\n", param);
      return -1;
    }
    else {
      // Check if the file exists and can be read.
      file = fopen(param, "r");
      if (!file) {
        SKERR("The file '%s' was not found or could not be opened.\n", param);
        return -1;
      }
      fclose(file);

      // Add the file to the execution list, fail if allocation error.
      if (add_file(&exec, param)) {
        SKERR("Unable to allocate enough memory to store another file.\n");
        return -1;
      }
    }
  }

  // Make sure that at least one file was provided for debugging.
  if (!exec.fileCount) {
    SKERR("Either a manifest or module file must be provided.\n");
    print_usage();
    return -1;
  }

  // Discover the file types
  for (idx = 0; idx < exec.fileCount; ++idx) {
    pDebugFile = &exec.files[idx];
    if (skCStrCompareEndsUTL(pDebugFile->pFilename, ".json")) {
      pDebugFile->fileType = FT_MANIFEST;
    }
    else {
      SKERR("Unknown or unsupported file extension: %s\n", pDebugFile->pFilename);
      return -1;
    }
  }

  // If there was no custom verification, we should turn everything on
  if (!customVerify) {
    for (vs = (VerificationStep)0; vs != VS_MAX_COUNT; ++vs) {
      exec.verify[vs] = SK_TRUE;
    }
  }

  // Configure the system allocator
  allocatorCreateInfo.underrunBufferSize = 32;
  allocatorCreateInfo.overrunBufferSize = 32;
  result = skCreateDebugAllocatorUTL(
    &allocatorCreateInfo,
    NULL,
    (SkDebugAllocatorUTL*)&exec.pSystemAllocator
  );
  if (result != SK_SUCCESS) {
    SKERR("Failed to configure the system allocator!");
    return -1;
  }

  // Configure the application information for skdbg
  exec.applicationInfo.sType = SK_STRUCTURE_TYPE_APPLICATION_INFO;
  exec.applicationInfo.apiVersion = SK_API_VERSION_0_0;
  exec.applicationInfo.engineVersion = SK_API_VERSION_0_0;
  exec.applicationInfo.applicationVersion = SK_API_VERSION_0_0;
  exec.applicationInfo.pApplicationName = "skdbg";
  exec.applicationInfo.pEngineName = "OpenSK";
  exec.sectionBegin = &begin_section;
  exec.sectionEnd = &end_section;

  //////////////////////////////////////////////////////////////////////////////
  // Process Files
  //////////////////////////////////////////////////////////////////////////////
  err = 0;
  for (idx = 0; idx < exec.fileCount; ++idx) {
    pDebugFile = &exec.files[idx];
    switch (pDebugFile->fileType) {
      case FT_MANIFEST:
        err += process_manifest(&exec, pDebugFile);
        break;
    }
  }

  // Destroy the system allocator
  err += skDestroyDebugAllocatorUTL(
    (SkDebugAllocatorUTL)exec.pSystemAllocator,
    NULL
  );

  // Display the final results
  if (err) {
    SKERR("There appears to be some errors with the tested binary, please fix them and try again.\n");
  }
  else {
    printf("Done - No errors or invalid information detected!\n");
  }

  return err;
}