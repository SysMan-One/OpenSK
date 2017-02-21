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
#ifndef   OPENSK_MAN_MANIFEST_1_0_0_H
#define   OPENSK_MAN_MANIFEST_1_0_0_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/dev/json.h>
#include <OpenSK/man/manifest.h>

////////////////////////////////////////////////////////////////////////////////
// Manifest 1.0.0 Defines
////////////////////////////////////////////////////////////////////////////////

#define SK_MANIFEST_VERSION_1_0_0 "1.0.0"

////////////////////////////////////////////////////////////////////////////////
// Manifest 1.0.0 Functions
////////////////////////////////////////////////////////////////////////////////

extern SkInternalResult SKAPI_CALL skCreateManifestMAN_1_0_0(
  SkManifestCreateInfoMAN const*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          manifestJson,
  SkManifestMAN*                        pManifest
);

extern void SKAPI_CALL skDestroyManifestMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkAllocationCallbacks const*          pAllocator
);

extern void SKAPI_CALL skGetManifestPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestPropertiesMAN*              pProperties
);

extern SkInternalResult SKAPI_CALL skEnumerateManifestDriverPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkDriverProperties*                   pProperties
);

extern SkInternalResult SKAPI_CALL skEnumerateManifestLayerPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkLayerProperties*                    pProperties
);

extern SkInternalResult SKAPI_CALL skInitializeManifestDriverCreateInfoMAN_1_0_0(
  SkManifestMAN                         manifest,
  char const*                           pDriverName,
  SkDriverCreateInfo*                   pCreateInfo
);


extern SkInternalResult SKAPI_CALL skInitializeManifestLayerCreateInfoMAN_1_0_0(
  SkManifestMAN                         manifest,
  char const*                           pLayerName,
  SkLayerCreateInfo*                    pCreateInfo
);

#endif // OPENSK_MAN_MANIFEST_1_0_0_H
