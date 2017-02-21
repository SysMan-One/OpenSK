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
 * Defines the JSON object structure for a 1.0.0 manifest file.
 * For more information, see the manifest_1_0_0.md file in this directory.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/json.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/ext/sk_loader.h>
#include <OpenSK/man/manifest.h>
#include <OpenSK/man/manifest_1_0_0.h>
#include <OpenSK/plt/platform.h>

// C99
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Manifest 1.0.0 Defines
////////////////////////////////////////////////////////////////////////////////

typedef struct SkManifestDriverIMPL {
  SkDriverProperties                    properties;
  uint32_t                              functionMapCount;
  SkLibraryPLT                          library;
  char*                                 pEnableEnvironment;
  char*                                 pDisableEnvironment;
  char*                                 pLibraryPath;
  char*                                 (*pFunctionMap)[2];
} SkManifestDriverIMPL;

typedef struct SkManifestLayerIMPL {
  SkLayerProperties                     properties;
  uint32_t                              functionMapCount;
  SkLibraryPLT                          library;
  char*                                 pEnableEnvironment;
  char*                                 pDisableEnvironment;
  char*                                 pLibraryPath;
  char*                                 (*pFunctionMap)[2];
} SkManifestLayerIMPL;

typedef struct SkManifestMAN_T {
  uint32_t                              manifestVersion;
  uint32_t                              validDriverCount;
  uint32_t                              definedDriverCount;
  uint32_t                              validLayerCount;
  uint32_t                              definedLayerCount;
  char*                                 pFilepath;
  SkManifestDriverIMPL*                 pDriver;
  SkManifestLayerIMPL*                  pLayers;
} SkManifestMAN_T;

typedef SkInternalResult (SKAPI_PTR *PFN_skProcessManifestTypeIMPL)(
  SkManifestMAN                         manifest,
  SkJsonObject                          object,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath
);

