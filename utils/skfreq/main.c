/*******************************************************************************
 * skfreq - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A simple application that plays the frequencies passed into it.
 ******************************************************************************/

#include <OpenSK/opensk.h>
#include <common/common.h>
#include <stdlib.h> // malloc
#include <stdio.h>  // fprintf
#include <stdarg.h> // va_arg
#include <string.h> // strcmp
#include <math.h>   // sin
#include "freq.inc"

/*******************************************************************************
 * Wave definitions
 ******************************************************************************/
static float
sine(float t) {
  return (float)sin(2.0f * M_PI * t);
}

static float
square(float t) {
  t -= ((int)t);
  return (t <= 0.5f) ? 1.0f : -1.0f;
}

static float
triangle(float t) {
  t -= ((int)t);
  return (t <= 0.5f) ? (-1.0f + t * 4.0f) : (3.0f - t * 4.0f);
}

static float
saw(float t) {
  return t - ((int)t);
}

static float
sea(float t) {
  t -= ((int)t);
  return (t <= 0.5f) ? (-1.0f + 12.0f * t * t) : (1.0f - 12.0f * (t - 0.5f) * (t - 0.5f));
}

static float
gull(float t) {
  t -= ((int)t);
  return (t <= 0.5f) ? (1.0f - 12.0f * t * t) : (-1.0f + 12.0f * (t - 0.5f) * (t - 0.5f));
}

/*******************************************************************************
 * User settings/properties and defaults
 ******************************************************************************/
static float duration = 1.0f;
static float intensity = 0.075f;
static int frequencyCount = 0;
static float *frequencies = NULL;
static float (*wave)(float) = &sine;
static char const* physicalDeviceName = NULL;
static char const* virtualDeviceName = NULL;
static char const* componentName = NULL;

// Non-user settings:
static SkPhysicalComponent physicalComponent = SK_NULL_HANDLE;
static SkVirtualComponent virtualComponent = SK_NULL_HANDLE;

/*******************************************************************************
 * Main Entry Point
 ******************************************************************************/
