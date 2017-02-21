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
 * OpenSK manifest header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_MAN_MANIFEST_H
#define   OPENSK_MAN_MANIFEST_H 1

#include <OpenSK/dev/utils.h>
#include <OpenSK/ext/sk_driver.h>
#include <OpenSK/ext/sk_layer.h>

#define SK_MANIFEST_VERSION_PROPERTY_NAME "sk_manifest"
#define skGetManifestVersionMAN(m) (*(uint32_t*)m)

////////////////////////////////////////////////////////////////////////////////
// Manifest Types
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkManifestMAN);

typedef enum SkManifestEnumerateFlagBitsMAN {
  SK_MANIFEST_ENUMERATE_ALL_BIT = 0x00000001,
  SK_MANIFEST_ENUMERATE_IMPLICIT_BIT = 0x00000002
} SkManifestEnumerateFlagBitsMAN;
typedef SkFlags SkManifestEnumerateFlagsMAN;

typedef struct SkManifestCreateInfoMAN {
  char const*                           pFilepath;
} SkManifestCreateInfoMAN;

typedef struct SkManifestPropertiesMAN {
  char const*                           pFilepath;
  uint32_t                              validLayerCount;
  uint32_t                              validDriverCount;
  uint32_t                              definedLayerCount;
  uint32_t                              definedDriverCount;
} SkManifestPropertiesMAN;

////////////////////////////////////////////////////////////////////////////////
// Manifest Functions
////////////////////////////////////////////////////////////////////////////////

extern SkInternalResult SKAPI_CALL skCreateManifestMAN(
  SkManifestCreateInfoMAN const*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkManifestMAN*                        pManifest
);

extern void SKAPI_CALL skDestroyManifestMAN(
  SkManifestMAN                         manifest,
  SkAllocationCallbacks const*          pAllocator
);

extern void SKAPI_CALL skGetManifestPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestPropertiesMAN*              pProperties
);

extern SkInternalResult SKAPI_CALL skEnumerateManifestDriverPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkDriverProperties*                   pProperties
);

extern SkInternalResult SKAPI_CALL skEnumerateManifestLayerPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkLayerProperties*                    pProperties
);

extern SkInternalResult SKAPI_CALL skInitializeManifestDriverCreateInfoMAN(
  SkManifestMAN                         manifest,
  char const*                           pDriverName,
  SkDriverCreateInfo*                   pCreateInfo
);

extern SkInternalResult SKAPI_CALL skInitializeManifestLayerCreateInfoMAN(
  SkManifestMAN                         manifest,
  char const*                           pLayerName,
  SkLayerCreateInfo*                    pCreateInfo
);

#endif // OPENSK_MAN_MANIFEST_H
