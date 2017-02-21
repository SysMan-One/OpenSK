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
 * Conversion functions to/from ALSA types. Separated for cleanliness.
 ******************************************************************************/

#ifndef   OPENSK_ALSA_CONVERT_H
#define   OPENSK_ALSA_CONVERT_H 1

// OpenSK
#include <OpenSK/opensk.h>

// ALSA
#include <alsa/asoundlib.h>

////////////////////////////////////////////////////////////////////////////////
// Conversion Defines
////////////////////////////////////////////////////////////////////////////////

#define SND_MAX_IDENTIFIER_SIZE 35

////////////////////////////////////////////////////////////////////////////////
// Conversion Functions
////////////////////////////////////////////////////////////////////////////////

static SkStreamFlagBits skConvertFromLocalPcmStreamTypeIMPL(
  snd_pcm_stream_t                      streamType
) {
  switch (streamType) {
    case SND_PCM_STREAM_CAPTURE:
      return SK_STREAM_PCM_READ_BIT;
    case SND_PCM_STREAM_PLAYBACK:
      return SK_STREAM_PCM_WRITE_BIT;
    default:
      return (SkStreamFlagBits)0;
  }
}

static SkStreamFlagBits skConvertFromLocalMidiStreamTypeIMPL(
  snd_rawmidi_stream_t                  streamType
) {
  switch (streamType) {
    case SND_RAWMIDI_STREAM_INPUT:
      return SK_STREAM_PCM_READ_BIT;
    case SND_RAWMIDI_STREAM_OUTPUT:
      return SK_STREAM_PCM_WRITE_BIT;
    default:
      return (SkStreamFlagBits)0;
  }
}

static SkAccessFlags skConvertFromLocalAccessTypeIMPL(
  snd_pcm_access_t                      accessMode,
  int                                   blockingMode
) {
  SkAccessFlags accessFlags;
  accessFlags = (SkAccessFlags)0;
  switch (accessMode) {
    case SND_PCM_ACCESS_RW_INTERLEAVED:
      accessFlags = SK_ACCESS_INTERLEAVED;
      break;
    case SND_PCM_ACCESS_RW_NONINTERLEAVED:
      accessFlags = SK_ACCESS_NONINTERLEAVED;
      break;
    case SND_PCM_ACCESS_MMAP_INTERLEAVED:
      accessFlags = SK_ACCESS_INTERLEAVED | SK_ACCESS_MEMORY_MAPPED;
      break;
    case SND_PCM_ACCESS_MMAP_NONINTERLEAVED:
      accessFlags = SK_ACCESS_NONINTERLEAVED | SK_ACCESS_MEMORY_MAPPED;
      break;
    default:
      return SK_ACCESS_FLAG_BITS_MAX_ENUM;
  }
  switch (blockingMode) {
    case 0:
      accessFlags |= SK_ACCESS_BLOCKING;
      break;
    case SND_PCM_NONBLOCK:
      accessFlags |= SK_ACCESS_NONBLOCKING;
      break;
    default:
      return SK_ACCESS_FLAG_BITS_MAX_ENUM;
  }
  return accessFlags;
}

static SkPcmFormat skConvertFromLocalPcmFormatIMPL(
  snd_pcm_format_t                      format
) {
  switch (format) {
    case SND_PCM_FORMAT_S8:
      return SK_PCM_FORMAT_S8;
    case SND_PCM_FORMAT_U8:
      return SK_PCM_FORMAT_U8;
    case SND_PCM_FORMAT_S16_LE:
      return SK_PCM_FORMAT_S16_LE;
    case SND_PCM_FORMAT_S16_BE:
      return SK_PCM_FORMAT_S16_BE;
    case SND_PCM_FORMAT_U16_LE:
      return SK_PCM_FORMAT_U16_LE;
    case SND_PCM_FORMAT_U16_BE:
      return SK_PCM_FORMAT_U16_BE;
    case SND_PCM_FORMAT_S24_LE:
      return SK_PCM_FORMAT_S24_LE;
    case SND_PCM_FORMAT_S24_BE:
      return SK_PCM_FORMAT_S24_BE;
    case SND_PCM_FORMAT_U24_LE:
      return SK_PCM_FORMAT_U24_LE;
    case SND_PCM_FORMAT_U24_BE:
      return SK_PCM_FORMAT_U24_BE;
    case SND_PCM_FORMAT_S32_LE:
      return SK_PCM_FORMAT_S32_LE;
    case SND_PCM_FORMAT_S32_BE:
      return SK_PCM_FORMAT_S32_BE;
    case SND_PCM_FORMAT_U32_LE:
      return SK_PCM_FORMAT_U32_LE;
    case SND_PCM_FORMAT_U32_BE:
      return SK_PCM_FORMAT_U32_BE;
    case SND_PCM_FORMAT_FLOAT_LE:
      return SK_PCM_FORMAT_F32_LE;
    case SND_PCM_FORMAT_FLOAT_BE:
      return SK_PCM_FORMAT_F32_BE;
    case SND_PCM_FORMAT_FLOAT64_LE:
      return SK_PCM_FORMAT_F64_LE;
    case SND_PCM_FORMAT_FLOAT64_BE:
      return SK_PCM_FORMAT_F64_BE;
    default:
      if ((uint32_t)format == SND_PCM_FORMAT_ANY) {
        return SK_PCM_FORMAT_UNDEFINED;
      }
      return SK_PCM_FORMAT_UNKNOWN;
  }
}

