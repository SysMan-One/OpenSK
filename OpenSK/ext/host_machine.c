/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A structure for gathering properties and path manipulation for the host.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/macros.h>
#include <OpenSK/ext/host_machine.h>

// C11
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// Non-Standard Headers
#include <unistd.h> // getuid
#include <pwd.h>    // getpwuid
#include <errno.h>  // errno
#include <sys/stat.h> // stat

////////////////////////////////////////////////////////////////////////////////
// Host Machine Defines
////////////////////////////////////////////////////////////////////////////////

typedef struct SkHostMachineEXT_T {
  SkAllocationCallbacks const*          pAllocator;
  SkStringHeapEXT                       properties;
  SkStringHeapEXT                       systemPaths;
} SkHostMachineEXT_T;

////////////////////////////////////////////////////////////////////////////////
// Host Machine Functions (IMPL)
////////////////////////////////////////////////////////////////////////////////

static SkResult skAddHostMachinePropertyIMPL(
  SkHostMachineEXT                      hostMachine,
  char const*                           propertyName,
  char const*                           propertyValue
) {
  SkResult result;
  size_t propertyNameLength;
  size_t propertyValueLength;

  // Calculate the required remaining capacity for the property
  propertyNameLength = strlen(propertyName);
  propertyValueLength = strlen(propertyValue);
  result = skStringHeapReserveRemainingEXT(hostMachine->properties, propertyNameLength + propertyValueLength, 2);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Add the property name and value
  // These calls shouldn't fail, because we already reserved space, but check anyways.
  // If the second call fails, we should remove the first call's data, since the property is incomplete.
  result = skStringHeapNAddEXT(hostMachine->properties, propertyName, propertyNameLength);
  if (result != SK_SUCCESS) {
    return result;
  }
  result = skStringHeapNAddEXT(hostMachine->properties, propertyValue, propertyValueLength);
  if (result != SK_SUCCESS) {
    skStringHeapRemoveAtEXT(hostMachine->properties, skStringHeapCountEXT(hostMachine->properties) - 1);
    return result;
  }

  return SK_SUCCESS;
}

#define skAddhostMachineProperty(key, value)                                  \
if (skAddHostMachinePropertyIMPL(hostMachine, key, value) != SK_SUCCESS) {    \
  skDestroyHostMachineEXT(hostMachine);                                       \
  SKFREE(pAllocator, pBuffer);                                                \
  return SK_ERROR_OUT_OF_HOST_MEMORY;                                         \
}

