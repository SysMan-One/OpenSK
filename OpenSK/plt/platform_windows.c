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
 * Implementation of the Windows platform-specific functionality.
 * This may or may not get compiled in, depending on the target platform.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/string.h>
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/plt/platform.h>

// C99
#include <ctype.h>
#include <string.h>

// Non-Standard
#include <Windows.h>
#include <Shlobj.h>
#include <wchar.h>
#include <Lmcons.h>

////////////////////////////////////////////////////////////////////////////////
// Windows Platform Globals
////////////////////////////////////////////////////////////////////////////////

typedef struct SkRegistryKeyIMPL {
  HKEY                                  hkey;
  char const*                           pSubKey;
} SkRegistryKeyIMPL;

static SkRegistryKeyIMPL skRegistryKeysIMPL[] = {
  { HKEY_LOCAL_MACHINE, "SOFTWARE\\OpenSK\\ExplicitLayers" },
  { HKEY_LOCAL_MACHINE, "SOFTWARE\\OpenSK\\ImplicitLayers" },
  { HKEY_LOCAL_MACHINE, "SOFTWARE\\OpenSK\\Drivers" },
  { HKEY_CURRENT_USER, "SOFTWARE\\OpenSK\\ExplicitLayers" },
  { HKEY_CURRENT_USER, "SOFTWARE\\OpenSK\\ImplicitLayers" },
  { HKEY_CURRENT_USER, "SOFTWARE\\OpenSK\\Drivers" }
};

////////////////////////////////////////////////////////////////////////////////
// Windows Platform Defines
////////////////////////////////////////////////////////////////////////////////

#define REGISTRY_KEY_COUNT                                                      \
(sizeof(skRegistryKeysIMPL) / sizeof(skRegistryKeysIMPL[0]))
#define SK_HOSTNAME_MAX 256

////////////////////////////////////////////////////////////////////////////////
// Windows Platform Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkPlatformPLT_T {
  SkAllocationCallbacks const*          pAllocator;
  SkStringVectorIMPL_T                  searchPaths;
  SkStringVectorIMPL_T                  manifestPaths;
  SkStringVectorIMPL_T                  environmentPaths;
  char*                                 pHomeDirectory;
  char*                                 pCurrentDirectory;
  char*                                 pExecutablePath;
  char const*                           pExecutableFilename;
  char                                  pHostname[MAX_COMPUTERNAME_LENGTH + 1];
  char                                  pUsername[UNLEN + 1];
} SkPlatformPLT_T;

