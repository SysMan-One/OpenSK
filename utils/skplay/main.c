/*******************************************************************************
 * skplay - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A simple application that plays a wave file passed into it.
 ******************************************************************************/

#include <OpenSK/opensk.h>
#include <common/common.h> // utility common functionality
#include <string.h> // strcmp
#include <stdio.h>  // printf
#include <stdlib.h> // realloc

/*******************************************************************************
 * User settings/properties and defaults
 ******************************************************************************/
static uint32_t filenameCount = 0;
static char const** filenames = NULL;
static SkAccessType accessType = SK_ACCESS_TYPE_INTERLEAVED;
static SkAccessMode accessMode = SK_ACCESS_MODE_BLOCK;
static char const* virtualDeviceName = NULL;
static char const* physicalDeviceName = NULL;
static char const* componentName = NULL;
static int allowUnexact = 0;

// Non-user settings:
static SkPhysicalComponent physicalComponent = SK_NULL_HANDLE;
static SkVirtualComponent virtualComponent = SK_NULL_HANDLE;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/
static int
play_subchunk_interleaved(FormatSubChunk* fmt, SubChunk* subChunk, SkStream outputStream) {
  int64_t samplesPlayed;
  uint64_t totalPlayed = 0;
  uint64_t total = 8 * subChunk->Header.SubChunk2Size / (fmt->NumChannels * fmt->BitsPerSample);
  while (totalPlayed < total) {
    samplesPlayed = skStreamWriteInterleaved(outputStream, &subChunk->Data[totalPlayed], total - totalPlayed);
    if (samplesPlayed < 0) {
      fprintf(stderr, "The stream encountered an unrecoverable error: %d\n", (int)-samplesPlayed);
      return 1;
    }
    totalPlayed += samplesPlayed;
  }

  return 0;
}

static int
play_subchunk_noninterleaved(FormatSubChunk* fmt, SubChunk* subChunk, SkStream outputStream) {
  int64_t samplesPlayed;
  uint64_t totalPlayed = 0;
  uint64_t total = 8 * subChunk->Header.SubChunk2Size / (fmt->NumChannels * fmt->BitsPerSample);

  // Create subchunk channels
  char** channels = alloca(sizeof(char*) * fmt->NumChannels);
  for (uint32_t idx = 0; idx < fmt->NumChannels; ++idx) {
    channels[idx] = malloc(total * fmt->BitsPerSample / 8);
    for (uint64_t s = 0; s < total; ++s) {
      memcpy(
        &channels[idx][s * fmt->BitsPerSample / 8],
        &subChunk->Data[(s * fmt->NumChannels + idx) * fmt->BitsPerSample / 8],
        fmt->BitsPerSample / 8
      );
    }
  }

  int err = 0;
  while (totalPlayed < total) {
    samplesPlayed = skStreamWriteNoninterleaved(outputStream, (void const*const*)channels, total - totalPlayed);
    if (samplesPlayed < 0) {
      fprintf(stderr, "The stream encountered an unrecoverable error: %d\n", (int)-samplesPlayed);
      err = 1;
      break;
    }
    totalPlayed += samplesPlayed;
  }

  for (uint32_t idx = 0; idx < fmt->NumChannels; ++idx) {
    free(channels[idx]);
  }

  return 0;
}

static int
play_wav_block(FormatSubChunk* fmt, SubChunk* subChunk, SkStream outputStream, FILE* file) {
  int (*pfn_playback)(FormatSubChunk*, SubChunk*, SkStream);

  switch (accessType) {
    case SK_ACCESS_TYPE_INTERLEAVED:
    case SK_ACCESS_TYPE_MMAP_INTERLEAVED:
    case SK_ACCESS_TYPE_MMAP_COMPLEX:
      pfn_playback = &play_subchunk_interleaved;
      break;
    case SK_ACCESS_TYPE_NONINTERLEAVED:
    case SK_ACCESS_TYPE_MMAP_NONINTERLEAVED:
      pfn_playback = &play_subchunk_noninterleaved;
      break;
  }

  int err;
  while (fileReadChunk(subChunk, file) == 0) {
    err = pfn_playback(fmt, subChunk, outputStream);
    if (err != 0) {
      return err;
    }
  }
  return 0;
}

