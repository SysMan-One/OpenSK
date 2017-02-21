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
 * A structure for holding on to terminal font information for pretty-printing.
 ******************************************************************************/
#ifndef   OPENSK_UTL_COLOR_CONFIG_H
#define   OPENSK_UTL_COLOR_CONFIG_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/plt/platform.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Color Config Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkColorConfigUTL);

typedef enum SkColorFormatUTL {
  SK_COLOR_DEFAULT       = 0x0000,

  // Background Colors
  SK_COLOR_RESET_BG      = 0x0000,
  SK_COLOR_BLACK_BG      = 0x0001,
  SK_COLOR_RED_BG        = 0x0002,
  SK_COLOR_GREEN_BG      = 0x0003,
  SK_COLOR_YELLOW_BG     = 0x0004,
  SK_COLOR_BLUE_BG       = 0x0005,
  SK_COLOR_MAGENTA_BG    = 0x0006,
  SK_COLOR_CYAN_BG       = 0x0007,
  SK_COLOR_GRAY_BG       = 0x0008,
  SK_COLOR_COLOR_MASK_BG = 0x000F,

  // Foreground Colors
  SK_COLOR_RESET_FG      = 0x0000,
  SK_COLOR_BLACK_FG      = 0x0010,
  SK_COLOR_RED_FG        = 0x0020,
  SK_COLOR_GREEN_FG      = 0x0030,
  SK_COLOR_YELLOW_FG     = 0x0040,
  SK_COLOR_BLUE_FG       = 0x0050,
  SK_COLOR_MAGENTA_FG    = 0x0060,
  SK_COLOR_CYAN_FG       = 0x0070,
  SK_COLOR_GRAY_FG       = 0x0080,
  SK_COLOR_COLOR_MASK_FG = 0x00F0,

  // Font Effects
  SK_COLOR_BOLD_BIT      = 0x0100,
  SK_COLOR_FAINT_BIT     = 0x0200,
  SK_COLOR_ITALIC_BIT    = 0x0400,
  SK_COLOR_UNDERLINE_BIT = 0x0800,
  SK_COLOR_BLINK_BIT     = 0x1000,
  SK_COLOR_NEGATIVE_BIT  = 0x2000,

  SK_COLOR_EFFECTS_MASK  = 0xFF00
} SkColorFormatUTL;

////////////////////////////////////////////////////////////////////////////////
// Color Config Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreateColorConfigUTL(
  SkPlatformPLT                         platform,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkColorConfigUTL*                     pColorConfig
);

void SKAPI_CALL skDestroyColorConfigUTL(
  SkColorConfigUTL                      colorConfig,
  SkAllocationCallbacks const*          pAllocator
);

SkResult SKAPI_CALL skEnumerateColorConfigFilesUTL(
  SkColorConfigUTL                      colorConfig,
  uint32_t*                             pFileCount,
  char const**                          pFiles
);

SkResult SKAPI_CALL skProcessColorConfigFileUTL(
  SkColorConfigUTL                      colorConfig,
  char const*                           pFilename
);

SkColorFormatUTL SKAPI_CALL skGetColorConfigFormatUTL(
  SkColorConfigUTL                      colorConfig,
  SkObject                              object
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_UTL_COLOR_CONFIG_H
