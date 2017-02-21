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
 * The loader is a static object which aggregates all of the information for
 * constructing an OpenSK instance. Anything that happens pre-instance is
 * usually done through the loader.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/json.h>
#include <OpenSK/dev/string.h>
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/ext/sk_loader.h>
#include <OpenSK/man/manifest.h>
#include <OpenSK/plt/platform.h>

// C99
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Private Types
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_VECTOR_IMPL(ManifestVector, SkManifestMAN);

typedef struct SkLoader_T {
  SkAllocationCallbacks const*          pAllocator;
  SkBool32                              isInitializing;
  SkBool32                              isInitialized;
  SkPlatformPLT                         platform;
  SkStringVectorIMPL_T                  searchPaths;
  SkManifestVectorIMPL_T                manifests;
} SkLoader_T;

static SkLoader_T skDefaultLoaderIMPL = { NULL };

////////////////////////////////////////////////////////////////////////////////
// Private Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SK_DEFINE_VECTOR_PUSH_METHOD_IMPL(ManifestVector, SkManifestMAN)

static SkResult skProcessManifestFileIMPL(
  SkLoader                              loader,
  char const*                           pManifestFile
) {
  SkResult result;
  SkManifestMAN manifest;
  SkInternalResult iresult;
  SkManifestCreateInfoMAN createInfo;

  // Attempt to parse the manifest file.
  createInfo.pFilepath = pManifestFile;
  iresult = skCreateManifestMAN(
    &createInfo,
    loader->pAllocator,
    &manifest
  );
  if (iresult != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Store the manifest in the loader
  result = skPushManifestVectorIMPL(
    loader->pAllocator,
    &loader->manifests,
    &manifest
  );
  if (result != SK_SUCCESS) {
    skDestroyManifestMAN(manifest, loader->pAllocator);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  return SK_SUCCESS;
}

static SkResult skInitializeLoader_InternalIMPL(
  SkLoader                              loader
) {
  uint32_t idx;
  SkResult result;
  uint32_t manifestCount;
  char const** ppManifests;

  // Initialize the platform
  result = skCreatePlatformPLT(
    loader->pAllocator,
    &loader->platform
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  // Count the number of manifests on the platform.
  result = skEnumeratePlatformManifestsPLT(
    loader->platform,
    &manifestCount,
    NULL
  );
  if (result != SK_SUCCESS) {
    skDestroyPlatformPLT(loader->pAllocator, loader->platform);
    return result;
  }

  // Construct an array of possible manifests
  ppManifests = skAllocate(
    loader->pAllocator,
    sizeof(char const*) * manifestCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!ppManifests) {
    skDestroyPlatformPLT(loader->pAllocator, loader->platform);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Enumerate the manifests on the platform.
  result = skEnumeratePlatformManifestsPLT(
    loader->platform,
    &manifestCount,
    ppManifests
  );
  if (result < SK_SUCCESS) {
    skFree(loader->pAllocator, ppManifests);
    skDestroyPlatformPLT(loader->pAllocator, loader->platform);
    return result;
  }

  // Start processing all of the manifest files
  for (idx = 0; idx < manifestCount; ++idx) {
    result = skProcessManifestFileIMPL(
      loader,
      ppManifests[idx]
    );
    if (result != SK_SUCCESS) {
      continue;
    }
  }

  skFree(loader->pAllocator, ppManifests);

  return SK_SUCCESS;
}

static SkResult skInitializeLoaderIMPL(
  SkLoader                              loader
) {
  SkResult result;

  // Skip initialization if it already happened or is happening.
  if (loader->isInitialized || loader->isInitializing) {
    return SK_SUCCESS;
  }

  // Actually initialize the loader
  loader->isInitializing = SK_TRUE;
  loader->manifests.allocationScope = SK_SYSTEM_ALLOCATION_SCOPE_LOADER;
  loader->searchPaths.allocationScope = SK_SYSTEM_ALLOCATION_SCOPE_LOADER;
  result = skInitializeLoader_InternalIMPL(loader);
  loader->isInitializing = SK_FALSE;
  if (result != SK_SUCCESS) {
    return result;
  }
  loader->isInitialized = SK_TRUE;

  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// API Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skInitializeLoader() {
  SkInternalResult result;
  result = skInitializeLoaderIMPL(&skDefaultLoaderIMPL);
  if (result != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDeinitializeLoader() {
  uint32_t idx;
  if (skDefaultLoaderIMPL.isInitialized) {

    // Delete all of the search path objects
    for (idx = 0; idx < skDefaultLoaderIMPL.searchPaths.count; ++idx) {
      skFree(skDefaultLoaderIMPL.pAllocator, skDefaultLoaderIMPL.searchPaths.pData[idx]);
    }
    skFree(skDefaultLoaderIMPL.pAllocator, skDefaultLoaderIMPL.searchPaths.pData);

    // Delete all of the internal manifests
    for (idx = 0; idx < skDefaultLoaderIMPL.manifests.count; ++idx) {
      skDestroyManifestMAN(skDefaultLoaderIMPL.manifests.pData[idx], skDefaultLoaderIMPL.pAllocator);
    }
    skFree(skDefaultLoaderIMPL.pAllocator, skDefaultLoaderIMPL.manifests.pData);

    // Destroy the platform object and zero-out the entire loader.
    skDestroyPlatformPLT(skDefaultLoaderIMPL.pAllocator, skDefaultLoaderIMPL.platform);
    memset(&skDefaultLoaderIMPL, 0, sizeof(SkLoader_T));
  }
}

SKAPI_ATTR SkResult SKAPI_CALL skSetLoaderAllocator(
  SkAllocationCallbacks const*          pAllocator
) {
  if (skDefaultLoaderIMPL.isInitialized) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }
  skDefaultLoaderIMPL.pAllocator = pAllocator;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializeLayerCreateInfo(
  char const*                           pLayerName,
  uint32_t*                             pCreateInfos,
  SkLayerCreateInfo*                    pCreateInfo
) {
  uint32_t idx;
  uint32_t count;
  SkInternalResult result;

  // Initialize the loader (lazy initialization)
  result = skInitializeLoaderIMPL(&skDefaultLoaderIMPL);
  if (result != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Count the number of layers available for the requested host.
  // Note: There can be multiple drivers on a single system. When this
  //       happens, we will create a create-info for each, and attempt
  //       to construct them until one of them constructs properly.
  count = 0;
  for (idx = 0; idx < skDefaultLoaderIMPL.manifests.count; ++idx) {
    result = skInitializeManifestLayerCreateInfoMAN(
      skDefaultLoaderIMPL.manifests.pData[idx],
      pLayerName,
      (pCreateInfo) ? &pCreateInfo[count] : NULL
    );
    if (result == SKI_SUCCESS) {
      ++count;
    }
  }

  *pCreateInfos = count;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skInitializeDriverCreateInfo(
  char const*                           pDriverName,
  uint32_t*                             pCreateInfos,
  SkDriverCreateInfo*                   pCreateInfo
) {
  uint32_t idx;
  uint32_t count;
  SkInternalResult result;

  // Initialize the loader (lazy initialization)
  result = skInitializeLoaderIMPL(&skDefaultLoaderIMPL);
  if (result != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Count the number of drivers available for the requested host.
  // Note: There can be multiple drivers on a single system. When this
  //       happens, we will create a create-info for each, and attempt
  //       to construct them until one of them constructs properly.
  count = 0;
  for (idx = 0; idx < skDefaultLoaderIMPL.manifests.count; ++idx) {
    result = skInitializeManifestDriverCreateInfoMAN(
      skDefaultLoaderIMPL.manifests.pData[idx],
      pDriverName,
      (pCreateInfo) ? &pCreateInfo[count] : NULL
    );
    if (result == SKI_SUCCESS) {
      ++count;
    }
  }

  *pCreateInfos = count;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLoaderLayerProperties(
  SkLoaderEnumerateFlags                enumerateFlags,
  uint32_t*                             pLayerCount,
  SkLayerProperties*                    pProperties
) {
  uint32_t idx;
  uint32_t count;
  SkManifestMAN manifest;
  uint32_t manifestCount;
  SkInternalResult result;
  SkManifestEnumerateFlagsMAN manifestFlags;

  // Initialize the loader (lazy initialization)
  result = skInitializeLoaderIMPL(&skDefaultLoaderIMPL);
  if (result != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Convert the enumerate flag to the manifest flags
  manifestFlags = 0;
  if (enumerateFlags & SK_LOADER_ENUMERATE_ALL_BIT) {
    manifestFlags |= SK_MANIFEST_ENUMERATE_ALL_BIT;
  }
  if (enumerateFlags & SK_LOADER_ENUMERATE_IMPLICIT_BIT) {
    manifestFlags |= SK_MANIFEST_ENUMERATE_IMPLICIT_BIT;
  }

  // Search for the layer in question
  count = 0;
  for (idx = 0; idx < skDefaultLoaderIMPL.manifests.count; ++idx) {
    manifest = skDefaultLoaderIMPL.manifests.pData[idx];
    manifestCount = *pLayerCount - count;
    result = skEnumerateManifestLayerPropertiesMAN(
      manifest,
      manifestFlags,
      &manifestCount,
      (pProperties) ? &pProperties[count] : NULL
    );
    if (result != SKI_SUCCESS) {
      return SK_INCOMPLETE;
    }
    count += manifestCount;
  }

  *pLayerCount = count;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateLoaderDriverProperties(
  SkLoaderEnumerateFlags                enumerateFlags,
  uint32_t*                             pDriverCount,
  SkDriverProperties*                   pProperties
) {
  uint32_t idx;
  uint32_t count;
  SkManifestMAN manifest;
  uint32_t manifestCount;
  SkInternalResult result;
  SkManifestEnumerateFlagsMAN manifestFlags;

  // Initialize the loader (lazy initialization)
  result = skInitializeLoaderIMPL(&skDefaultLoaderIMPL);
  if (result != SKI_SUCCESS) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }

  // Convert the enumerate flag to the manifest flags
  manifestFlags = 0;
  if (enumerateFlags & SK_LOADER_ENUMERATE_ALL_BIT) {
    manifestFlags |= SK_MANIFEST_ENUMERATE_ALL_BIT;
  }
  if (enumerateFlags & SK_LOADER_ENUMERATE_IMPLICIT_BIT) {
    manifestFlags |= SK_MANIFEST_ENUMERATE_IMPLICIT_BIT;
  }

  // Search for the driver in question
  count = 0;
  for (idx = 0; idx < skDefaultLoaderIMPL.manifests.count; ++idx) {
    manifest = skDefaultLoaderIMPL.manifests.pData[idx];
    manifestCount = *pDriverCount - count;
    result = skEnumerateManifestDriverPropertiesMAN(
      manifest,
      manifestFlags,
      &manifestCount,
      (pProperties) ? &pProperties[count] : NULL
    );
    if (result != SKI_SUCCESS) {
      return SK_INCOMPLETE;
    }
    count += manifestCount;
  }

  *pDriverCount = count;
  return SK_SUCCESS;
}