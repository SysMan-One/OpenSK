/*******************************************************************************
 * OpenSK (Utils) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * WARNING: Nonstandard functionality, not intended for use outside of skutils
 ******************************************************************************/
#include "common.h"
#include <stdarg.h> // va_arg
#include <stddef.h> // NULL
#include <string.h> // strcmp
#include <stdlib.h> // realloc

int
genCheckOptions(char const *param, ...) {
  int passing = 0;
  va_list ap;

  va_start(ap, param);
  char const *cmp = va_arg(ap, char const*);
  while (cmp != NULL)
  {
    if (strcmp(param, cmp) == 0) {
      passing = 1;
      break;
    }
    cmp = va_arg(ap, char const*);
  }
  va_end(ap);

  return passing;
}

static union { uint32_t u32; unsigned char u8[4]; } intEndianCheck = { (uint32_t)0x01234567 };
int
genIsIntegerBigEndian() {
  return intEndianCheck.u8[3] == 0x01;
}

static union { float f32; unsigned char u8[4]; } floatEndianCheck = { (float)0x01234567 };
int
genIsFloatBigEndian() {
  return floatEndianCheck.u8[3] == 0x01;
}

uint16_t
genReverseEndiannessU16(uint16_t ui) {
  return (ui << 8) | (ui >> 8);
}

uint32_t
genReverseEndiannessU32(uint32_t ui) {
  return (ui << 24) | ((0xFF00 & ui) << 8) | (0xFF00 & (ui >> 8)) | (ui >> 24);
}

uint64_t
genReverseEndiannessU64(uint64_t ui) {
  return
    ((0xFF000000 & ui) << 8) | (0xFF000000 & (ui >> 8)) |
    ((0xFF0000 & ui) << 24) | (0xFF0000 & (ui >> 24)) |
    ((0xFF00 & ui) << 40) | (0xFF00 & (ui >> 40)) |
    (ui << 56) | (ui >> 56);
}

int16_t
genReverseEndiannessS16(int16_t i) {
  return (int16_t)genReverseEndiannessU16((uint16_t)i);
}

int32_t
genReverseEndiannessS32(int32_t i) {
  return (int16_t)genReverseEndiannessU32((uint16_t)i);
}

int64_t
genReverseEndiannessS64(int64_t i) {
  return (int16_t)genReverseEndiannessU64((uint16_t)i);
}

float
genReverseEndiannessFloat32(float f) {
  return (float)genReverseEndiannessU32((uint32_t)f);
}

long double
genReverseEndiannessFloat64(long double f) {
  return (long double)genReverseEndiannessU64((uint64_t)f);
}

FileType
fileGetType(RiffChunk const* chunk) {
  if (chunk->ChunkId.AsUint32 != 0x52494646) {
    return FT_INVALID;
  }
  if (chunk->Format.AsUint32  == 0x57415645) {
    return FT_WAVE;
  }
  return FT_UNKNOWN;
}

int
fileReadRiff(RiffChunk* chunk, FILE* file) {
  if (fread(chunk, sizeof(RiffChunk), 1, file) != 1) {
    return GENREAD;
  }
  if (genIsIntegerBigEndian()) {
    chunk->ChunkId.AsUint32 = genReverseEndiannessU32(chunk->ChunkId.AsUint32);
    chunk->Format.AsUint32 = genReverseEndiannessU32(chunk->Format.AsUint32);
  }
  else {
    chunk->ChunkSize = genReverseEndiannessU32(chunk->ChunkSize);
  }
  return 0;
}

AudioType
fileGetAudioType(FormatSubChunk const* fmt) {
  if (fmt->SubChunk1ID.AsUint32 != 0x666d7420) {
    return AT_INVALID;
  }
  switch (fmt->AudioFormat) {
    case 1: return AT_PCM;
    case 3: return AT_FLOAT;
    default: break;
  }
  return AT_UNKNOWN;
}

SkFormat
fileGetFormatType(AudioType at, FormatSubChunk const* fmt) {
  if (at > 0) {
    switch (fmt->BitsPerSample) {
      case 8 : return SK_FORMAT_U8;
      case 16: return SK_FORMAT_S16_LE;
      case 32: return (at == AT_FLOAT) ? SK_FORMAT_FLOAT : SK_FORMAT_UNKNOWN;
      default: break;
    }
  }
  return SK_FORMAT_UNKNOWN;
}