static int
play_wav_nonblock(FormatSubChunk* fmt, SubChunk* subChunk, SkStream outputStream, FILE* file) {
  fprintf(stderr, "This is not yet implemented!\n");
  return -1;
}

static int
play_wav(SkInstance instance, FILE* file) {
  FormatSubChunk fmt;
  SubChunk subChunk;

  // Read the wave format
  if (fileReadFormat(&fmt, file) != 0) {
    return 1;
  }

  // Check that it's a supported audio type
  AudioType audioType = fileGetAudioType(&fmt);
  switch (audioType) {
    case AT_PCM:
    case AT_FLOAT:
      break;
    case AT_INVALID:
      fprintf(stderr, "Invalid audio type, the file is either corrupt or not a wave.\n");
      return 1;
    case AT_UNKNOWN:
      fprintf(stderr, "Unsupported audio type, this encoding is not supported by skplay.\n");
      return 1;
  }

  // Check that it's a supported wave format
  SkFormat format = fileGetFormatType(audioType, &fmt);
  switch (format) {
    case SK_FORMAT_UNKNOWN:
      fprintf(stderr, "Unsupported wave format, this format is not known to skplay.\n");
      return 1;
    default:
      break;
  }

  // Create the Stream Request
  SkStreamRequestInfo requestInfo;
  memset(&requestInfo, 0, sizeof(SkStreamRequestInfo));
  requestInfo.streamType = SK_STREAM_TYPE_PLAYBACK;
  requestInfo.accessType = accessType;
  requestInfo.accessMode = accessMode;
  requestInfo.channels = fmt.NumChannels;
  requestInfo.rate = fmt.SampleRate;
  requestInfo.formatType = format;

  // Open the output stream
  SkStream outputStream;
  SkResult result = devOpenOneOf(instance, physicalComponent, virtualComponent, &requestInfo, &outputStream);
  if (result != SK_SUCCESS) {
    switch (result) {
      case SK_ERROR_STREAM_BUSY:
        fprintf(stderr, "The requested playback stream is busy, and cannot be acquired.\n");
        break;
      case SK_ERROR_STREAM_REQUEST_UNSUPPORTED:
        fprintf(stderr, "The requested playback stream does not support the requested access mode/type.\n");
        break;
      case SK_ERROR_STREAM_REQUEST_FAILED:
        fprintf(stderr, "The requested playback stream could not be acquired for unknown reasons.\n");
        break;
    }
    return 1;
  }

  // Check the stream we acquired is what was expected
  SkStreamInfo streamInfo;
  skGetStreamInfo(outputStream, &streamInfo);
  int transientErrors = 0;
  if (streamInfo.rate.value != requestInfo.rate) {
    ++transientErrors;
    fprintf(stderr, "The stream does not support the bitrate '%d' (acquired: %d).\n", requestInfo.rate, streamInfo.rate.value);
  }
  if (streamInfo.channels != requestInfo.channels) {
    ++transientErrors;
    fprintf(stderr, "The stream does not support the number of channels '%d' (acquired: %d).\n", requestInfo.channels, streamInfo.channels);
  }
  if (transientErrors > 0) {
    if (allowUnexact) {
      fprintf(stderr, "Because of the previous errors, the output audio may not play correctly.\n");
    }
    else {
      fprintf(stderr, "Aborting due to previous errors.\n");
      skDestroyStream(outputStream, SK_FALSE);
      return -1;
    }
  }

  // Prepare to play the subchunk
  int err;
  memset(&subChunk, 0, sizeof(SubChunk));
  switch (accessMode) {
    case SK_ACCESS_MODE_BLOCK:
      err = play_wav_block(&fmt, &subChunk, outputStream, file);
      break;
    case SK_ACCESS_MODE_NONBLOCK:
      err = play_wav_nonblock(&fmt, &subChunk, outputStream, file);
      break;
  }
  free(subChunk.Data);

  skDestroyStream(outputStream, SK_TRUE);
  return err;
}

static int
play_file(SkInstance instance, FILE* file) {
  RiffChunk riff;
  if (fileReadRiff(&riff, file) != 0) {
    fprintf(stderr, "Failed to read file!\n");
    return 1;
  }
  switch (fileGetType(&riff)) {
    case FT_WAVE:
      return play_wav(instance, file);
    case FT_UNKNOWN:
    case FT_INVALID:
      fprintf(stderr, "Unsupported or corrupt file, please try another file.\n");
      break;
  }
  return 1;
}