static SkResult skResolveHostMachinePathIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkHostMachineEXT                      hostMachine,
  char const*                           pathSpec,
  char const*                           filename,
  char**                                pBuffer,
  size_t*                               pBufferCapacity
) {
  size_t pathSpecLength;
  size_t filenameLength;

  // Dynamically re-allocate the buffer if needed
  pathSpecLength = strlen(pathSpec);
  filenameLength = strlen(filename);
  if (*pBufferCapacity < pathSpecLength + filenameLength) {
    SKFREE(pAllocator, *pBuffer);
    *pBuffer = SKALLOC(pAllocator, pathSpecLength + filenameLength + 2, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
    if (!*pBuffer) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
  }

  // Create and clean the working directory path
  strcpy(*pBuffer, pathSpec);
  (*pBuffer)[pathSpecLength] = '/';
  strcpy(&(*pBuffer)[pathSpecLength + 1], filename);
  skCleanHostMachinePathEXT(hostMachine, *pBuffer);

  return SK_SUCCESS;
}

static SkBool32 skIsHostMachinePathAbsoluteEXT(
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
    else if (pPath[0] == ':' && pPath[1] == '/') {
      return SK_TRUE;
    }
    else {
      return SK_FALSE;
    }
    ++pPath;
  }

  return SK_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Host Machine Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateHostMachineEXT(
  SkAllocationCallbacks const*          pAllocator,
  SkHostMachineEXT*                     pHostMachine,
  int                                   argc,
  char const*                           argv[]
) {
  SkResult result;
  char* pChar;
  char* pCharEnd;
  char const* pCharConst;
  char const* pCharConstEnd;
  struct passwd* passwd;

  char* pBuffer;
  size_t pBufferSize;
  SkHostMachineEXT hostMachine;

  (void)argc;

  // Construct the working buffer
  pBufferSize = 512;
  pBuffer = SKALLOC(pAllocator, pBufferSize, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!pBuffer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Construct the SkHostMachineEXT object
  hostMachine = SKALLOC(pAllocator, sizeof(SkHostMachineEXT_T), SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!hostMachine) {
    SKFREE(pAllocator, pBuffer);
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  hostMachine->pAllocator = pAllocator;

  // Construct the property heap
  result = skCreateStringHeapEXT(pAllocator, &hostMachine->properties, 512);
  if (result != SK_SUCCESS) {
    SKFREE(pAllocator, hostMachine);
    SKFREE(pAllocator, pBuffer);
    return result;
  }

  // Construct the system paths heap
  result = skCreateStringHeapEXT(pAllocator, &hostMachine->systemPaths, 512);
  if (result != SK_SUCCESS) {
    skDestroyStringHeapEXT(hostMachine->properties);
    SKFREE(pAllocator, hostMachine);
    SKFREE(pAllocator, pBuffer);
    return result;
  }

  // Fill in the system paths
  // Note: Some properties require path information to resolve, so do this first!
  pCharConst = getenv("PATH");
  while (pCharConst) {
    pCharConstEnd = strchr(pCharConst, ':');
    if (pCharConstEnd) {
      result = skStringHeapNAddEXT(hostMachine->systemPaths, pCharConst, pCharConstEnd - pCharConst);
      ++pCharConstEnd; // Skip the path separator
    }
    else {
      result = skStringHeapAddEXT(hostMachine->systemPaths, pCharConst);
    }
    if (result != SK_SUCCESS) {
      skDestroyHostMachineEXT(hostMachine);
      SKFREE(pAllocator, pBuffer);
      return result;
    }
    pCharConst = pCharConstEnd;
  }

  // Fill in properties (HOSTNAME)
  if (gethostname(pBuffer, pBufferSize) == 0) {
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_HOSTNAME, pBuffer);
  }

  // Fill in properties (USER, HOME, SHELL)
  passwd = getpwuid(getuid());
  if (passwd) {
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_USERNAME, passwd->pw_name);
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_HOME_DIRECTORY, passwd->pw_dir);
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_SHELL_PATH, passwd->pw_shell);
  }

  // Fill in properties (CWD)
  do {
    pCharConst = getcwd(pBuffer, pBufferSize);
    if (pCharConst != NULL) {
      skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_WORKING_DIRECTORY, pCharConst);
    }
    else {
      // Current buffer doesn't have enough space, resize...
      if (errno == ERANGE) {
        pBufferSize *= 2;
        pChar = SKALLOC(pAllocator, pBufferSize, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
        if (!pChar) {
          skDestroyStringHeapEXT(hostMachine->properties);
          SKFREE(pAllocator, pBuffer);
          return SK_ERROR_OUT_OF_HOST_MEMORY;
        }
        SKFREE(pAllocator, pBuffer);
        pBuffer = pChar;
      }
      // Some other error has occurred, and we don't know how to continue
      else {
        break;
      }
    }
  } while (!pCharConst);

  // Fill in properties (TTY)
  pCharConst = getenv("TERM");
  if (pCharConst) {
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_TERMINAL_NAME, pCharConst);
  }

  // Fill in properties (EXE)
  result = skResolveHostMachinePathEXT(hostMachine, argv[0], &pChar);
  if (result == SK_SUCCESS) {
    skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_EXE_PATH, pChar);
    pCharEnd = strrchr(pChar, '/');
    if (pCharEnd) {
      *pCharEnd = 0;
      skAddhostMachineProperty(SKEXT_HOST_MACHINE_PROPERTY_EXE_DIRECTORY, pChar);
    }
    SKFREE(pAllocator, pChar);
  }

  *pHostMachine = hostMachine;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyHostMachineEXT(
  SkHostMachineEXT                      hostMachine
) {
  skDestroyStringHeapEXT(hostMachine->systemPaths);
  skDestroyStringHeapEXT(hostMachine->properties);
  SKFREE(hostMachine->pAllocator, hostMachine);
}

