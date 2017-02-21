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
 * OpenSK platform header. (Will not be present in final package, internal.)
 ******************************************************************************/

#ifndef   OPENSK_PLT_PLATFORM_H
#define   OPENSK_PLT_PLATFORM_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/dev/utils.h>

////////////////////////////////////////////////////////////////////////////////
// Platform Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkLibraryPLT);
SK_DEFINE_HANDLE(SkPlatformPLT);

////////////////////////////////////////////////////////////////////////////////
// Platform Structures
////////////////////////////////////////////////////////////////////////////////

typedef enum SkPlatformPropertyPLT {
  SK_PLATFORM_PROPERTY_HOSTNAME_PLT = 0,
  SK_PLATFORM_PROPERTY_USERNAME_PLT = 1,
  SK_PLATFORM_PROPERTY_HOME_DIRECTORY_PLT = 2,
  SK_PLATFORM_PROPERTY_WORKING_DIRECTORY_PLT = 3,
  SK_PLATFORM_PROPERTY_EXE_DIRECTORY_PLT = 4,
  SK_PLATFORM_PROPERTY_EXE_FILENAME_PLT = 5,
  SK_PLATFORM_PROPERTY_TERMINAL_NAME_PLT = 6,
  SK_PLATFORM_PROPERTY_BEGIN_RANGE = SK_PLATFORM_PROPERTY_HOSTNAME_PLT,
  SK_PLATFORM_PROPERTY_END_RANGE = SK_PLATFORM_PROPERTY_TERMINAL_NAME_PLT,
  SK_PLATFORM_PROPERTY_RANGE_SIZE = (SK_PLATFORM_PROPERTY_TERMINAL_NAME_PLT - SK_PLATFORM_PROPERTY_HOSTNAME_PLT + 1),
  SK_PLATFORM_PROPERTY_MAX_ENUM = 0x7FFFFFFF
} SkPlatformPropertyPLT;

////////////////////////////////////////////////////////////////////////////////
// Platform Functions
////////////////////////////////////////////////////////////////////////////////

extern SkResult SKAPI_CALL skCreatePlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT*                        pPlatform
);

extern void SKAPI_CALL skDestroyPlatformPLT(
  SkAllocationCallbacks const*          pAllocator,
  SkPlatformPLT                         platform
);

extern char const* SKAPI_CALL skGetPlatformPropertyPLT(
  SkPlatformPLT                         platform,
  SkPlatformPropertyPLT                 property
);

extern SkResult SKAPI_CALL skEnumeratePlatformPathsPLT(
  SkPlatformPLT                         platform,
  uint32_t*                             pPlatformPathCount,
  char const**                          ppPlatformPaths
);

extern SkResult SKAPI_CALL skEnumeratePlatformManifestsPLT(
  SkPlatformPLT                         platform,
  uint32_t*                             pManifestCount,
  char const**                          ppManifests
);

////////////////////////////////////////////////////////////////////////////////
// Non-Dispatchable Platform Functions
////////////////////////////////////////////////////////////////////////////////

extern void* SKAPI_CALL skAllocatePLT(
  size_t                                size,
  size_t                                alignment
);

extern void SKAPI_CALL skFreePLT(
  void*                                 memory
);

extern char* SKAPI_CALL skRemovePathStemPLT(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pPath
);

extern char* SKAPI_CALL skCombinePathsPLT(
  SkAllocationCallbacks const*          pAllocator,
  char const*                           pLeftPath,
  char const*                           pRightPath
);

extern SkBool32 SKAPI_CALL skIsAbsolutePathPLT(
  char const*                           pPath
);

extern SkBool32 SKAPI_CALL skFileExistsPLT(
  char const*                           pFile
);

extern SkResult SKAPI_CALL skLoadLibraryPLT(
  char const*                           pLibraryPath,
  SkLibraryPLT*                         pLibrary
);

extern void SKAPI_CALL skUnloadLibraryPLT(
  SkLibraryPLT                          library
);

extern PFN_skVoidFunction SKAPI_CALL skGetLibraryProcAddrPLT(
  SkLibraryPLT                          library,
  char const*                           pName
);

#endif // OPENSK_PLT_PLATFORM_H