static SkChannel skConvertFromLocalChannelIMPL(
  unsigned int                          channel
) {
  switch (channel) {
    case SND_CHMAP_NA:
      return SK_CHANNEL_NOT_APPLICABLE;
    case SND_CHMAP_MONO:
      return SK_CHANNEL_MONO;
    case SND_CHMAP_FL:
      return SK_CHANNEL_FRONT_LEFT;
    case SND_CHMAP_FR:
      return SK_CHANNEL_FRONT_RIGHT;
    case SND_CHMAP_FC:
      return SK_CHANNEL_FRONT_CENTER;
    case SND_CHMAP_LFE:
      return SK_CHANNEL_LOW_FREQUENCY;
    case SND_CHMAP_SL:
      return SK_CHANNEL_SIDE_LEFT;
    case SND_CHMAP_SR:
      return SK_CHANNEL_SIDE_RIGHT;
    case SND_CHMAP_RL:
      return SK_CHANNEL_BACK_LEFT;
    case SND_CHMAP_RR:
      return SK_CHANNEL_BACK_RIGHT;
    case SND_CHMAP_RC:
      return SK_CHANNEL_BACK_CENTER;
    default:
      return SK_CHANNEL_UNKNOWN;
  }
}

static snd_pcm_format_t skConvertToLocalPcmFormatIMPL(
  SkPcmFormat                           format
) {
  switch (format) {
    case SK_PCM_FORMAT_UNDEFINED:
      return SND_PCM_FORMAT_ANY;
    case SK_PCM_FORMAT_S8:
      return SND_PCM_FORMAT_S8;
    case SK_PCM_FORMAT_U8:
      return SND_PCM_FORMAT_U8;
    case SK_PCM_FORMAT_S16_LE:
      return SND_PCM_FORMAT_S16_LE;
    case SK_PCM_FORMAT_S16_BE:
      return SND_PCM_FORMAT_S16_BE;
    case SK_PCM_FORMAT_U16_LE:
      return SND_PCM_FORMAT_U16_LE;
    case SK_PCM_FORMAT_U16_BE:
      return SND_PCM_FORMAT_U16_BE;
    case SK_PCM_FORMAT_S24_LE:
      return SND_PCM_FORMAT_S24_LE;
    case SK_PCM_FORMAT_S24_BE:
      return SND_PCM_FORMAT_S24_BE;
    case SK_PCM_FORMAT_U24_LE:
      return SND_PCM_FORMAT_U24_LE;
    case SK_PCM_FORMAT_U24_BE:
      return SND_PCM_FORMAT_U24_BE;
    case SK_PCM_FORMAT_S32_LE:
      return SND_PCM_FORMAT_S32_LE;
    case SK_PCM_FORMAT_S32_BE:
      return SND_PCM_FORMAT_S32_BE;
    case SK_PCM_FORMAT_U32_LE:
      return SND_PCM_FORMAT_U32_LE;
    case SK_PCM_FORMAT_U32_BE:
      return SND_PCM_FORMAT_U32_BE;
    case SK_PCM_FORMAT_F32_LE:
      return SND_PCM_FORMAT_FLOAT_LE;
    case SK_PCM_FORMAT_F32_BE:
      return SND_PCM_FORMAT_FLOAT_BE;
    case SK_PCM_FORMAT_F64_LE:
      return SND_PCM_FORMAT_FLOAT64_LE;
    case SK_PCM_FORMAT_F64_BE:
      return SND_PCM_FORMAT_FLOAT64_BE;
    default:
      return SND_PCM_FORMAT_UNKNOWN;
  }
}

#endif // OPENSK_ALSA_CONVERT_H