SKAPI_ATTR char const* SKAPI_CALL skNextHostMachinePropertyEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           property
) {
  if (!property) {
    return skStringHeapNextEXT(hostMachine->properties, NULL);
  }
  else {
    property = skStringHeapNextEXT(hostMachine->properties, property);
    if (property) {
      return skStringHeapNextEXT(hostMachine->properties, property);
    }
  }

  // Generally, this shouldn't happen - if it does the user provided the wrong input.
  // TODO: Assert?
  return NULL;
}

SKAPI_ATTR char const* SKAPI_CALL skGetHostMachinePropertyEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           property
) {
  char const* currProperty = NULL;
  while ((currProperty = skNextHostMachinePropertyEXT(hostMachine, currProperty))) {
    if (strcmp(currProperty, property) == 0) {
      return skStringHeapNextEXT(hostMachine->properties, currProperty);
    }
  }
  return NULL;
}

SKAPI_ATTR char const* SKAPI_CALL skNextHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           systemPath
) {
  return skStringHeapNextEXT(hostMachine->systemPaths, systemPath);
}

SKAPI_ATTR char* SKAPI_CALL skCleanHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char*                                 path
) {
  char* pCurr;
  char* pComp;
  char* pPath;
  pCurr = pComp = path;
  (void)hostMachine;

  // Clean up the filesystem root ([a-Z]+:/|/)/?
  // This portion of the path is optional, as a path may be relative.
  // Examples: /, //, sk://, C:/
  // Case: ([a-Z]+:/)
  if (isalpha(*pComp)) {
    while (isalpha(*pComp)) {
      ++pCurr;
      ++pComp;
    }
    if (pComp[0] != ':' || pComp[1] != '/') {
      pCurr = pComp = path;
      goto clean_path;
    }
    pCurr += 2;
    pComp += 2;
  }
  // Case: (/)
  else if (*pComp == '/') {
    ++pCurr;
    ++pComp;
  }
  // Fail!
  else {
    goto clean_path;
  }

  // Case: /?
  if (*pComp == '/') {
    ++pCurr;
    ++pComp;
  }

  // Clean up the filesystem path (([a-Z.]|.|..)/)*
  clean_path:

  pPath = pCurr;
  while (*pComp) {

    // Check for current-directory no-op
    if (pComp[0] == '.' && pComp[1] == '\0') {
      ++pComp;
    }
    else if (pComp[0] == '.' && pComp[1] == '/') {
      pComp += 2;
    }

    // Check for moving-up directory op
    else if (pComp[0] == '.' && pComp[1] == '.' && pComp[2] == '\0') {
      pComp += 2;
      if ((pCurr == pPath)
          ||(pCurr[-1] == '.' && pCurr[-2] == '.' && pCurr - 2 == pPath)
          ||(pCurr[-1] == '.' && pCurr[-2] == '.' && pCurr[-3] == '/')) {
        if (pCurr != pPath) {
          *pCurr = '/'; ++pCurr;
        }
        *pCurr = '.'; ++pCurr;
        *pCurr = '.'; ++pCurr;
      }
      else {
        while (pCurr != pPath) {
          --pCurr;
          if (*pCurr == '/') break;
        }
      }
    }
    else if (pComp[0] == '.' && pComp[1] == '.' && pComp[2] == '/') {
      pComp += 3;
      if ((pCurr == pPath)
          ||(pCurr[-1] == '.' && pCurr[-2] == '.' && pCurr - 2 == pPath)
          ||(pCurr[-1] == '.' && pCurr[-2] == '.' && pCurr[-3] == '/')) {
        if (pCurr != pPath) {
          *pCurr = '/'; ++pCurr;
        }
        *pCurr = '.'; ++pCurr;
        *pCurr = '.'; ++pCurr;
      }
      else {
        while (pCurr != pPath) {
          --pCurr;
          if (*pCurr == '/') break;
        }
      }
    }

    // Otherwise, we have a stem of the path - process it.
    else {
      if (pCurr != pPath) {
        *pCurr = '/'; ++pCurr;
      }
      while (*pComp && *pComp != '/') {
        *pCurr = *pComp;
        ++pCurr; ++pComp;
      }
      if (*pComp == '/') {
        ++pComp;
      }
    }
  }

  *pCurr = 0;
  return path;
}