int
fileReadFormat(FormatSubChunk *chunk, FILE* file) {
  if (fread(chunk, sizeof(FormatSubChunk), 1, file) != 1) {
    return GENREAD;
  }
  if (genIsIntegerBigEndian()) {
    chunk->SubChunk1ID.AsUint32 = genReverseEndiannessU32(chunk->SubChunk1ID.AsUint32);
  }
  else {
    chunk->SubChunk1Size  = genReverseEndiannessU32(chunk->SubChunk1Size);
    chunk->AudioFormat    = genReverseEndiannessU16(chunk->AudioFormat);
    chunk->NumChannels    = genReverseEndiannessU16(chunk->NumChannels);
    chunk->SampleRate     = genReverseEndiannessU32(chunk->SampleRate);
    chunk->ByteRate       = genReverseEndiannessU32(chunk->ByteRate);
    chunk->BlockAlign     = genReverseEndiannessU16(chunk->BlockAlign);
    chunk->BitsPerSample  = genReverseEndiannessU16(chunk->BitsPerSample);
  }

  // Ignore the extended chunk information
  for (uint32_t idx = 16; idx < chunk->SubChunk1Size; ++idx) {
    if (fgetc(file) < 0) {
      return GENREAD;
    }
  }

  return 0;
}

int
fileReadChunk(SubChunk* subChunk, FILE* file) {
  if (fread(&subChunk->Header, sizeof(SubChunkHeader), 1, file) != 1) {
    return GENDONE;
  }

  // Convert endianness
  if (genIsIntegerBigEndian()) {
    subChunk->Header.SubChunk2ID.AsUint32 = genReverseEndiannessU32(subChunk->Header.SubChunk2ID.AsUint32);
  }
  else {
    subChunk->Header.SubChunk2Size = genReverseEndiannessU32(subChunk->Header.SubChunk2Size);
  }

  // Prepare subchunk buffer
  subChunk->Data = realloc(subChunk->Data, subChunk->Header.SubChunk2Size);
  if (subChunk->Data == NULL) {
    return GENALLOC;
  }

  // Read subchunk data
  if (fread(subChunk->Data, subChunk->Header.SubChunk2Size, 1, file) != 1) {
    return GENREAD;
  }

  return 0;
}

SkPhysicalDevice
devFindPhysicalDevice(SkPhysicalDevice *devices, uint32_t deviceCount, char const* name) {
  int deviceId;
  if (sscanf(name, "%d", &deviceId) == 1) {
    if (deviceId < deviceCount) {
      return devices[deviceId];
    }
  }
  else {
    SkDeviceProperties properties;
    for (uint32_t idx = 0; idx < deviceCount; ++idx) {
      skGetPhysicalDeviceProperties(devices[idx], &properties);
      if (strcmp(properties.deviceName, name) == 0) {
        return devices[idx];
      }
    }
  }
  return SK_NULL_HANDLE;
}

SkPhysicalComponent
devFindPhysicalComponent(SkPhysicalComponent *components, uint32_t componentCount, char const* name) {
  int componentId;
  if (sscanf(name, "%d", &componentId) == 1) {
    if (componentId < componentCount) {
      return components[componentId];
    }
  }
  else {
    SkComponentProperties properties;
    for (uint32_t idx = 0; idx < componentCount; ++idx) {
      skGetPhysicalComponentProperties(components[idx], &properties);
      if (strcmp(properties.componentName, name) == 0) {
        return components[idx];
      }
    }
  }
  return SK_NULL_HANDLE;
}

