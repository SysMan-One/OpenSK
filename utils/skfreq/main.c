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
 * A simple application that plays the frequencies passed into it.
 ******************************************************************************/
#define _USE_MATH_DEFINES

// External Dependencies
#include <OpenSK/opensk.h>
#include <OpenSK/dev/utils.h>
#include <stdio.h>  // fprintf
#include <stdlib.h> // malloc
#include <string.h> // strcmp
#include <math.h>   // sin

// Utility Dependencies
#include <OpenSK/utl/error.h>   // skCheck<Function>UTL();
#include <OpenSK/utl/string.h>  // skCheckParamsUTL();
#include <OpenSK/utl/macros.h>  // STR()
#include "freq.inc"

// Defaults and constants
#define SKFREQ_INTENSITY_SCALAR 0.1
#define SKFREQ_DEFAULT_DURATION 1.0
#define SKFREQ_DEFAULT_INTENSITY 1.0
#define SKFREQ_DEFAULT_DUTYCYCL 0.5
#define SKFREQ_DEFAULT_WAVEFORM sine
#define SKFREQ_DEFAULT_ENDPOINT NULL

/*******************************************************************************
 * Wave definitions
 ******************************************************************************/
static float
sine(float t, float d) {
  (void)d;
  return (float)sin(2.0f * M_PI * t);
}

static float
square(float t, float d) {
  t -= ((int)t);
  return (t <= d) ? 1.0f : -1.0f;
}

static float
triangle(float t, float d) {
  t -= ((int)t);
  if (d == 0 && t == 0) {
    return 1.0f;
  }
  if (d == 1 && t == 1) {
    return 0.0f;
  }
  if (t <= d) {
    return -1.0f + t * (2.0f / d);
  }
  else {
    return 1.0f - (t - d) * (2.0f / (1.0f - d));
  }
}

static float
saw(float t, float d) {
  (void)d;
  return triangle(t, 1.0);
}

/*******************************************************************************
 * User settings/properties and defaults
 ******************************************************************************/
static float duration               = (float)(SKFREQ_DEFAULT_DURATION);
static float intensity              = (float)(SKFREQ_DEFAULT_INTENSITY);
static float dutyCycle              = (float)(SKFREQ_DEFAULT_DUTYCYCL);
static float (*wave)(float, float)  = &SKFREQ_DEFAULT_WAVEFORM;
static char const* skPath           = SKFREQ_DEFAULT_ENDPOINT;
static uint32_t frequencyCount      = 0;
static float *frequencies           = NULL;
static float *currFrequency         = NULL;

/*******************************************************************************
 * Main Entry Point
 ******************************************************************************/