SKAPI_ATTR SkResult SKAPI_CALL skResolveHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           filename,
  char**                                pAllocatedResult
) {
  char *pBuffer;
  SkResult result;
  char const* pCharConst;
  size_t pBufferCapacity;
  size_t filenameLength;
  struct stat fileStats;

  // Allocate a workspace for the filename (and clean it)
  pBufferCapacity = filenameLength = strlen(filename);
  pBuffer = SKALLOC(hostMachine->pAllocator, filenameLength + 1, SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!pBuffer) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  skCleanHostMachinePathEXT(hostMachine, pBuffer);
  strcpy(pBuffer, filename);

  // Check if it's an absolute path (if it exists, we're done!)
  if (skIsHostMachinePathAbsoluteEXT(pBuffer)) {
    if (stat(pBuffer, &fileStats) == 0) {
      (*pAllocatedResult) = pBuffer;
      return SK_SUCCESS;
    }
    else {
      return SK_ERROR_NOT_FOUND;
    }
  }

  // Check if it resides within the working directory
  pCharConst = skGetHostMachinePropertyEXT(hostMachine, SKEXT_HOST_MACHINE_PROPERTY_WORKING_DIRECTORY);
  if (pCharConst) {

    // Construct the path from it's parts
    result = skResolveHostMachinePathIMPL(hostMachine->pAllocator, hostMachine, pCharConst, filename, &pBuffer, &pBufferCapacity);
    if (result != SK_SUCCESS) {
      return result;
    }

    // Check if the path exists, if so we're done!
    if (stat(pBuffer, &fileStats) == 0) {
      (*pAllocatedResult) = pBuffer;
      return SK_SUCCESS;
    }

  }

  // Check if it resides within the executable directory
  pCharConst = skGetHostMachinePropertyEXT(hostMachine, SKEXT_HOST_MACHINE_PROPERTY_EXE_DIRECTORY);
  if (pCharConst) {

    // Construct the path from it's parts
    result = skResolveHostMachinePathIMPL(hostMachine->pAllocator, hostMachine, pCharConst, filename, &pBuffer, &pBufferCapacity);
    if (result != SK_SUCCESS) {
      return result;
    }

    // Check if the path exists, if so we're done!
    if (stat(pBuffer, &fileStats) == 0) {
      (*pAllocatedResult) = pBuffer;
      return SK_SUCCESS;
    }

  }

  // Check if it resides within the system directories
  pCharConst = NULL;
  while ((pCharConst = skNextHostMachinePathEXT(hostMachine, pCharConst))) {

    // Construct the path from it's parts
    result = skResolveHostMachinePathIMPL(hostMachine->pAllocator, hostMachine, pCharConst, filename, &pBuffer, &pBufferCapacity);
    if (result != SK_SUCCESS) {
      return result;
    }

    // Check if the path exists, if so we're done!
    if (stat(pBuffer, &fileStats) == 0) {
      (*pAllocatedResult) = pBuffer;
      return SK_SUCCESS;
    }

  }

  SKFREE(hostMachine->pAllocator, pBuffer);
  return SK_ERROR_NOT_FOUND;
}
