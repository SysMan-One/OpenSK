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
 * Driver-specific stream request structures and global definitions.
 ******************************************************************************/
#ifndef   OPENSK_ICD_ALSA_H
#define   OPENSK_ICD_ALSA_H 1

// Includes
#include <alsa/asoundlib.h>
#include <OpenSK/opensk.h>

////////////////////////////////////////////////////////////////////////////////
// ICD Defines
////////////////////////////////////////////////////////////////////////////////
#define SK_DRIVER_OPENSK_ALSA "SK_DRIVER_OPENSK_ALSA"
#ifndef   SND_PCM_ACCESS_ANY
#define   SND_PCM_ACCESS_ANY (SND_PCM_ACCESS_LAST + 1)
#endif // SND_PCM_ACCESS_ANY
#ifndef   SND_PCM_FORMAT_ANY
#define   SND_PCM_FORMAT_ANY (SND_PCM_FORMAT_LAST + 1)
#endif // SND_PCM_FORMAT_ANY
#ifndef   SND_PCM_SUBFORMAT_ANY
#define   SND_PCM_SUBFORMAT_ANY (SND_PCM_SUBFORMAT_LAST + 1)
#endif // SND_PCM_SUBFORMAT_ANY
#ifndef   SND_PCM_BLOCK
#define   SND_PCM_BLOCK 0
#endif // SND_PCM_BLOCK

////////////////////////////////////////////////////////////////////////////////
// ICD Types
////////////////////////////////////////////////////////////////////////////////
typedef struct SkAlsaHardwareParameters {
  int                                   blockingMode;
  snd_pcm_stream_t                      streamType;
  snd_pcm_access_t                      accessMode;
  snd_pcm_format_t                      formatType;
  snd_pcm_subformat_t                   subformatType;
  unsigned int                          channels;
  unsigned int                          rate;
  int                                   rateDir;
  unsigned int                          periods;
  int                                   periodsDir;
  snd_pcm_uframes_t                     periodSize;
  int                                   periodSizeDir;
  unsigned int                          periodTime;
  int                                   periodTimeDir;
  snd_pcm_uframes_t                     bufferSize;
  unsigned int                          bufferTime;
  int                                   bufferTimeDir;
} SkAlsaHardwareParameters;

typedef struct SkAlsaSoftwareParameters {
  snd_pcm_uframes_t                     availMin;
  snd_pcm_uframes_t                     silenceSize;
  snd_pcm_uframes_t                     silenceThreshold;
  snd_pcm_uframes_t                     startThreshold;
  snd_pcm_uframes_t                     stopThreshold;
} SkAlsaSoftwareParameters;

typedef struct SkAlsaPcmStreamRequest {
  SkStructureType                       sType;
  void const*                           pNext;
  SkAlsaHardwareParameters              hw;
  SkAlsaSoftwareParameters              sw;
} SkAlsaPcmStreamRequest;

typedef struct SkAlsaPcmStreamInfo {
  SkStructureType                       sType;
  void const*                           pNext;
  SkAlsaHardwareParameters              hw;
  SkAlsaSoftwareParameters              sw;
} SkAlsaPcmStreamInfo;

////////////////////////////////////////////////////////////////////////////////
// ICD Functions
////////////////////////////////////////////////////////////////////////////////
SKAPI_ATTR void SKAPI_CALL skGetDriverProperties_alsa(
  SkDriver                             driver,
  SkDriverProperties*                  pProperties
);

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetDriverProcAddr_alsa(
  SkDriver                              driver,
  char const*                           symbol
);

SKAPI_ATTR PFN_skVoidFunction SKAPI_CALL skGetPcmStreamProcAddr_alsa(
  SkPcmStream                           stream,
  char const*                           symbol
);

#endif // OPENSK_ICD_ALSA_H