int
main(int argc, char const *argv[]) {

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (int idx = 1; idx < argc; ++idx) {
    char const *param = argv[idx];
    if (genCheckOptions(param, "-d", "--duration", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a duration after '%s' in parameter list!\n", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &duration) != 1) {
        fprintf(stderr, "Failed to parse duration '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (genCheckOptions(param, "-i", "--intensity")) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a frequency after '%s' in parameter list!\n", param);
        return -1;
      }
      if (sscanf(argv[idx], "%f", &intensity) != 1) {
        fprintf(stderr, "Failed to parse intensity '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (genCheckOptions(param, "-w", "--wave", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a frequency after '%s' in parameter list!\n", param);
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
      else if (strcmp(param, "sea") == 0) {
        wave = &sea;
      }
      else if (strcmp(param, "gull") == 0) {
        wave = &gull;
      }
      else {
        fprintf(stderr, "Unknown wave type '%s'!\n", param);
        return -1;
      }
    }
    else if (genCheckOptions(param, "-p", "--physical-device", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a physical device number (or name) after '%s' in parameter list!\n", param);
        return -1;
      }
      if (physicalDeviceName) {
        fprintf(stderr, "Multiple physical devices provided, please only provide one device!\n");
        return -1;
      }
      physicalDeviceName = argv[idx];
    }
    else if (genCheckOptions(param, "-v", "--virtual-device", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a virtual device number (or name) after '%s' in parameter list!\n", param);
        return -1;
      }
      if (virtualDeviceName) {
        fprintf(stderr, "Multiple virtual devices provided, please only provide one device!\n");
        return -1;
      }
      virtualDeviceName = argv[idx];
    }
    else if (genCheckOptions(param, "-c", "--component", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected a component number (or name) after '%s' in parameter list!\n", param);
        return -1;
      }
      if (componentName) {
        fprintf(stderr, "Multiple components provided, please only provide one component!\n");
        return -1;
      }
      componentName = argv[idx];
    }
    else if (genCheckOptions(param, "-h", "--help", NULL)) {
      printf(
        "Usage: skfreq [options] [frequencies...]\n"
        "Options:\n"
        "  -h, --help            Prints this help documentation.\n"
        "  -p, --physical-device Either an integer for physical device index, or the device's name.\n"
        "  -v, --virtual-device  Either an integer for virtual device index, or the device's name.\n"
        "  -c, --component       Either an integer for component index, or the component's name.\n"
        "  -d, --duration        Parses the next argument as a float, for note duration.\n"
        "                        (Default: 1.0f - One second)\n"
        "  -w, --wave            Parses the next argument, uses that as the wave type.\n"
        "                        (Default: sine) (Options: sine, square, triangle, saw)\n"
        "  -i, --intensity       Parses the next argument as a float, for note intensity.\n"
        "                        (Default: 0.25f)\n"
        "\n"
        "Options such as -p, -v, and -c are optional, if no device/component is provided then\n"
        " the default stream will be used in place of a specific device/component.\n"
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
        fprintf(stderr, "Invalid or unsupported argument '%s'! (See --help)\n", param);
        return -1;
      }
      ++frequencyCount;
      frequencies = (float*)realloc(frequencies, sizeof(frequencies) * frequencyCount);
      frequencies[frequencyCount - 1] = frequency;
    }
  }

  // Check that at least one frequency was provided.
  if (frequencyCount == 0) {
    fprintf(stderr, "skfreq requires at least one frequency to play.\n");
    return -1;
  }

  // Check that if a component is named, a device is also named.
  if (componentName) {
    if (!physicalDeviceName && !virtualDeviceName) {
      fprintf(stderr, "A device must be provided along with a component.\n");
      return -1;
    }
  }

  // Check that if a device is named, a component is also named.
  if (physicalDeviceName || virtualDeviceName) {
    if (!componentName) {
      fprintf(stderr, "A component must be provided along with a device.\n");
      return -1;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance.
  //////////////////////////////////////////////////////////////////////////////
  SkResult result;
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "skfreq";
  applicationInfo.pEngineName = "opensk";
  applicationInfo.applicationVersion = 1;
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledExtensionCount = 0;
  instanceInfo.ppEnabledExtensionNames = NULL;

  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (result != SK_SUCCESS) {
    printf("Failed to create OpenSK instance: %d\n", result);
    return -1;
  }

  if (physicalDeviceName) {
    physicalComponent = devResolvePhysicalComponent(instance, physicalDeviceName, componentName);
    if (physicalComponent == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to resolve requested physical component.\n");
      return -1;
    }
  }
  else if (virtualDeviceName) {
    virtualComponent = devResolveVirtualComponent(instance, virtualDeviceName, componentName);
    if (virtualComponent == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to resolve requested virtual component.\n");
      return -1;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Construct a default output stream - uses the best available stream.
  //////////////////////////////////////////////////////////////////////////////
  SkStreamRequestInfo requestInfo;
  memset(&requestInfo, 0, sizeof(SkStreamRequestInfo)); // Clear the request (important!)
  requestInfo.streamType  = SK_STREAM_TYPE_PLAYBACK;    // We want to play sound...
  requestInfo.accessMode  = SK_ACCESS_MODE_BLOCK;       // We want the device to block...
  requestInfo.accessType  = SK_ACCESS_TYPE_INTERLEAVED; // We want to pass interleaved data...
  requestInfo.formatType  = SK_FORMAT_FLOAT;            // We want the audio unit to be float...
  requestInfo.channels    = 2;                          // We want two channels (Left/Right)...
  requestInfo.rate        = 44100;                      // We want it to be 44.1kHz (CD quality)...

  SkStream outputStream;
  SkStreamInfo streamInfo;
  result = devOpenOneOf(instance, physicalComponent, virtualComponent, &requestInfo, &outputStream);
  if (result != SK_SUCCESS) {
    switch (result) {
      case SK_ERROR_STREAM_BUSY:
        fprintf(stderr, "The requested playback stream is busy, and cannot be acquired.\n");
        break;
      case SK_ERROR_STREAM_REQUEST_UNSUPPORTED:
        fprintf(stderr, "The requested playback stream does not support the required configuration.\n");
        break;
      case SK_ERROR_STREAM_REQUEST_FAILED:
        fprintf(stderr, "The requested playback stream could not be acquired for unknown reasons.\n");
        break;
    }
    return 1;
  }
  skGetStreamInfo(outputStream, &streamInfo);

  // Create the output buffer
  uint32_t total = (uint32_t)(streamInfo.rate.value * duration);
  float* buffer  = (float*)malloc(total * streamInfo.sampleBits / 8);
  float* currSample;
  for (uint32_t idx = 0; idx < total; ++idx) {
    currSample = &buffer[idx * streamInfo.channels];

    // Sum all of the frequencies up...
    currSample[0] = 0;
    for (uint32_t ddx = 0; ddx < frequencyCount; ++ddx) {
      currSample[0] += wave((float)idx * frequencies[ddx] / streamInfo.rate.value);
    }
    currSample[0] *= intensity;

    // Apply each resultant frequency to each channel...
    for (uint32_t cnl = 1; cnl < streamInfo.channels; ++cnl) {
      currSample[cnl] = currSample[0];
    }
  }

  // Copy the buffer data into the stream until it all has been played.
  int64_t samplesPlayed;
  uint32_t totalPlayed = 0;
  while (totalPlayed < total) {
    samplesPlayed = skStreamWriteInterleaved(outputStream, buffer, total);
    if (result < 0) {
      fprintf(stderr, "The stream encountered an unrecoverable error: %d\n", -result);
      return -2;
    }
    totalPlayed += samplesPlayed;
  }

  // We no longer need the buffer after it was written.
  free(buffer);
  free(frequencies);

  // Destroy the default stream, waiting for all pending utils to play.
  skDestroyStream(outputStream, SK_TRUE);
  skDestroyInstance(instance);
  return 0;
}