////////////////////////////////////////////////////////////////////////////////
// Windows Platform Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkResult skReadRegisteryKeyIMPL(
  SkPlatformPLT                         platform,
  HKEY                                  registryKey
) {
  uint32_t idx;
  HRESULT result;
  LSTATUS status;
  char *pValueName;
  char *pNewValueName;
  DWORD value;
  DWORD valueType;
  DWORD valueSize;
  DWORD valueCount;
  DWORD curValueLength;
  DWORD maxValueLength;
  uint32_t valueNameCapacity;

  // Set the defaults for the rest of the function to use.
  valueSize = sizeof(DWORD);
  pValueName = NULL;
  valueNameCapacity = 0;

  // Grab the number of values in the registry key
  status = RegQueryInfoKeyA(
    registryKey,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &valueCount,
    &maxValueLength,
    NULL,
    NULL,
    NULL
  );
  if (status != ERROR_SUCCESS) {
    return SK_SUCCESS;
  }

  // If this key doesn't contain any values, skip it.
  if (!valueCount) {
    return SK_SUCCESS;
  }

  // Allocate the name array so that we can hold all value names.
  pNewValueName = skAllocate(
    platform->pAllocator,
    maxValueLength + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_COMMAND
  );
  if (!pNewValueName) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  valueNameCapacity = maxValueLength + 1;
  pValueName = pNewValueName;

  // Read the values for the registry key.
  for (idx = 0; idx < valueCount; ++idx) {

    // Grab the value, along with it's name, from the current key.
    curValueLength = valueNameCapacity;
    status = RegEnumValueA(
      registryKey,
      idx,
      pValueName,
      &curValueLength,
      NULL,
      &valueType,
      (LPBYTE)&value,
      &valueSize
    );
    if (status != ERROR_SUCCESS) {
      continue;
    }

    // Check that a DWORD of some type is returned.
    if (valueType != REG_DWORD_BIG_ENDIAN && 
        valueType != REG_DWORD_LITTLE_ENDIAN) {
      continue;
    }

    // Note: Endian conversion is not needed. If the value is zero it should
    //       be considered, otherwise it should be skipped. However, we do require
    //       that the value be some kind of DWORD.
    if (value) {
      continue;
    }

    // Allocate space for the new string to exist under.
    pNewValueName = skAllocate(
      platform->pAllocator,
      curValueLength + 1,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pNewValueName) {
      skFree(platform->pAllocator, pValueName);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    strncpy(pNewValueName, pValueName, curValueLength);
    pNewValueName[curValueLength] = 0;

    // Push the newly-found value into the list of identified paths.
    result = skPushStringVectorIMPL(
      platform->pAllocator,
      &platform->manifestPaths,
      &pNewValueName
    );
    if (result != SK_SUCCESS) {
      skFree(platform->pAllocator, pValueName);
      return result;
    }
  }

  return SK_SUCCESS;
}

static SkResult skGetFolderPathIMPL(
  SkPlatformPLT                         platform,
  KNOWNFOLDERID const* const            knownFolderId,
  char**                                pDestination
) {
  errno_t error;
  PWSTR pWidePath;
  PWSTR pIndirectPath;
  HRESULT result;
  mbstate_t mbstate;
  size_t stringLength;
  size_t convertedCount;

  // Read the home directory value
  result = SHGetKnownFolderPath(
    knownFolderId,
    KF_FLAG_DEFAULT,
    NULL,
    &pWidePath
  );
  if (FAILED(result)) {
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  // Determine the length of the string and allocate data for it.
  stringLength = lstrlenW(pWidePath);
  *pDestination = skAllocate(
    platform->pAllocator,
    stringLength + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!*pDestination) {
    CoTaskMemFree(pWidePath);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Process the conversion from WSTR to CSTR.
  pIndirectPath = pWidePath;
  memset((void*)&mbstate, 0, sizeof(mbstate));
  error = wcsrtombs_s(&convertedCount, *pDestination, stringLength + 1, &pIndirectPath, stringLength, &mbstate);
  CoTaskMemFree(pWidePath);
  if (error != 0) {
    skFree(platform->pAllocator, *pDestination);
    return SK_ERROR_SYSTEM_INTERNAL;
  }

  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Windows Platform Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreatePlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT*                        pPlatform
) {
  uint32_t idx;
  HRESULT result;
  LSTATUS status;
  HKEY registryKey;
  DWORD dwordLength;
  DWORD copiedLength;
  BOOL callSucceeded;
  SkPlatformPLT platform;
  char* pCharBuffer;
  HANDLE findHandle;
  WIN32_FIND_DATA findFileData;

  // Allocate the platform for the system.
  platform = skClearAllocate(
    pAllocator, 
    sizeof(SkPlatformPLT_T), 
    1, 
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!platform) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  platform->pAllocator = pAllocator;

  // Grab all of the environment paths.
  result = skStringVectorDeserializeIMPL(
    &platform->environmentPaths,
    pAllocator,
    getenv("PATH"),
    ";\0",
    2
  );
  if (result != SK_SUCCESS) {
    skDestroyPlatformPLT(pAllocator, platform);
    return result;
  }

  // Grab all of the manifest paths.
  result = skStringVectorDeserializeIMPL(
    &platform->searchPaths,
    pAllocator,
    getenv("SK_PATH"),
    ";\0",
    2
  );
  if (result != SK_SUCCESS) {
    skDestroyPlatformPLT(pAllocator, platform);
    return result;
  }

  // Grab the current user's username (or set to empty string if failed).
  dwordLength = UNLEN + 1;
  callSucceeded = GetUserNameA(
    platform->pUsername,
    &dwordLength
  );
  if (!callSucceeded) {
    platform->pUsername[0] = 0;
  }

  // As long as we have a buffer greater than MAX_COMPUTERNAME_LENGTH bytes,
  // this should work. Otherwise we will just set the string to empty.
  dwordLength = MAX_COMPUTERNAME_LENGTH + 1;
  if (!GetComputerNameA(platform->pHostname, &dwordLength)) {
    platform->pHostname[0] = 0;
  }

  // Read the home directory value
  result = skGetFolderPathIMPL(
    platform,
    &FOLDERID_Profile,
    &platform->pHomeDirectory
  );
  if (result != SK_SUCCESS) {
    skDestroyPlatformPLT(pAllocator, platform);
    return result;
  }

  // Grab the current working directory's length, and then value.
  dwordLength = GetCurrentDirectoryA(0, NULL) + 1;
  platform->pCurrentDirectory = skAllocate(
    pAllocator,
    dwordLength,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!platform->pCurrentDirectory) {
    skDestroyPlatformPLT(pAllocator, platform);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  if (!GetCurrentDirectoryA(dwordLength, platform->pCurrentDirectory)) {
    platform->pCurrentDirectory[0] = 0;
  }

  // Determine the executable's directory
  dwordLength = 64;
  do {
    dwordLength *= 2;

    // Allocate more space for the executable path.
    pCharBuffer = skReallocate(
      pAllocator,
      platform->pExecutablePath,
      dwordLength,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pCharBuffer) {
      skDestroyPlatformPLT(pAllocator, platform);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    platform->pExecutablePath = pCharBuffer;

    // Attempt to copy the module file name into the path.
    copiedLength = GetModuleFileNameA(NULL, platform->pExecutablePath, dwordLength);
  } while (dwordLength == copiedLength);

  // Truncate and separate the executable path from the filename.
  pCharBuffer = strrchr(platform->pExecutablePath, '\\');
  if (pCharBuffer) {
    pCharBuffer[0] = 0;
    platform->pExecutableFilename = &pCharBuffer[1];
  }

  // Identify all of the possible manifests
  for (idx = 0; idx < platform->searchPaths.count; ++idx) {

    // Concatenate the search path with the search terms.
    pCharBuffer = skCombinePathsPLT(pAllocator, platform->searchPaths.pData[idx], "*.json");
    if (!pCharBuffer) {
      skDestroyPlatformPLT(pAllocator, platform);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }

    // Attempt to search in the provided directory
    findHandle = FindFirstFileA(
      pCharBuffer,
      &findFileData
    );
    skFree(pAllocator, pCharBuffer);
    if (findHandle == INVALID_HANDLE_VALUE) {
      continue;
    }

    // Enumerate the directory, until we find a matching manifest.
    do {

      // Duplicate the string
      pCharBuffer = skCombinePathsPLT(
        pAllocator,
        platform->searchPaths.pData[idx],
        findFileData.cFileName
      );
      if (!pCharBuffer) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }

      // Add the file to the list of manifests.
      result = skPushStringVectorIMPL(
        pAllocator,
        &platform->manifestPaths,
        &pCharBuffer
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pCharBuffer);
        return result;
      }

    } while (FindNextFileA(findHandle, &findFileData));

    FindClose(findHandle);
  }

  // Read all of the registry keys, see if any are present.
  // Note: The order this happens is important, we want the system
  //       drivers to appear after the user-specified drivers.
  if (!getenv("SK_NO_SYSTEM_PATH")) {
    for (idx = 0; idx < REGISTRY_KEY_COUNT; ++idx) {

      // Attempt to read the registry key
      status = RegOpenKeyExA(
        skRegistryKeysIMPL[idx].hkey,
        skRegistryKeysIMPL[idx].pSubKey,
        0,
        KEY_READ,
        &registryKey
      );
      if (status != ERROR_SUCCESS) {
        continue;
      }

      // Attempt to read all of the values within the key
      result = skReadRegisteryKeyIMPL(
        platform,
        registryKey
      );

      // Close the registry key
      status = RegCloseKey(registryKey);
      if (status != ERROR_SUCCESS) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_SYSTEM_INTERNAL;
      }

      // If the result of the read wasn't successful, abort.
      if (result != SK_SUCCESS) {
        skDestroyPlatformPLT(pAllocator, platform);
        return result;
      }
    }
  }

  // Return the fully-constructed platform.
  *pPlatform = platform;
  return SK_SUCCESS;
}

void SKAPI_CALL skDestroyPlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT                         platform
) {
  uint32_t idx;

  // Free the dynamic manifest search paths
  for (idx = 0; idx < platform->environmentPaths.count; ++idx) {
    skFree(pAllocator, platform->environmentPaths.pData[idx]);
  }
  for (idx = 0; idx < platform->manifestPaths.count; ++idx) {
    skFree(pAllocator, platform->manifestPaths.pData[idx]);
  }
  for (idx = 0; idx < platform->searchPaths.count; ++idx) {
    skFree(pAllocator, platform->searchPaths.pData[idx]);
  }
  skFree(pAllocator, platform->pHomeDirectory);
  skFree(pAllocator, platform->pExecutablePath);
  skFree(pAllocator, platform->pCurrentDirectory);
  skFree(pAllocator, platform);
}

char const* SKAPI_CALL skGetPlatformPropertyPLT(
  SkPlatformPLT                         platform,
  SkPlatformPropertyPLT                 property
) {
  switch (property) {
  case SK_PLATFORM_PROPERTY_HOSTNAME_PLT:
    return (*platform->pHostname) ? platform->pHostname : NULL;
  case SK_PLATFORM_PROPERTY_USERNAME_PLT:
    return (*platform->pUsername) ? platform->pUsername : NULL;
  case SK_PLATFORM_PROPERTY_HOME_DIRECTORY_PLT:
    return platform->pHomeDirectory;
  case SK_PLATFORM_PROPERTY_WORKING_DIRECTORY_PLT:
    return platform->pCurrentDirectory;
  case SK_PLATFORM_PROPERTY_EXE_DIRECTORY_PLT:
    return platform->pExecutablePath;
  case SK_PLATFORM_PROPERTY_EXE_FILENAME_PLT:
    return platform->pExecutableFilename;
  case SK_PLATFORM_PROPERTY_TERMINAL_NAME_PLT:
    return getenv("TERM");
  }
  return NULL;
}

SkResult SKAPI_CALL skEnumeratePlatformPathsPLT(
  SkPlatformPLT                         platform,
  uint32_t*                             pPlatformPathCount,
  char const**                          ppPlatformPaths
) {
  uint32_t pathCount;

  // If only the path count was requested, return only that.
  if (!ppPlatformPaths) {
    *pPlatformPathCount = platform->environmentPaths.count;
  }

  // Otherwise, we should enumerate the path vector and copy contents.
  else {
    for (pathCount = 0; pathCount < platform->environmentPaths.count; ++pathCount) {
      ppPlatformPaths[pathCount] = platform->environmentPaths.pData[pathCount];
      if (pathCount >= *pPlatformPathCount) {
        return SK_INCOMPLETE;
      }
    }
    *pPlatformPathCount = pathCount;
  }

  return SK_SUCCESS;
}

SkResult SKAPI_CALL skEnumeratePlatformManifestsPLT(
  SkPlatformPLT                         platform,
  uint32_t*                             pManifestCount,
  char const**                          ppManifests
) {
  uint32_t pathCount;

  // If only the path count was requested, return only that.
  if (!ppManifests) {
    *pManifestCount = platform->manifestPaths.count;
  }

  // Otherwise, we should enumerate the path vector and copy contents.
  else {
    for (pathCount = 0; pathCount < platform->manifestPaths.count; ++pathCount) {
      ppManifests[pathCount] = platform->manifestPaths.pData[pathCount];
      if (pathCount >= *pManifestCount) {
        return SK_INCOMPLETE;
      }
    }
    *pManifestCount = pathCount;
  }

  return SK_SUCCESS;
}

void* SKAPI_CALL skAllocatePLT(
  size_t                                size,
  size_t                                alignment
) {
  return _aligned_malloc(size, alignment);
}

void SKAPI_CALL skFreePLT(
  void*                                 memory
) {
  _aligned_free(memory);
}

char* SKAPI_CALL skRemovePathStemPLT(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pPath
) {
  char* pNewPath;
  char const* pLast;

  // Find the last path separator.
  pLast = strrchr(pPath, '\\');
  if (!pLast) {
    return NULL;
  }

  // Construct the path
  pNewPath = skAllocate(
    pAllocator,
    (pLast - pPath) + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pNewPath) {
    return NULL;
  }
  strncpy(pNewPath, pPath, (pLast - pPath));
  pNewPath[pLast - pPath] = 0;
  return pNewPath;
}

char* SKAPI_CALL skCombinePathsPLT(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pLeftPath,
  char const*                           pRightPath
) {
  char* pPath;
  int addSeparator;
  size_t leftPathLength;
  size_t rightPathLength;

  // Start out assuming we won't add a separator in
  addSeparator = 0;
  leftPathLength = strlen(pLeftPath);
  rightPathLength = strlen(pRightPath);

  // Discover if we need to add or remove a separator
  if (pLeftPath[leftPathLength - 1] == '\\') {
    if (pRightPath[0] == '\\') {
      addSeparator = -1;
    }
  }
  else if (pRightPath[0] != '\\') {
    addSeparator = 1;
  }

  // Allocate the final path (left + right + separator + null)
  pPath = skAllocate(
    pAllocator,
    leftPathLength + rightPathLength + addSeparator + 1,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pPath) {
    return NULL;
  }

  // Combine the paths
  strcpy(pPath, pLeftPath);
  strcpy(&pPath[leftPathLength + addSeparator], pRightPath);
  if (addSeparator == 1) {
    pPath[leftPathLength] = '\\';
  }

  return pPath;
}

SkBool32 SKAPI_CALL skIsAbsolutePathPLT(
  char const*                           pPath
) {

  // Default: Check for root directory
  if (*pPath == '\\') {
    return SK_TRUE;
  }

  // Otherwise, look for a device directory
  while (*pPath) {
    if (isalpha(*pPath)) {
      // This is okay, continue...
    }
    else if (pPath[0] == ':' && pPath[1] == '\\' && pPath[2] == '\\') {
      return SK_TRUE;
    }
    else {
      return SK_FALSE;
    }
    ++pPath;
  }

  return SK_FALSE;
}

SkBool32 SKAPI_CALL skFileExistsPLT(
  char const*                           pFile
) {
  DWORD dwAttrib = GetFileAttributesA(pFile);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

SkInternalResult SKAPI_CALL skLoadLibraryPLT(
  char const*                           pLibraryPath,
  SkLibraryPLT*                         pLibrary
) {
  *pLibrary = (SkLibraryPLT)LoadLibraryA(pLibraryPath);
  return (*pLibrary) ? SK_SUCCESS : SK_ERROR_INITIALIZATION_FAILED;
}

void SKAPI_CALL skUnloadLibraryPLT(
  SkLibraryPLT                          library
) {
  (void)FreeLibrary((HMODULE)library);
}

PFN_skVoidFunction SKAPI_CALL skGetLibraryProcAddrPLT(
  SkLibraryPLT                          library,
  char const*                           pName
) {
  return (PFN_skVoidFunction)GetProcAddress((HMODULE)library, pName);
}

void SKAPI_CALL skGenerateUuid(
  uint8_t                               pUuid[SK_UUID_SIZE]
) {
  UUID uuid;
  (void)UuidCreate(&uuid);
  memcpy(pUuid, &uuid, sizeof(SK_UUID_SIZE));
}