////////////////////////////////////////////////////////////////////////////////
// Manifest 1.0.0 Private Functions
////////////////////////////////////////////////////////////////////////////////
static SkInternalResult skProcessLibraryPropertyIMPL(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath,
  SkJsonObject                          object,
  char const*                           pPropertyName,
  char**                                pDestination
) {
  char* pFullLibraryPath;
  char const* pLibraryPath;

  // Get the underlying library name
  if (!skTryGetJsonStringProperty(object, pPropertyName, &pLibraryPath)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Format the library path (if relative, combine with search path)
  if (skIsAbsolutePathPLT(pLibraryPath)) {
    pFullLibraryPath = skAllocate(
      pAllocator,
      strlen(pLibraryPath),
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
  }
  else {
    pFullLibraryPath = skCombinePathsPLT(
      pAllocator,
      pSearchPath,
      pLibraryPath
    );
  }
  if (!pFullLibraryPath) {
    return SKI_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Check that the file actually exists
  if (!skFileExistsPLT(pFullLibraryPath)) {
    skFree(pAllocator, pFullLibraryPath);
    return SKI_ERROR_MANIFEST_DRIVER_NOT_FOUND;
  }

  *pDestination = pFullLibraryPath;
  return SKI_SUCCESS;
}

static void skDestroyManifestFunctionMapIMPL(
  char*                                 (*pFunctionMap)[2],
  uint32_t                              functionMapSize,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  for (idx = 0; idx < functionMapSize; ++idx) {
    skFree(pAllocator, pFunctionMap[idx][0]);
  }
  skFree(pAllocator, pFunctionMap);
}

static SkInternalResult skProcessFunctionMapIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          object,
  char const*                           pPropertyName,
  char*                                 (**pDestination)[2],
  uint32_t*                             pFunctionCount
) {
  uint32_t idx;
  size_t keySize;
  size_t valueSize;
  uint32_t functionCount;
  SkJsonObject functionMap;
  SkJsonProperty functionProperty;
  char const* pFunctionValue;
  char* pFunctionMapEntity;
  char* (*pFunctionMap)[2];

  // Make sure that the function map exists.
  if (!skTryGetJsonObjectProperty(object, pPropertyName, &functionMap)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Allocate the function map for the manifest
  functionCount = skGetJsonObjectPropertyCount(functionMap);
  pFunctionMap = skClearAllocate(
    pAllocator,
    sizeof(char*[2]) * functionCount,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pFunctionMap) {
    return SKI_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Enumerate the functions mapped, add them to the map.
  for (idx = 0; idx < functionCount; ++idx) {

    // Get the key/value pair for the JSON object.
    functionProperty = skGetJsonObjectPropertyElement(functionMap, idx);
    if (!skTryCastJsonString(functionProperty.propertyValue, &pFunctionValue)) {
      skDestroyManifestFunctionMapIMPL(pFunctionMap, functionCount, pAllocator);
      return SKI_ERROR_MANIFEST_UNEXPECTED_TYPE;
    }

    // Get some statistics about the key/value pair.
    keySize = strlen(functionProperty.pPropertyName);
    valueSize = strlen(pFunctionValue);

    // Allocate space for referencing the key/value pair.
    // Note: Additional 2 bytes are required for null-terminator.
    pFunctionMapEntity = skAllocate(
      pAllocator,
      sizeof(char) * (keySize + valueSize + 2),
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pFunctionMapEntity) {
      skDestroyManifestFunctionMapIMPL(pFunctionMap, functionCount, pAllocator);
      return SKI_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Assign our allocated memory to the function map.
    pFunctionMap[idx][0] = pFunctionMapEntity;
    pFunctionMap[idx][1] = &pFunctionMapEntity[keySize + 1];
    strcpy(pFunctionMap[idx][0], functionProperty.pPropertyName);
    strcpy(pFunctionMap[idx][1], pFunctionValue);
  }

  *pDestination = pFunctionMap;
  *pFunctionCount = functionCount;
  return SKI_SUCCESS;
}

static SkInternalResult skProcessStringPropertyIMPL(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  char*                                 pDestination,
  size_t                                maxLength
) {
  size_t stringLength;
  char const* pString;

  // Check for the JSON property
  if (!skTryGetJsonStringProperty(object, pPropertyName, &pString)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Check that the JSON property fits in the destination
  stringLength = strlen(pString);
  if (stringLength >= maxLength) {
    return SKI_ERROR_MANIFEST_OVERFLOW;
  }

  // Copy the string and return
  strcpy(pDestination, pString);
  return SKI_SUCCESS;
}

static SkInternalResult skProcessStringDuplicatorPropertyIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          object,
  char const*                           pPropertyName,
  char**                                pDestination
) {
  char const* pString;
  char* pNewString;

  // Check for the JSON property
  if (!skTryGetJsonStringProperty(object, pPropertyName, &pString)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Check that the JSON property fits in the destination
  pNewString = skAllocate(
    pAllocator,
    strlen(pString) + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pNewString) {
    return SKI_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Copy the string and return
  strcpy(pNewString, pString);
  *pDestination = pNewString;
  return SKI_SUCCESS;
}

static SkInternalResult skProcessVersionPropertyIMPL(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  uint32_t*                             pDestination
) {
  uint32_t major, minor, patch;
  char const* pString;

  // Check for the JSON property
  if (!skTryGetJsonStringProperty(object, pPropertyName, &pString)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Grab the spec version, we must at least read one number.
  major = minor = patch = 0;
  if (!sscanf(pString, "%u.%u.%u", &major, &minor, &patch)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  *pDestination = SK_MAKE_VERSION(major, minor, patch);
  return SKI_SUCCESS;
}

static SkInternalResult skProcessUuidPropertyIMPL(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  uint8_t                               pDestination[SK_UUID_SIZE]
) {
  SkResult result;
  char const* pString;

  // Check for the JSON property
  if (!skTryGetJsonStringProperty(object, pPropertyName, &pString)) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  result = skParseUuid(pString, pDestination);
  if (result != SK_SUCCESS) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  return SKI_SUCCESS;
}

static void skDestroyManifestDriverIMPL(
  SkManifestDriverIMPL*                pDriver,
  SkAllocationCallbacks const*          pAllocator
) {
  skFree(pAllocator, pDriver->pLibraryPath);
  skFree(pAllocator, pDriver->pEnableEnvironment);
  skFree(pAllocator, pDriver->pDisableEnvironment);
  skDestroyManifestFunctionMapIMPL(pDriver->pFunctionMap, pDriver->functionMapCount, pAllocator);
}

static void skDestroyManifestLayerIMPL(
  SkManifestLayerIMPL*                  pLayer,
  SkAllocationCallbacks const*          pAllocator
) {
  skFree(pAllocator, pLayer->pLibraryPath);
  skFree(pAllocator, pLayer->pEnableEnvironment);
  skFree(pAllocator, pLayer->pDisableEnvironment);
  skDestroyManifestFunctionMapIMPL(pLayer->pFunctionMap, pLayer->functionMapCount, pAllocator);
}

#define SK_CHECK(proc) do {  result = proc; if (result != SKI_SUCCESS) { skDestroyManifestDriverIMPL(&reg, pAllocator); return result; } } while (0)
static SkInternalResult SKAPI_CALL skProcessManifestDriverIMPL(
  SkManifestMAN                         manifest,
  SkJsonObject                          object,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath
) {
  SkInternalResult result;
  SkManifestDriverIMPL reg;

  // Configure for dynamic registration
  memset(&reg, 0, sizeof(SkManifestDriverIMPL));
  SK_CHECK(skProcessLibraryPropertyIMPL(pAllocator, pSearchPath, object, "library_path", &reg.pLibraryPath));

  // Process the required driver properties
  SK_CHECK(skProcessUuidPropertyIMPL(object, "uuid", reg.properties.driverUuid));
  SK_CHECK(skProcessStringPropertyIMPL(object, "id", reg.properties.identifier, SK_MAX_IDENTIFIER_SIZE));
  SK_CHECK(skProcessStringPropertyIMPL(object, "name", reg.properties.driverName, SK_MAX_NAME_SIZE));
  SK_CHECK(skProcessStringPropertyIMPL(object, "display_name", reg.properties.displayName, SK_MAX_NAME_SIZE));
  SK_CHECK(skProcessStringPropertyIMPL(object, "description", reg.properties.description, SK_MAX_DESCRIPTION_SIZE));
  SK_CHECK(skProcessVersionPropertyIMPL(object, "api_version", &reg.properties.apiVersion));
  SK_CHECK(skProcessVersionPropertyIMPL(object, "impl_version", &reg.properties.implVersion));
  SK_CHECK(skProcessFunctionMapIMPL(pAllocator, object, "functions", &reg.pFunctionMap, &reg.functionMapCount));

  // At this point, the driver is valid, we should see if it's okay to add.
  if (manifest->validDriverCount >= manifest->definedDriverCount) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Check for implicit flags (these are optional)
  (void)skProcessStringDuplicatorPropertyIMPL(pAllocator, object, "enable_environment", &reg.pEnableEnvironment);
  (void)skProcessStringDuplicatorPropertyIMPL(pAllocator, object, "disable_environment", &reg.pDisableEnvironment);

  // Add the driver to the list of valid drivers in the manifest.
  manifest->pDriver[manifest->validDriverCount] = reg;
  ++manifest->validDriverCount;

  return SKI_SUCCESS;
}
#undef SK_CHECK

#define SK_CHECK(proc) do {  result = proc; if (result != SKI_SUCCESS) { skDestroyManifestLayerIMPL(&reg, pAllocator); return result; } } while (0)
static SkInternalResult SKAPI_CALL skProcessManifestLayerIMPL(
  SkManifestMAN                         manifest,
  SkJsonObject                          object,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath
) {
  SkInternalResult result;
  SkManifestLayerIMPL reg;

  // Configure for dynamic registration
  memset(&reg, 0, sizeof(SkManifestLayerIMPL));
  SK_CHECK(skProcessLibraryPropertyIMPL(pAllocator, pSearchPath, object, "library_path", &reg.pLibraryPath));

  // Process the required driver properties
  SK_CHECK(skProcessUuidPropertyIMPL(object, "uuid", reg.properties.layerUuid));
  SK_CHECK(skProcessStringPropertyIMPL(object, "name", reg.properties.layerName, SK_MAX_NAME_SIZE));
  SK_CHECK(skProcessStringPropertyIMPL(object, "display_name", reg.properties.displayName, SK_MAX_NAME_SIZE));
  SK_CHECK(skProcessStringPropertyIMPL(object, "description", reg.properties.description, SK_MAX_DESCRIPTION_SIZE));
  SK_CHECK(skProcessVersionPropertyIMPL(object, "api_version", &reg.properties.apiVersion));
  SK_CHECK(skProcessVersionPropertyIMPL(object, "impl_version", &reg.properties.implVersion));
  SK_CHECK(skProcessFunctionMapIMPL(pAllocator, object, "functions", &reg.pFunctionMap, &reg.functionMapCount));

  // At this point, the driver is valid, we should see if it's okay to add.
  if (manifest->validLayerCount >= manifest->definedLayerCount) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // Check for implicit flags (these are optional)
  (void)skProcessStringDuplicatorPropertyIMPL(pAllocator, object, "enable_environment", &reg.pEnableEnvironment);
  (void)skProcessStringDuplicatorPropertyIMPL(pAllocator, object, "disable_environment", &reg.pDisableEnvironment);

  // Add the driver to the list of valid drivers in the manifest.
  manifest->pLayers[manifest->validLayerCount] = reg;
  ++manifest->validLayerCount;

  return SKI_SUCCESS;
}
#undef SK_CHECK

static SkInternalResult skProcessManifestObjectIMPL(
  SkManifestMAN                         manifest,
  SkJsonObject                          object,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath,
  PFN_skProcessManifestTypeIMPL         pfnProcessManifestType
) {
  return pfnProcessManifestType(manifest, object, pAllocator, pSearchPath);
}

static SkInternalResult skProcessManifestArrayIMPL(
  SkManifestMAN                         manifest,
  SkJsonArray                           array,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath,
  PFN_skProcessManifestTypeIMPL         pfnProcessManifestType
) {
  uint32_t idx;
  uint32_t length;
  SkJsonObject object;
  SkInternalResult result;

  // Process all of the array elements
  length = skGetJsonArrayLength(array);
  for (idx = 0; idx < length; ++idx) {

    // Check that the next element is an object, as expected
    if (!skTryGetJsonObjectElement(array, idx, &object)) {
      return SKI_ERROR_MANIFEST_UNEXPECTED_TYPE;
    }

    // Process the object and make sure no errors occurred
    result = skProcessManifestObjectIMPL(
      manifest, object, pAllocator, pSearchPath, pfnProcessManifestType
    );
    if (result != SKI_SUCCESS) {
      return result;
    }

  }

  return SKI_SUCCESS;
}

static SkInternalResult skProcessManifestValueIMPL(
  SkManifestMAN                         manifest,
  SkJsonValue                           value,
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pSearchPath,
  PFN_skProcessManifestTypeIMPL         pfnProcessManifestType
) {
  switch (skGetJsonType(value)) {
    case SK_JSON_TYPE_OBJECT:
      return skProcessManifestObjectIMPL(
        manifest, skCastJsonObject(value), pAllocator, pSearchPath, pfnProcessManifestType
      );
    case SK_JSON_TYPE_ARRAY:
      return skProcessManifestArrayIMPL(
        manifest, skCastJsonArray(value), pAllocator, pSearchPath, pfnProcessManifestType
      );
    default:
      return SKI_ERROR_MANIFEST_UNEXPECTED_TYPE;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Manifest 1.0.0 Functions
////////////////////////////////////////////////////////////////////////////////

SkInternalResult SKAPI_CALL skCreateManifestMAN_1_0_0(
  SkManifestCreateInfoMAN const*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          manifestJson,
  SkManifestMAN*                        pManifest
) {
  char* pSearchPath;
  uint32_t driverCount;
  uint32_t layerCount;
  SkJsonValue driverValue;
  SkJsonValue layerValue;
  SkManifestMAN manifest;
  SkInternalResult result;

  // Check to see that the driver object/array is present and valid
  driverCount = 0;
  if (skTryGetJsonValueProperty(manifestJson, "driver", &driverValue)) {
    if (skGetJsonType(driverValue) != SK_JSON_TYPE_OBJECT) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    if (skGetJsonValueProperty(manifestJson, "drivers")) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    driverCount = 1;
  }
  else if (skTryGetJsonValueProperty(manifestJson, "drivers", &driverValue)) {
    if (skGetJsonType(driverValue) != SK_JSON_TYPE_ARRAY) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    driverCount = skGetJsonArrayLength(skCastJsonArray(driverValue));
  }

  // Check to see that the layer object/array is present and valid
  layerCount = 0;
  if (skTryGetJsonValueProperty(manifestJson, "layer", &layerValue)) {
    if (skGetJsonType(layerValue) != SK_JSON_TYPE_OBJECT) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    if (skGetJsonValueProperty(manifestJson, "layers")) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    layerCount = 1;
  }
  else if (skTryGetJsonValueProperty(manifestJson, "layers", &layerValue)) {
    if (skGetJsonType(layerValue) != SK_JSON_TYPE_ARRAY) {
      return SKI_ERROR_MANIFEST_INVALID;
    }
    layerCount = skGetJsonArrayLength(skCastJsonArray(layerValue));
  }

  // Construct a "search path", which will be used for testing for files.
  // Note: it's valid for this to be null (in the case of relative paths).
  pSearchPath = skRemovePathStemPLT(
    pAllocator,
    pCreateInfo->pFilepath
  );

  // Allocate the manifest object and information required for properties.
  // Note: Manifests are always created for the loader, so we can assume that.
  manifest = skClearAllocate(
    pAllocator,
    sizeof(SkManifestMAN_T) +
    sizeof(SkManifestDriverIMPL) * driverCount +
    sizeof(SkManifestLayerIMPL) * layerCount +
    strlen(pCreateInfo->pFilepath) + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!manifest) {
    return SKI_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the manifest object
  manifest->manifestVersion = SK_MAKE_VERSION(1, 0, 0);
  manifest->definedDriverCount = driverCount;
  manifest->definedLayerCount = layerCount;
  manifest->pDriver = (SkManifestDriverIMPL*)&manifest[1];
  manifest->pLayers = (SkManifestLayerIMPL*)&manifest->pDriver[driverCount];
  manifest->pFilepath = (char*)&manifest->pLayers[layerCount];
  strcpy(manifest->pFilepath, pCreateInfo->pFilepath);

  // If the driver object/array is present, process it
  if (driverCount) {
    result = skProcessManifestValueIMPL(
      manifest, driverValue, pAllocator, pSearchPath, &skProcessManifestDriverIMPL
    );
    if (result < SKI_SUCCESS) {
      skFree(pAllocator, pSearchPath);
      skDestroyManifestMAN_1_0_0(manifest, pAllocator);
      return result;
    }
  }
  if (layerCount) {
    result = skProcessManifestValueIMPL(
      manifest, layerValue, pAllocator, pSearchPath, &skProcessManifestLayerIMPL
    );
    if (result < SKI_SUCCESS) {
      skFree(pAllocator, pSearchPath);
      skDestroyManifestMAN_1_0_0(manifest, pAllocator);
      return result;
    }
  }

  skFree(pAllocator, pSearchPath);
  *pManifest = manifest;
  return SKI_SUCCESS;
}

void SKAPI_CALL skDestroyManifestMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  for (idx = 0; idx < manifest->validDriverCount; ++idx) {
    skDestroyManifestDriverIMPL(&manifest->pDriver[idx], pAllocator);
  }
  for (idx = 0; idx < manifest->validLayerCount; ++idx) {
    skDestroyManifestLayerIMPL(&manifest->pLayers[idx], pAllocator);
  }
  skFree(pAllocator, manifest);
}

void SKAPI_CALL skGetManifestPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestPropertiesMAN*              pProperties
) {
  memset(pProperties, 0, sizeof(SkManifestPropertiesMAN));
  pProperties->pFilepath = manifest->pFilepath;
  pProperties->validLayerCount = manifest->validLayerCount;
  pProperties->validDriverCount = manifest->validDriverCount;
  pProperties->definedLayerCount = manifest->definedLayerCount;
  pProperties->definedDriverCount = manifest->definedDriverCount;
}

SkInternalResult SKAPI_CALL skEnumerateManifestDriverPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkDriverProperties*                   pProperties
) {
  uint32_t idx;
  uint32_t count;
  SkManifestDriverIMPL* pDriver;

  count = 0;
  for (idx = 0; idx < manifest->validDriverCount; ++idx) {
    pDriver = &manifest->pDriver[idx];

    // Check to see if the driver is active (via. environment variables)
    if (enumerateFlags & SK_MANIFEST_ENUMERATE_IMPLICIT_BIT) {
      if (pDriver->pDisableEnvironment && getenv(pDriver->pDisableEnvironment)) {
        continue;
      }
      if (pDriver->pEnableEnvironment && !getenv(pDriver->pEnableEnvironment)) {
        continue;
      }
    }

    // The driver is active, see if we can return it.
    if (pProperties) {
      if (count >= *pCreateInfoCount) {
        return SKI_INCOMPLETE;
      }
      pProperties[count] = pDriver->properties;
    }
    ++count;
  }

  *pCreateInfoCount = count;
  return SKI_SUCCESS;
}

SkInternalResult SKAPI_CALL skEnumerateManifestLayerPropertiesMAN_1_0_0(
  SkManifestMAN                         manifest,
  SkManifestEnumerateFlagsMAN           enumerateFlags,
  uint32_t*                             pCreateInfoCount,
  SkLayerProperties*                    pProperties
) {
  uint32_t idx;
  uint32_t count;
  SkManifestLayerIMPL* pLayer;

  count = 0;
  for (idx = 0; idx < manifest->validLayerCount; ++idx) {
    pLayer = &manifest->pLayers[idx];

    // Check to see if the driver is active (via. environment variables)
    if (enumerateFlags & SK_MANIFEST_ENUMERATE_IMPLICIT_BIT) {
      if (pLayer->pDisableEnvironment && getenv(pLayer->pDisableEnvironment)) {
        continue;
      }
      if (pLayer->pEnableEnvironment && !getenv(pLayer->pEnableEnvironment)) {
        continue;
      }
    }

    // The driver is active, see if we can return it.
    if (pProperties) {
      if (count >= *pCreateInfoCount) {
        return SKI_INCOMPLETE;
      }
      pProperties[count] = pLayer->properties;
    }
    ++count;
  }

  *pCreateInfoCount = count;
  return SKI_SUCCESS;
}

static PFN_skVoidFunction skGetMappedFunction(
  SkLibraryPLT                          library,
  PFN_skVoidFunction                    pfnExcludeFunction,
  char*                                 (*pFunctionMap)[2],
  uint32_t                              functionMapCount,
  char const*                           pName
) {
  uint32_t idx;
  PFN_skVoidFunction pfnVoidFunction;

  // See if we should map this function to a different name.
  for (idx = 0; idx < functionMapCount; ++idx) {
    if (strcmp(pFunctionMap[idx][0], pName) == 0) {
      pName = pFunctionMap[idx][1];
      break;
    }
  }

  // Get the function requested from the library
  pfnVoidFunction = skGetLibraryProcAddrPLT(
    library,
    pName
  );
  if (pfnVoidFunction == pfnExcludeFunction) {
    pfnVoidFunction = NULL;
  }
  return pfnVoidFunction;
}

SkInternalResult SKAPI_CALL skInitializeManifestDriverCreateInfoMAN_1_0_0(
  SkManifestMAN                         manifest,
  char const*                           pDriverName,
  SkDriverCreateInfo*                   pCreateInfo
) {
  uint32_t idx;
  SkInternalResult result;
  SkManifestDriverIMPL* pDriver;
  PFN_skGetDriverProcAddr pfnGetProcAddr;

  // Find the driver for the given manifest - each manifest should have only
  // one uniquely defined constructable object.
  pDriver = NULL;
  for (idx = 0; idx < manifest->definedDriverCount; ++idx) {
    if (strcmp(manifest->pDriver[idx].properties.driverName, pDriverName) == 0) {
      pDriver = &manifest->pDriver[idx];
      break;
    }
  }
  if (!pDriver) {
    return SKI_ERROR_MANIFEST_DRIVER_NOT_FOUND;
  }

  // Otherwise, we should attempt to initialize the driver.
  if (!pDriver->library) {
    result = skLoadLibraryPLT(
      pDriver->pLibraryPath,
      &pDriver->library
    );
    if (result != SKI_SUCCESS) {
      return result;
    }
  }

  // If we've gotten this far, we should check to see if the user wants create-info.
  if (!pCreateInfo) {
    return SKI_SUCCESS;
  }

  // Find the function name for skGetProcAddr
  pfnGetProcAddr = (PFN_skGetDriverProcAddr)skGetMappedFunction(
    pDriver->library,
    (PFN_skVoidFunction)&skGetDriverProcAddr,
    pDriver->pFunctionMap,
    pDriver->functionMapCount,
    "skGetDriverProcAddr"
  );
  if (!pfnGetProcAddr) {
    return SKI_ERROR_MANIFEST_INVALID;
  }

  // After initialization, grab the function for skGetProcAddr.
  pCreateInfo->sType = SK_STRUCTURE_TYPE_DRIVER_CREATE_INFO;
  pCreateInfo->pNext = NULL;
  pCreateInfo->properties = pDriver->properties;
  pCreateInfo->pfnGetDriverProcAddr = pfnGetProcAddr;
  return SKI_SUCCESS;
}

SkInternalResult SKAPI_CALL skInitializeManifestLayerCreateInfoMAN_1_0_0(
  SkManifestMAN                         manifest,
  char const*                           pLayerName,
  SkLayerCreateInfo*                    pCreateInfo
) {
  uint32_t idx;
  SkInternalResult result;
  SkManifestLayerIMPL* pLayer;

  // Find the driver for the given manifest - each manifest should have only
  // one uniquely defined constructable object.
  pLayer = NULL;
  for (idx = 0; idx < manifest->definedLayerCount; ++idx) {
    if (strcmp(manifest->pLayers[idx].properties.layerName, pLayerName) == 0) {
      pLayer = &manifest->pLayers[idx];
      break;
    }
  }
  if (!pLayer) {
    return SKI_ERROR_MANIFEST_LAYER_NOT_FOUND;
  }

  // Otherwise, we should attempt to initialize the driver.
  if (!pLayer->library) {
    result = skLoadLibraryPLT(
      pLayer->pLibraryPath,
      &pLayer->library
    );
    if (result != SKI_SUCCESS) {
      return result;
    }
  }

  // If we've gotten this far, we should check to see if the user wants create-info.
  if (!pCreateInfo) {
    return SKI_SUCCESS;
  }

  // After initialization, grab the functions for skGet*ProcAddr.
  // Note: We do not need to check the results for these proc addr calls.
  //       They do not need to pass because layers can only implement parts.
  pCreateInfo->sType = SK_STRUCTURE_TYPE_LAYER_CREATE_INFO;
  pCreateInfo->pNext = NULL;
  pCreateInfo->properties = pLayer->properties;
  pCreateInfo->pfnGetInstanceProcAddr = (PFN_skGetInstanceProcAddr)skGetMappedFunction(
    pLayer->library,
    (PFN_skVoidFunction)&skGetInstanceProcAddr,
    pLayer->pFunctionMap,
    pLayer->functionMapCount,
    "skGetInstanceProcAddr"
  );
  pCreateInfo->pfnGetDriverProcAddr = (PFN_skGetDriverProcAddr)skGetMappedFunction(
    pLayer->library,
    (PFN_skVoidFunction)&skGetDriverProcAddr,
    pLayer->pFunctionMap,
    pLayer->functionMapCount,
    "skGetDriverProcAddr"
  );
  pCreateInfo->pfnGetPcmStreamProcAddr = (PFN_skGetPcmStreamProcAddr)skGetMappedFunction(
    pLayer->library,
    (PFN_skVoidFunction)&skGetPcmStreamProcAddr,
    pLayer->pFunctionMap,
    pLayer->functionMapCount,
    "skGetPcmStreamProcAddr"
  );

  return SKI_SUCCESS;
}
