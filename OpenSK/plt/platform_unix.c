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
 * Implementation of the UNIX platform-specific functionality.
 * This may or may not get compiled in, depending on the target platform.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/string.h>
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/plt/platform.h>

// C99
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Non-Standard
#include <dlfcn.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////
// Unix Platform Globals
////////////////////////////////////////////////////////////////////////////////

static char const *skStaticSearchPathsIMPL[] = {
  "/usr/local/share/opensk/implicit.d/",
  "/usr/local/share/opensk/explicit.d/",
  "/usr/local/share/opensk/drivers.d/",
  "/usr/local/share/opensk/extensions.d/",
  "/usr/share/opensk/implicit.d/",
  "/usr/share/opensk/explicit.d/",
  "/usr/share/opensk/drivers.d/",
  "/usr/share/opensk/extensions.d/",
  "/etc/opensk/implicit.d/",
  "/etc/opensk/explicit.d/",
  "/etc/opensk/drivers.d/",
  "/etc/opensk/extensions.d/"
};

static char const *skDynamicSearchPathPostfixIMPL[] = {
  ".local/share/opensk/implicit.d/",
  ".local/share/opensk/explicit.d/",
  ".local/share/opensk/drivers.d/",
  ".local/share/opensk/extensions.d/"
};

static char const *skExecutableResolutionPathsIMPL[] = {
  "/proc/self/exe",
  "/proc/curproc/file",
  "/proc/self/path/a.out"
};

////////////////////////////////////////////////////////////////////////////////
// Unix Platform Defines
////////////////////////////////////////////////////////////////////////////////

#define STATIC_SEARCH_PATHS                                                     \
(sizeof(skStaticSearchPathsIMPL) / sizeof(skStaticSearchPathsIMPL[0]))

#define DYNAMIC_SEARCH_PATHS                                                    \
(sizeof(skDynamicSearchPathPostfixIMPL) / sizeof(skDynamicSearchPathPostfixIMPL[0]))

#define EXECUTABLE_RESOLUTION_PATHS                                             \
(sizeof(skExecutableResolutionPathsIMPL) / sizeof(skExecutableResolutionPathsIMPL[0]))

////////////////////////////////////////////////////////////////////////////////
// Unix Platform Types
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
  char*                                 pShellPath;
  char                                  pHostname[HOST_NAME_MAX + 1];
  char                                  pUsername[LOGIN_NAME_MAX + 1];
} SkPlatformPLT_T;