/*******************************************************************************
 * Main Entry Point
 ******************************************************************************/
int
main(int argc, char const *argv[]) {

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (int idx = 1; idx < argc; ++idx) {
    char const* param = argv[idx];
    if (genCheckOptions(param, "-t", "--access-type", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected an access type after '%s' in parameter list!\n", param);
        return -1;
      }
      if (strcmp(argv[idx], "interleaved") == 0) {
        accessType = SK_ACCESS_TYPE_INTERLEAVED;
      }
      else if (strcmp(argv[idx], "noninterleaved") == 0) {
        accessType = SK_ACCESS_TYPE_NONINTERLEAVED;
      }
      else if (strcmp(argv[idx], "mmap-interleaved") == 0) {
        accessType = SK_ACCESS_TYPE_MMAP_INTERLEAVED;
      }
      else if (strcmp(argv[idx], "mmap-noninterleaved") == 0) {
        accessType = SK_ACCESS_TYPE_MMAP_NONINTERLEAVED;
      }
      else if (strcmp(argv[idx], "mmap-complex") == 0) {
        accessType = SK_ACCESS_TYPE_MMAP_COMPLEX;
      }
      else {
        fprintf(stderr, "Unexpected access type '%s'!\n", argv[idx]);
        return -1;
      }
    }
    else if (genCheckOptions(param, "-m", "--access-mode", NULL)) {
      ++idx;
      if (idx >= argc) {
        fprintf(stderr, "Expected an access mode after '%s' in parameter list!\n", param);
        return -1;
      }
      if (strcmp(argv[idx], "block") == 0) {
        accessMode = SK_ACCESS_MODE_BLOCK;
      }
      else if (strcmp(argv[idx], "nonblock") == 0) {
        accessMode = SK_ACCESS_MODE_NONBLOCK;
      }
      else {
        fprintf(stderr, "Unexpected access mode '%s'!\n", argv[idx]);
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
    else if (genCheckOptions(param, "-U", "--allow-unexact", NULL)) {
      allowUnexact = 1;
    }
    else if (genCheckOptions(param, "-h", "--help", NULL)) {
      printf(
        "Usage: skplay [options] [files...]\n"
        "Options:\n"
        "  -h, --help            Prints this help documentation.\n"
        "  -p, --physical-device Either an integer for physical device index, or the device's name.\n"
        "  -v, --virtual-device  Either an integer for virtual device index, or the device's name.\n"
        "  -c, --component       Either an integer for component index, or the component's name.\n"
        "  -t, --access-type     The underlying access type for the playback stream.\n"
        "                        Not all hardware/software will support all access types.\n"
        "                        One of: interleaved, noninterleaved, mmap-interleaved,\n"
        "                                mmap-noninterleaved, mmap-complex\n"
        "                        (Default: interleaved)\n"
        "  -m, --access-mode     The underlying access mode for the playback stream.\n"
        "                        One of: block, nonblock\n"
        "                        (Default: block)\n"
        "  -U, --allow-unexact   Don't err out if the stream is not exactly what is required for playback.\n"
        "\n"
        "Options such as -p, -v, and -c are optional, if no device/component is provided then\n"
        " the default stream will be used in place of a specific device/component.\n"
      );
      return 0;
    }
    else {
      ++filenameCount;
      filenames = (char const**)realloc(filenames, sizeof(char const*) * filenameCount);
      filenames[filenameCount - 1] = argv[idx];
    }
  }

  // Check that at least one sound file was provided.
  if (filenameCount == 0) {
    fprintf(stderr, "skplay requires at least one file to play.\n");
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
  applicationInfo.pApplicationName = "skplay";
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

  int errors = 0;
  FILE* file = NULL;
  for (uint32_t idx = 0; idx < filenameCount; ++idx) {
    file = fopen(filenames[idx], "rb");
    if (file == NULL) {
      fprintf(stderr, "Failed to open the file '%s'!\n", filenames[idx]);
      continue;
    }
    printf("Playing file '%s'...\n", filenames[idx]);
    errors += play_file(instance, file);
    fclose(file);
  }

  skDestroyInstance(instance);
  return errors;
}
