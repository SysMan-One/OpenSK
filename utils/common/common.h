/*******************************************************************************
 * OpenSK (Utils) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * WARNING: Nonstandard functionality, not intended for use outside of skutils
 ******************************************************************************/
#ifndef   OPENSK_COMMON_H
#define   OPENSK_COMMON_H

#include <OpenSK/opensk.h>
#include <stdint.h>// *int*_t
#include <stdio.h> // FILE

typedef enum FileType {
  FT_WAVE,
  FT_UNKNOWN = -1,
  FT_INVALID = -2
} FileType;

typedef enum AudioType {
  AT_PCM = 1,
  AT_FLOAT = 3,
  AT_UNKNOWN = -1,
  AT_INVALID = -2
} AudioType;

typedef union Identifier {
  char AsChar[4];
  uint32_t AsUint32;
} Identifier;

typedef struct RiffChunk {
  Identifier ChunkId;
  uint32_t ChunkSize;
  Identifier Format;
} RiffChunk;

typedef struct FormatSubChunk {
  Identifier SubChunk1ID;
  uint32_t SubChunk1Size;
  uint16_t AudioFormat;
  uint16_t NumChannels;
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
} FormatSubChunk;

typedef struct SubChunkHeader {
  Identifier SubChunk2ID;
  uint32_t SubChunk2Size;
} SubChunkHeader;

typedef struct SubChunk {
  SubChunkHeader Header;
  char* Data;
} SubChunk;

// errors
#define GENREAD 1
#define GENDONE 2
#define GENALLOC 2

// general functionality
int genCheckOptions(char const* param, ...);
int genIsIntegerBigEndian();
int genIsFloatBigEndian();
uint16_t genReverseEndiannessU16(uint16_t ui);
uint32_t genReverseEndiannessU32(uint32_t ui);
uint64_t genReverseEndiannessU64(uint64_t ui);
int16_t genReverseEndiannessS16(int16_t i);
int32_t genReverseEndiannessS32(int32_t i);
int64_t genReverseEndiannessS64(int64_t i);
float genReverseEndiannessFloat32(float f);
long double genReverseEndiannessFloat64(long double f);

// file reading
FileType fileGetType(RiffChunk const* chunk);
int fileReadRiff(RiffChunk* chunk, FILE* file);
AudioType fileGetAudioType(FormatSubChunk const* fmt);
SkFormat fileGetFormatType(AudioType at, FormatSubChunk const* fmt);
int fileReadFormat(FormatSubChunk* fmt, FILE* file);
int fileReadChunk(SubChunk* chunk, FILE* file);

// device handling
SkPhysicalDevice devFindPhysicalDevice(SkPhysicalDevice* comps, uint32_t count, char const* name);
SkPhysicalComponent devFindPhysicalComponent(SkPhysicalComponent* comps, uint32_t count, char const* name);
SkPhysicalComponent devResolvePhysicalComponent(SkInstance instance, char const* device, char const* comp);
SkVirtualDevice devFindVirtualDevice(SkVirtualDevice* comps, uint32_t count, char const* name);
SkVirtualComponent devFindVirtualComponent(SkVirtualComponent* comps, uint32_t count, char const* name);
SkVirtualComponent devResolveVirtualComponent(SkInstance instance, char const* device, char const* comp);
SkResult devOpenOneOf(SkInstance instance, SkPhysicalComponent physicalComponent, SkVirtualComponent virtualComponent, SkStreamRequestInfo *info, SkStream* stream);

#endif // OPENSK_COMMON_H