SkPhysicalComponent
devResolvePhysicalComponent(SkInstance instance, char const* dev, char const* comp) {
  if (dev && comp) {
    SkResult result;
    uint32_t deviceCount;
    result = skEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to enumerate physical devices.\n");
      return SK_NULL_HANDLE;
    }

    SkPhysicalDevice *devices = alloca(sizeof(SkPhysicalDevice) * deviceCount);
    result = skEnumeratePhysicalDevices(instance, &deviceCount, devices);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to populate physical devices.\n");
      return SK_NULL_HANDLE;
    }

    SkPhysicalDevice device = devFindPhysicalDevice(devices, deviceCount, dev);
    if (device == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to find the requested physical device.\n");
      return SK_NULL_HANDLE;
    }

    uint32_t componentCount;
    result = skEnumeratePhysicalComponents(device, &componentCount, NULL);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to enumerate physical components.\n");
      return SK_NULL_HANDLE;
    }

    SkPhysicalComponent *components = alloca(sizeof(SkPhysicalComponent) * componentCount);
    result = skEnumeratePhysicalComponents(device, &componentCount, components);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to populate physical components.\n");
      return SK_NULL_HANDLE;
    }

    SkPhysicalComponent physicalComponent = devFindPhysicalComponent(components, componentCount, comp);
    if (physicalComponent == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to find the requested physical component.\n");
      return SK_NULL_HANDLE;
    }

    return physicalComponent;
  }
  return SK_NULL_HANDLE;
}

SkVirtualDevice
devFindVirtualDevice(SkVirtualDevice *devices, uint32_t deviceCount, char const* name) {
  int deviceId;
  if (sscanf(name, "%d", &deviceId) == 1) {
    if (deviceId < deviceCount) {
      return devices[deviceId];
    }
  }
  else {
    SkDeviceProperties properties;
    for (uint32_t idx = 0; idx < deviceCount; ++idx) {
      skGetVirtualDeviceProperties(devices[idx], &properties);
      if (strcmp(properties.deviceName, name) == 0) {
        return devices[idx];
      }
    }
  }
  return SK_NULL_HANDLE;
}

SkVirtualComponent
devFindVirtualComponent(SkVirtualComponent *components, uint32_t componentCount, char const* name) {
  int componentId;
  if (sscanf(name, "%d", &componentId) == 1) {
    if (componentId < componentCount) {
      return components[componentId];
    }
  }
  else {
    SkComponentProperties properties;
    for (uint32_t idx = 0; idx < componentCount; ++idx) {
      skGetVirtualComponentProperties(components[idx], &properties);
      if (strcmp(properties.componentName, name) == 0) {
        return components[idx];
      }
    }
  }
  return SK_NULL_HANDLE;
}

SkVirtualComponent
devResolveVirtualComponent(SkInstance instance, char const* dev, char const* comp) {
  if (dev && comp) {
    SkResult result;
    uint32_t deviceCount;
    result = skEnumerateVirtualDevices(instance, &deviceCount, NULL);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to enumerate virtual devices.\n");
      return SK_NULL_HANDLE;
    }

    SkVirtualDevice *devices = alloca(sizeof(SkVirtualDevice) * deviceCount);
    result = skEnumerateVirtualDevices(instance, &deviceCount, devices);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to populate virtual devices.\n");
      return SK_NULL_HANDLE;
    }

    SkVirtualDevice device = devFindVirtualDevice(devices, deviceCount, dev);
    if (device == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to find the requested virtual device.\n");
      return SK_NULL_HANDLE;
    }

    uint32_t componentCount;
    result = skEnumerateVirtualComponents(device, &componentCount, NULL);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to enumerate virtual components.\n");
      return SK_NULL_HANDLE;
    }

    SkVirtualComponent *components = alloca(sizeof(SkVirtualComponent) * componentCount);
    result = skEnumerateVirtualComponents(device, &componentCount, components);
    if (result != SK_SUCCESS) {
      fprintf(stderr, "Failed to populate virtual components.\n");
      return SK_NULL_HANDLE;
    }

    SkVirtualComponent virtualComponent = devFindVirtualComponent(components, componentCount, comp);
    if (virtualComponent == SK_NULL_HANDLE) {
      fprintf(stderr, "Failed to find the requested virtual component.\n");
      return SK_NULL_HANDLE;
    }

    return virtualComponent;
  }

  return SK_NULL_HANDLE;
}

SkResult
devOpenOneOf(SkInstance instance, SkPhysicalComponent physicalComponent, SkVirtualComponent virtualComponent, SkStreamRequestInfo *info, SkStream* stream) {
  if (physicalComponent != SK_NULL_HANDLE) {
    return skRequestPhysicalStream(physicalComponent, info, stream);
  }
  else if (virtualComponent != SK_NULL_HANDLE) {
    return skRequestVirtualStream(virtualComponent, info, stream);
  }
  return skRequestDefaultStream(instance, info, stream);
}