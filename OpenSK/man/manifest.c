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
 * Abstract manifest type for handling different manifest versions.
 ******************************************************************************/

#include <OpenSK/man/manifest.h>
#include <OpenSK/man/manifest_1_0_0.h>

SkInternalResult SKAPI_CALL skCreateManifestMAN(
  SkManifestCreateInfoMAN const*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkManifestMAN*                        pManifest
) {
  SkInternalResult result;
  char const* pVersionString;
  SkJsonObject manifestJson;

  // Parse the manifest file
  result = skCreateJsonObjectFromFile(
    pCreateInfo->pFilepath,
    pAllocator,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND,
    &manifestJson
  );
  if (result != SKI_SUCCESS) {
    return result;
  }

  // Check that the manifest version property exists
  if (!skTryGetJsonStringProperty(manifestJson, SK_MANIFEST_VERSION_PROPERTY_NAME, &pVersionString)) {
    skDestroyJsonObject(pAllocator, manifestJson);
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Determine the proper manifest parser by version
  result = SKI_ERROR_MANIFEST_UNSUPPORTED;
  if (strcmp(pVersionString, SK_MANIFEST_VERSION_1_0_0) == 0) {
    result = skCreateManifestMAN_1_0_0(
      pCreateInfo,
      pAllocator,
      manifestJson,
      pManifest
    );
  }

  // Return the manifest object
  skDestroyJsonObject(pAllocator, manifestJson);
  return result;
}

void SKAPI_CALL skDestroyManifestMAN(
  SkManifestMAN                         manifest,
  SkAllocationCallbacks const*          pAllocator
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      skDestroyManifestMAN_1_0_0(manifest, pAllocator);
      break;
    default:
      // Invalid manifest
      break;
  }
}

void SKAPI_CALL skGetManifestPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestPropertiesMAN*              pProperties
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      skGetManifestPropertiesMAN_1_0_0(manifest, pProperties);
      break;
    default:
      // Invalid manifest
      break;
  }
}

SkInternalResult SKAPI_CALL skEnumerateManifestDriverPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkDriverProperties*                  pProperties
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      return skEnumerateManifestDriverPropertiesMAN_1_0_0(
        manifest,
        enumerateFlags,
        pCreateInfoCount,
        pProperties
      );
    default:
      return SKI_ERROR_MANIFEST_INVALID;
  }
}

SkInternalResult SKAPI_CALL skEnumerateManifestLayerPropertiesMAN(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkLayerProperties*                    pProperties
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      return skEnumerateManifestLayerPropertiesMAN_1_0_0(
        manifest,
        enumerateFlags,
        pCreateInfoCount,
        pProperties
      );
    default:
      return SKI_ERROR_MANIFEST_INVALID;
  }
}

SkInternalResult SKAPI_CALL skInitializeManifestDriverCreateInfoMAN(
  SkManifestMAN                         manifest,
  char const*                           pDriverName,
  SkDriverCreateInfo*                  pCreateInfo
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      return skInitializeManifestDriverCreateInfoMAN_1_0_0(
        manifest,
        pDriverName,
        pCreateInfo
      );
    default:
      return SKI_ERROR_MANIFEST_INVALID;
  }
}

SkInternalResult SKAPI_CALL skInitializeManifestLayerCreateInfoMAN(
  SkManifestMAN                         manifest,
  char const*                           pLayerName,
  SkLayerCreateInfo*                    pCreateInfo
) {
  switch (skGetManifestVersionMAN(manifest)) {
    case SK_MAKE_VERSION(1, 0, 0):
      return skInitializeManifestLayerCreateInfoMAN_1_0_0(
        manifest,
        pLayerName,
        pCreateInfo
      );
    default:
      return SKI_ERROR_MANIFEST_INVALID;
  }
}