////////////////////////////////////////////////////////////////////////////////
// Unix Platform Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreatePlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT*                        pPlatform
) {
  char *pChar;
  uint32_t idx;
  DIR *directory;
  SkResult result;
  size_t cwdLength;
  ssize_t exeLength;
  SkPlatformPLT platform;
  struct passwd *passwd;
  struct dirent *entry;

  // Allocate the platform object
  platform = skClearAllocate(
    pAllocator,
    sizeof(SkPlatformPLT_T),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!platform) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Grab all of the environment paths.
  result = skStringVectorDeserializeIMPL(
    &platform->environmentPaths,
    pAllocator,
    getenv("PATH"),
    ":\0",
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
    ":\0",
    2
  );
  if (result != SK_SUCCESS) {
    skDestroyPlatformPLT(pAllocator, platform);
    return result;
  }

  // Grab the hostname information
  if (gethostname(platform->pHostname, HOST_NAME_MAX) != 0) {
    platform->pHostname[0] = 0;
  }

  // Grab the user, home, and shell information
  passwd = getpwuid(getuid());
  if (passwd) {
    strncpy(platform->pUsername, passwd->pw_name, LOGIN_NAME_MAX);
    platform->pUsername[LOGIN_NAME_MAX] = 0;
    platform->pHomeDirectory = skDuplicateCString(
      passwd->pw_dir,
      pAllocator
    );
    if (!platform->pHomeDirectory) {
      skDestroyPlatformPLT(pAllocator, platform);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    platform->pShellPath = skDuplicateCString(
      passwd->pw_shell,
      pAllocator
    );
    if (!platform->pShellPath) {
      skDestroyPlatformPLT(pAllocator, platform);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }

  // Prime the current directory buffer with an initial allocation.
  // Note: We guess that the path may initially be no longer than 32-bytes.
  cwdLength = 32;
  platform->pCurrentDirectory = skAllocate(
    pAllocator,
    cwdLength,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!platform->pCurrentDirectory) {
    skDestroyPlatformPLT(pAllocator, platform);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // If we fail to get the current working directory because of buffer length,
  // we should double the buffer size and then try again. Otherwise, no CWD.
  while (!getcwd(platform->pCurrentDirectory, cwdLength)) {

    // Check that the reason we failed is not unexpected
    if (errno != ERANGE) {
      skFree(pAllocator, platform->pCurrentDirectory);
      break;
    }

    // Double the path allocation size
    cwdLength *= 2;
    pChar = skReallocate(
      pAllocator,
      platform->pCurrentDirectory,
      cwdLength,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pChar) {
      skDestroyPlatformPLT(pAllocator, platform);
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    platform->pCurrentDirectory = pChar;

  }

  // Prime the executable directory buffer with an initial allocation.
  // Note: We guess that the path may initially be no longer than 32-bytes.
  cwdLength = 32;
  platform->pExecutablePath = skAllocate(
    pAllocator,
    cwdLength,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!platform->pExecutablePath) {
    skDestroyPlatformPLT(pAllocator, platform);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Handle resolving the executable path in Unix environments.
  // Note: This may still fail on some systems due to permissions. However,
  //       without grabbing the argv[0] value, this is the best we can do.
  for (idx = 0; idx < EXECUTABLE_RESOLUTION_PATHS; ++idx ) {

    // If result is -1, this didn't work, proceed to next path
    exeLength = readlink(
      skExecutableResolutionPathsIMPL[idx],
      platform->pExecutablePath,
      cwdLength
    );
    if (exeLength < 0) {
      continue;
    }

    // If result is equal to the buffer length, we need more buffer space.
    while (exeLength == (ssize_t)cwdLength) {

      // Double the path allocation size
      cwdLength *= 2;
      pChar = skReallocate(
        pAllocator,
        platform->pExecutablePath,
        cwdLength,
        1,
        SK_SYSTEM_ALLOCATION_SCOPE_LOADER
      );
      if (!pChar) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }
      platform->pExecutablePath = pChar;

      // Attempt to grab the executable path again.
      exeLength = readlink(
        skExecutableResolutionPathsIMPL[idx],
        platform->pExecutablePath,
        cwdLength
      );

    }

    // If the executable length is positive, we've resolved correctly.
    if (exeLength > 0) {
      pChar = strrchr(platform->pExecutablePath, '/');
      if (pChar) {
        pChar[0] = 0;
        platform->pExecutableFilename = &pChar[1];
      }
      break;
    }

  }

  // If we leave the previous loop with an error, free the string.
  if (exeLength < 0) {
    skFree(pAllocator, platform->pExecutablePath);
    platform->pExecutablePath = NULL;
  }

  // Only process the system paths if environment variable is not set.
  if (!getenv("SK_NO_SYSTEM_PATH")) {

    // Grab all of the user-directory search paths.
    for (idx = 0; idx < DYNAMIC_SEARCH_PATHS; ++idx) {

      // Combine the home directory with the search subpath.
      pChar = skCombinePathsPLT(
        pAllocator,
        platform->pHomeDirectory,
        skDynamicSearchPathPostfixIMPL[idx]
      );
      if (!pChar) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }

      // Add the string to the search path vector.
      result = skPushStringVectorIMPL(
        pAllocator,
        &platform->searchPaths,
        &pChar
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pChar);
        skDestroyPlatformPLT(pAllocator, platform);
        return result;
      }

    }

    // Grab all of the system search paths.
    for (idx = 0; idx < STATIC_SEARCH_PATHS; ++idx) {

      // Duplicate the internal search path representation.
      pChar = skDuplicateCString(
        skStaticSearchPathsIMPL[idx],
        pAllocator
      );
      if (!pChar) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }

      // Add the string to the search path vector.
      result = skPushStringVectorIMPL(
        pAllocator,
        &platform->searchPaths,
        &pChar
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pChar);
        skDestroyPlatformPLT(pAllocator, platform);
        return result;
      }

    }

  }

  // Find all of the manifest files within the search paths provided.
  for (idx = 0; idx < platform->searchPaths.count; ++idx) {

    // Attempt to open the provided directory.
    directory = opendir(platform->searchPaths.pData[idx]);
    if (!directory) {
      continue;
    }

    // Read all of the files in the directory.
    while ((entry = readdir(directory))) {

      // Skip all directory entries
      if (entry->d_type == DT_DIR) {
        continue;
      }

      // Skip all non-manifest files (files not ending in .json)
      if (!skCStringEndsWith(entry->d_name, ".json")) {
        continue;
      }

      // Add each file to a list of potential manifests
      pChar = skCombinePathsPLT(
        pAllocator,
        platform->searchPaths.pData[idx],
        entry->d_name
      );
      if (!pChar) {
        skDestroyPlatformPLT(pAllocator, platform);
        return SK_ERROR_OUT_OF_HOST_MEMORY;
      }

      // Add the string to the manifest path vector.
      result = skPushStringVectorIMPL(
        pAllocator,
        &platform->manifestPaths,
        &pChar
      );
      if (result != SK_SUCCESS) {
        skFree(pAllocator, pChar);
        skDestroyPlatformPLT(pAllocator, platform);
        return result;
      }

    }
    closedir(directory);
  }

  *pPlatform = platform;
  return SK_SUCCESS;
}