int
main(int argc, char const *argv[]) {
  uint32_t idx, ddx;

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  char const *param;
  for (idx = 1; idx < (uint32_t)argc; ++idx) {
    param = argv[idx];
    if (skCheckParamUTL(param, "-d", "--duration")) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a duration after '%s' in parameter list!", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &duration) != 1) {
        SKERR("Failed to parse duration '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (skCheckParamUTL(param, "-D", "--duty-cycle")) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a value after '%s' in parameter list!", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &dutyCycle) != 1) {
        SKERR("Failed to parse duty cycle '%s'!\n", argv[idx]);
        return -1;
      }
      if (dutyCycle > 1.0f || dutyCycle < 0.0f) {
        SKERR("Invalid duty cycle value passed-in '%s'!", argv[idx]);
        return -1;
      }
    }
    else if (skCheckParamUTL(param, "-i", "--intensity")) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a frequency after '%s' in parameter list!", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &intensity) != 1) {
        SKERR("Failed to parse intensity '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (skCheckParamUTL(param, "-w", "--wave")) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a frequency after '%s' in parameter list!", param);
        return -1;
      }
      param = argv[idx];
      if (strcmp(param, "sine") == 0) {
        wave = &sine;
      }
      else if (strcmp(param, "square") == 0) {
        wave = &square;
      }
      else if (strcmp(param, "triangle") == 0) {
        wave = &triangle;
      }
      else if (strcmp(param, "saw") == 0) {
        wave = &saw;
      }
      else {
        SKERR("Unknown wave type '%s'!\n", param);
        return -1;
      }
    }
    else if (skCheckParamUTL(param, "-p", "--path", NULL)) {
      ++idx;
      if (idx >= (uint32_t)argc) {
        SKERR("Expected a canonical stream path after '%s' in parameter list!", param);
        return -1;
      }
      if (skPath) {
        SKERR("Multiple stream paths provided, please only provide one stream path!");
        return -1;
      }
      skPath = argv[idx];
    }
    else if (skCheckParamUTL(param, "-h", "--help", NULL)) {
      printf(
        "Usage: skfreq [options] [frequencies...]\n"
        "\n"
        "Plays a provided set of frequencies on the target OpenSK endpoint.\n"
        "  This utility exists to provide a simple example and test case.\n"
        "\n"
        "Options:\n"
        "  -h, --help       Prints this help documentation.\n"
        "  -p, --playback   The canonical SK path leading to an endpoint.\n"
        "                   e.g. sk://<host>/<endpoint>\n"
        "  -d, --duration   Parses the next argument as a float in seconds, for note duration.\n"
        "                   (Default: " SKSTR(SKFREQ_DEFAULT_DURATION) ")\n"
        "  -w, --wave       Parses the next argument, uses that as the wave type.\n"
        "                   (Default: " SKSTR(SKFREQ_DEFAULT_WAVEFORM) ") (Options: sine, square, triangle, saw)\n"
        "  -D, --duty-cycle Sets the duty cycle for the waveform. (Only: square, triangle)\n"
        "                   (Default: " SKSTR(SKFREQ_DEFAULT_DUTYCYCL) ")\n"
        "  -i, --intensity  Parses the next argument as a float, for note intensity.\n"
        "                   (Default: " SKSTR(SKFREQ_DEFAULT_INTENSITY) ")\n"
        "\n"
        "OpenSK is copyright Trent Reed 2016 - All rights reserved.\n"
        "Full documentation can be found online at <http://www.opensk.org/>.\n"
      );
      return 0;
    }
    else {
      float frequency;
      if (sscanf(param, "%f", &frequency) == 1) {
        // Note set by numeric frequency.
      }
#define NOTE_DEFINE(name, octave, freq) else if (strcmp(name #octave, param) == 0) frequency = freq;
#include "notes.inc"
#undef  NOTE_DEFINE
      else {
        SKERR("Invalid or unsupported argument '%s'! (See --help)", param);
        return -1;
      }
      ++frequencyCount;

      // Note: Later-on, we will use double the space for additive calculations.
      frequencies = (float*)realloc(frequencies, 2 * sizeof(float) * frequencyCount);
      frequencies[frequencyCount - 1] = frequency;
    }
  }

  // Check that at least one frequency was provided.
  if (frequencyCount == 0) {
    SKERR("skfreq requires at least one frequency to play.");
    return -1;
  }

  // Check that if a canonical stream path is named
  // TODO: When sk://sys/default is supported, default to that instead.
  if (!skPath) {
    SKERR("A canonical stream path must be provided to play a frequency.");
    free(frequencies);
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance.
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  memset(&applicationInfo, 0, sizeof(SkApplicationInfo));
  applicationInfo.sType = SK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pApplicationName = "skfreq";
  applicationInfo.pEngineName = "opensk";
  applicationInfo.applicationVersion = SK_API_VERSION_0_0;
  applicationInfo.engineVersion = SK_API_VERSION_0_0;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  memset(&instanceInfo, 0, sizeof(SkInstanceCreateInfo));
  instanceInfo.sType = SK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceInfo.pApplicationInfo = &applicationInfo;

  // Create the OpenSK instance
  SkResult result;
  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (skCheckCreateInstanceUTL(result)) {
    free(frequencies);
    return result;
  }

  // Get the endpoint requested by the user
  SkEndpoint endpoint;
  endpoint = skResolveObject(instance, skPath);
  if (skCheckResolveObjectUTL(endpoint, SK_OBJECT_TYPE_ENDPOINT, skPath)) {
    skDestroyInstance(instance, NULL);
    free(frequencies);
    return SK_ERROR_INVALID;
  }

  // Calculate the machine endianness for floating-point types.
  SkPcmFormat machineFloat;
  if (skIsFloatBigEndian()) {
    machineFloat = SK_PCM_FORMAT_F32_BE;
  }
  else {
    machineFloat = SK_PCM_FORMAT_F32_LE;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Construct a default output stream - uses the best available stream.
  //////////////////////////////////////////////////////////////////////////////
  // TODO: We should add support for the conversion-layer on top of this.
  SkPcmStreamRequest request;
  memset(&request, 0, sizeof(SkPcmStreamRequest));
  request.sType         = SK_STRUCTURE_TYPE_PCM_STREAM_REQUEST;
  request.streamType    = SK_STREAM_PCM_WRITE_BIT;
  request.accessFlags   = SK_ACCESS_BLOCKING
                        | SK_ACCESS_INTERLEAVED;
  request.formatType    = machineFloat;
  request.channels      = 2;
  request.sampleRate    = 44100;
  request.bufferSamples = 4096;

  // Request the PCM stream for writing the waveform to
  SkPcmStream playbackStream;
  SkPcmStreamInfo info;
  result = skRequestPcmStream(endpoint, &request, &playbackStream);
  if (skCheckRequestPcmStreamUTL(result)) {
    skDestroyInstance(instance, NULL);
    free(frequencies);
    return result;
  }
  info.sType = SK_STRUCTURE_TYPE_PCM_STREAM_INFO;
  result = skGetPcmStreamInfo(playbackStream, SK_STREAM_PCM_WRITE_BIT, &info);
  if (skCheckGetPcmStreamInfoUTL(result)) {
    skDestroyInstance(instance, NULL);
    free(frequencies);
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Calculate Waveform
  //////////////////////////////////////////////////////////////////////////////
  // Calculate the total number of samples to process.
  uint32_t totalSamples;
  totalSamples = (uint32_t)(duration * info.sampleRate);
  if (!totalSamples) {
    SKERR("The total samples to process came out to zero, is the input correct?");
    skDestroyInstance(instance, NULL);
    free(frequencies);
    return -1;
  }

  // Allocate the buffer for the the entire waveform
  // Note: This is done instead of a ring buffer for debugging/simplicity.
  void* sampleData;
  sampleData = malloc(totalSamples * (info.frameBits / 8));
  if (!sampleData) {
    SKERR("Failed to allocate the waveform buffer.");
    skDestroyInstance(instance, NULL);
    free(frequencies);
    return -1;
  }

  // To preserve note quality, we process the waveforms additively.
  currFrequency = &frequencies[frequencyCount];
  for (idx = 0; idx < frequencyCount; ++idx) {
    frequencies[idx] /= info.sampleRate;
    currFrequency[idx] = 0.0;
  }

  // Generate the output waveform
  float calculatedSample;
  unsigned char* currSample = sampleData;
  for (idx = 0; idx < totalSamples; ++idx) {

    // Sum all of the frequencies up...
    calculatedSample = 0;
    for (ddx = 0; ddx < frequencyCount; ++ddx) {
      currFrequency[ddx] += frequencies[ddx];
      while (currFrequency[ddx] >= 1.0f) currFrequency[ddx] -= 1.0f;
      calculatedSample += wave(currFrequency[ddx], dutyCycle);
    }
    calculatedSample *= intensity * SKFREQ_INTENSITY_SCALAR;

    // Apply each resultant frequency to each channel...
    for (ddx = 0; ddx < info.channels; ++ddx) {
      *(float*)currSample = calculatedSample;
      currSample += info.sampleBits / 8;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Write data
  //////////////////////////////////////////////////////////////////////////////
  // Attempt to play the samples.
  int64_t samplesPlayed;
  samplesPlayed = skWritePcmStreamInterleaved(playbackStream, sampleData, totalSamples);
  if (skCheckWritePcmStreamUTL(samplesPlayed, totalSamples)) {
    free(sampleData);
    free(frequencies);
    skDestroyInstance(instance, NULL);
    return -1;
  }

  // We specifically close the playback stream so that it can finish playing.
  result = skClosePcmStream(playbackStream, SK_TRUE);
  if (skCheckClosePcmStreamUTL(result)) {
    free(sampleData);
    free(frequencies);
    skDestroyInstance(instance, NULL);
    return -1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Finalize and Destroy
  //////////////////////////////////////////////////////////////////////////////
  skDestroyInstance(instance, NULL);
  free(sampleData);
  free(frequencies);

  return 0;
}
