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
 * A simple application that captures and plays samples from endpoints.
 ******************************************************************************/

// External Dependencies
#include <OpenSK/opensk.h>
#include <stdio.h>  // malloc
#include <stdlib.h>
#include <string.h>

// Utility Dependencies
#include <OpenSK/utl/error.h>
#include <OpenSK/utl/string.h>
#include <OpenSK/utl/macros.h>

// Defaults and constants
#define SKECHO_DEFAULT_DURATION 5.0
#define SKECHO_DEFAULT_ENDPOINT NULL

/*******************************************************************************
 * User settings/properties and defaults
 ******************************************************************************/
static float duration               = (float)(SKECHO_DEFAULT_DURATION);
static char const* skCapturePath    = SKECHO_DEFAULT_ENDPOINT;
static char const* skPlaybackPath   = SKECHO_DEFAULT_ENDPOINT;

/*******************************************************************************
 * Main Entry Point
 ******************************************************************************/
int
main(int argc, char const* argv[]) {
  uint32_t idx;

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  char const *param;
  for (idx = 1; idx < (uint32_t)argc; ++idx) {
    param = argv[idx];
    if (skCheckParamUTL(param, "-d", "--duration")) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a duration after '%s' in parameter list!\n", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &duration) != 1) {
        SKERR("Failed to parse duration '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (skCheckParamUTL(param, "-c", "--capture", NULL)) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a canonical stream path after '%s' in parameter list!\n", param);
        return -1;
      }
      if (skCapturePath) {
        SKERR("Multiple capture stream paths provided, please only provide one stream path!\n");
        return -1;
      }
      skCapturePath = argv[idx];
    }
    else if (skCheckParamUTL(param, "-p", "--playback", NULL)) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a canonical stream path after '%s' in parameter list!\n", param);
        return -1;
      }
      if (skPlaybackPath) {
        SKERR("Multiple playback stream paths provided, please only provide one stream path!\n");
        return -1;
      }
      skPlaybackPath = argv[idx];
    }
    else if (skCheckParamUTL(param, "-h", "--help", NULL)) {
      printf(
        "Usage: skecho [options]\n"
        "\n"
        "Records from one device path, and then plays the recording back.\n"
        "  This utility exists to provide a simple example and test case.\n"
        "\n"
        "Options:\n"
        "  -h, --help       Prints this help documentation.\n"
        "  -c, --capture    The canonical SK path leading to an endpoint for capture.\n"
        "  -p, --playback   The canonical SK path leading to an endpoint for playback.\n"
        "                   e.g. sk://<host>/<endpoint>\n"
        "  -d, --duration   Parses the next argument as a float in seconds, for note duration.\n"
        "                   (Default: " SKSTR(SKECHO_DEFAULT_DURATION) ")\n"
        "\n"
        "OpenSK is copyright Trent Reed 2016 - All rights reserved.\n"
        "Full documentation can be found online at <http://www.opensk.org/>.\n"
      );
      return 0;
    }
    else {
      SKERR("Invalid or unsupported argument '%s'! (See --help)\n", param);
      return -1;
    }
  }

  // Check that if a canonical stream path is named
  // TODO: When sk://sys/default is supported, default to that instead.
  if (!skCapturePath) {
    SKERR("A canonical stream path must be provided to capture data.");
    return -1;
  }
  if (!skPlaybackPath) {
    SKERR("A canonical stream path must be provided to playback data.");
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance.
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  memset(&applicationInfo, 0, sizeof(SkApplicationInfo));
  applicationInfo.sType = SK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;
  applicationInfo.applicationVersion = SK_API_VERSION_0_0;
  applicationInfo.engineVersion = SK_API_VERSION_0_0;
  applicationInfo.pApplicationName = "skecho";
  applicationInfo.pEngineName = "OpenSK";

  SkInstanceCreateInfo createInfo;
  memset(&createInfo, 0, sizeof(SkInstanceCreateInfo));
  createInfo.sType = SK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &applicationInfo;

  // Create the OpenSK instance
  SkResult result;
  SkInstance instance;
  result = skCreateInstance(&createInfo, NULL, &instance);
  if (skCheckCreateInstanceUTL(result)) {
    return result;
  }

  // Get the endpoints requested by the user
  SkEndpoint captureEndpoint;
  captureEndpoint = skResolveObject(instance, skCapturePath);
  if (skCheckResolveObjectUTL(captureEndpoint, SK_OBJECT_TYPE_ENDPOINT, skCapturePath)) {
    skDestroyInstance(instance, NULL);
    return SK_ERROR_INVALID;
  }
  SkEndpoint playbackEndpoint;
  playbackEndpoint = skResolveObject(instance, skPlaybackPath);
  if (skCheckResolveObjectUTL(playbackEndpoint, SK_OBJECT_TYPE_ENDPOINT, skPlaybackPath)) {
    skDestroyInstance(instance, NULL);
    return SK_ERROR_INVALID;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Construct a default input/output stream - uses the best available stream.
  //////////////////////////////////////////////////////////////////////////////
  SkPcmStreamRequest captureRequest;
  memset(&captureRequest, 0, sizeof(SkPcmStreamRequest));
  captureRequest.sType          = SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST;
  captureRequest.streamType     = SK_STREAM_PCM_READ_BIT;
  captureRequest.accessFlags    = SK_ACCESS_BLOCKING
                                | SK_ACCESS_INTERLEAVED;
  captureRequest.formatType     = SK_PCM_FORMAT_S16_LE;
  captureRequest.channels       = 2;
  captureRequest.sampleRate     = 44100;
  captureRequest.bufferSamples  = 4096;

  // Request the PCM stream for reading the waveform from
  SkPcmStream captureStream;
  result = skRequestPcmStream(captureEndpoint, &captureRequest, &captureStream);
  if (skCheckRequestPcmStreamUTL(result)) {
    skDestroyInstance(instance, NULL);
    return result;
  }
  SkPcmStreamInfo captureInfo;
  captureInfo.sType = SK_STRUCTURE_TYPE_PCM_STREAM_INFO;
  result = skGetPcmStreamInfo(captureStream, SK_STREAM_PCM_READ_BIT, &captureInfo);
  if (skCheckGetPcmStreamInfoUTL(result)) {
    skDestroyInstance(instance, NULL);
    return -1;
  }

  SkPcmStreamRequest playbackRequest;
  memset(&playbackRequest, 0, sizeof(SkPcmStreamRequest));
  playbackRequest.sType         = SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST;
  playbackRequest.streamType    = SK_STREAM_PCM_WRITE_BIT;
  playbackRequest.accessFlags   = SK_ACCESS_BLOCKING
                                | SK_ACCESS_INTERLEAVED;
  playbackRequest.formatType    = SK_PCM_FORMAT_S16_LE;
  playbackRequest.channels      = 2;
  playbackRequest.sampleRate    = 44100;
  playbackRequest.bufferSamples = 4096;

  // Request the PCM stream for writing the waveform to
  SkPcmStream playbackStream;
  result = skRequestPcmStream(playbackEndpoint, &playbackRequest, &playbackStream);
  if (skCheckRequestPcmStreamUTL(result)) {
    skDestroyInstance(instance, NULL);
    return result;
  }
  SkPcmStreamInfo playbackInfo;
  playbackInfo.sType = SK_STRUCTURE_TYPE_PCM_STREAM_INFO;
  result = skGetPcmStreamInfo(playbackStream, SK_STREAM_PCM_WRITE_BIT, &playbackInfo);
  if (skCheckGetPcmStreamInfoUTL(result)) {
    skDestroyInstance(instance, NULL);
    return -1;
  }

  // Compare the stream infos and make sure they are compatible.
  // TODO: We should add support for the conversion-layer on top of this.
  if (skCheckPcmStreamInfoCompatibilityUTL(&captureInfo, &playbackInfo)) {
    skDestroyInstance(instance, NULL);
    return result;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Read and write data
  //////////////////////////////////////////////////////////////////////////////
  // Calculate the total number of samples to process.
  uint32_t totalSamples;
  totalSamples = (uint32_t)(duration * captureInfo.sampleRate);
  if (!totalSamples) {
    SKERR("The total samples to process came out to zero, is the input correct?");
    skDestroyInstance(instance, NULL);
    return -1;
  }

  // Allocate enough data to hold all samples.
  // Note: This is done instead of a ring buffer for debugging/simplicity.
  void* sampleData;
  sampleData = malloc(totalSamples * (captureInfo.frameBits / 8));
  if (!sampleData) {
    SKERR("Failed to allocate enough space for capturing the requested data.");
    skDestroyInstance(instance, NULL);
    return -1;
  }

  // Read all of the sample data requested by the user.
  // Note: We configure the stream blocking, so all samples should be read.
  int64_t samplesCaptured;
  samplesCaptured = skReadPcmStreamInterleaved(captureStream, sampleData, totalSamples);
  if (skCheckWritePcmStreamUTL(samplesCaptured, totalSamples)) {
    skDestroyInstance(instance, NULL);
    free(sampleData);
    return -1;
  }

  // Play all of the samples back into the playback stream.
  // Note: We configure the stream blocking, so all samples should be played.
  int64_t samplesPlayback;
  samplesPlayback = skWritePcmStreamInterleaved(playbackStream, sampleData, totalSamples);
  if (skCheckWritePcmStreamUTL(samplesPlayback, totalSamples)) {
    skDestroyInstance(instance, NULL);
    free(sampleData);
    return -1;
  }

  // We specifically close the playback stream so that it can finish playing.
  result = skClosePcmStream(playbackStream, SK_TRUE);
  if (skCheckClosePcmStreamUTL(result)) {
    skDestroyInstance(instance, NULL);
    free(sampleData);
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Finalize and Destroy
  //////////////////////////////////////////////////////////////////////////////
  // Note: When calling skDestroyInstance, all SkObjects are destroyed.
  skDestroyInstance(instance, NULL);
  free(sampleData);

  return 0;
}