void SKAPI_CALL skDestroyPlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT                         platform
) {
  uint32_t idx;

  // Free the dynamic manifest search paths
  for (idx = 0; idx < platform->searchPaths.count; ++idx) {
    skFree(pAllocator, platform->searchPaths.pData[idx]);
  }
  skFree(pAllocator, platform->searchPaths.pData);
  for (idx = 0; idx < platform->environmentPaths.count; ++idx) {
    skFree(pAllocator, platform->environmentPaths.pData[idx]);
  }
  skFree(pAllocator, platform->environmentPaths.pData);
  for (idx = 0; idx < platform->manifestPaths.count; ++idx) {
    skFree(pAllocator, platform->manifestPaths.pData[idx]);
  }
  skFree(pAllocator, platform->manifestPaths.pData);
  skFree(pAllocator, platform->pHomeDirectory);
  skFree(pAllocator, platform->pCurrentDirectory);
  skFree(pAllocator, platform->pExecutablePath);
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
  case SK_PLATFORM_PROPERTY_RANGE_SIZE:
  case SK_PLATFORM_PROPERTY_MAX_ENUM:
    break;
  }
  return NULL;
}

SkResult SKAPI_CALL skEnumeratePlatformPathsPLT(
  SkPlatformPLT                         platform,
  uint32_t*                             pPlatformPathCount,
  char const**                          ppPlatformPaths
){
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

////////////////////////////////////////////////////////////////////////////////
// Non-Dispatchable Platform Functions
////////////////////////////////////////////////////////////////////////////////

void* SKAPI_CALL skAllocatePLT(
  size_t                                size,
  size_t                                alignment
) {
  return aligned_alloc(alignment, size);
}

void SKAPI_CALL skFreePLT(
  void*                                 memory
) {
  free(memory);
}

char* SKAPI_CALL skRemovePathStemPLT(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pPath
) {
  char* pNewPath;
  char const* pLast;

  // Find the last path separator.
  pLast = strrchr(pPath, '/');
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
  if (pLeftPath[leftPathLength - 1] == '/') {
    if (pRightPath[0] == '/') {
      addSeparator = -1;
    }
  }
  else if (pRightPath[0] != '/') {
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
    pPath[leftPathLength] = '/';
  }

  return pPath;
}

SkBool32 SKAPI_CALL skIsAbsolutePathPLT(
  char const*                           pPath
) {

  // Default: Check for root directory
  if (*pPath == '/') {
    return SK_TRUE;
  }

  // Otherwise, look for a device directory
  while (*pPath) {
    if (isalpha(*pPath)) {
      // This is okay, continue...
    }
    else if (pPath[0] == ':' && pPath[1] == '/' && pPath[2] == '/') {
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
  if (access(pFile, F_OK) != -1) {
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkResult SKAPI_CALL skLoadLibraryPLT(
  char const*                           pLibraryPath,
  SkLibraryPLT*                         pLibrary
) {
  void* pDlResult;

  // Just store the dlopen in the result.
  pDlResult = dlopen(pLibraryPath, RTLD_LAZY);
  if (!pDlResult) {
    return SK_ERROR_INITIALIZATION_FAILED;
  }
  *pLibrary = (SkLibraryPLT)pDlResult;

  return SK_SUCCESS;
}

void SKAPI_CALL skUnloadLibraryPLT(
  SkLibraryPLT                          library
) {
  (void)dlclose((void*)library);
}

PFN_skVoidFunction SKAPI_CALL skGetLibraryProcAddrPLT(
  SkLibraryPLT                          library,
  char const*                           pName
) {
  return (PFN_skVoidFunction)(intptr_t)dlsym((void*)library, pName);
}

void SKAPI_CALL skGenerateUuid(
  uint8_t                               pUuid[SK_UUID_SIZE]
) {
  uuid_clear(pUuid);
  uuid_generate(pUuid);
}
