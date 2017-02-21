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
 * A set of helpful OpenSK allocators for utility purposes.
 ******************************************************************************/

// OpenSK
#include <OpenSK/ext/sk_global.h>
#include <OpenSK/utl/allocators.h>

// C11
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Non-Standard
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

////////////////////////////////////////////////////////////////////////////////
// Debug Allocator
////////////////////////////////////////////////////////////////////////////////

struct SkAllocationLocationIMPL;
typedef struct SkAllocationInfoIMPL {
  size_t                                allocationSize;
  time_t                                allocationTime;
  time_t                                deallocationTime;
  unsigned char*                        pUserMemory;
  struct SkAllocationLocationIMPL*      pLocation;
  SkSystemAllocationScope               allocationScope;
  unsigned char                         pMemory[4];
} SkAllocationInfoIMPL;

typedef struct SkAllocationLocationIMPL {
  intptr_t                              stackHash;
  uint32_t                              stackDepth;
  time_t                                creationTime;
  time_t                                lastAccessed;
  time_t                                lastModified;
  uint32_t                              allocationCapacity;
  uint32_t                              allocationCount;
  SkAllocationInfoIMPL**                allocations;
  void*                                 stackTrace[1];
} SkAllocationLocationIMPL;

typedef struct SkAllocationStatisticsIMPL {
  size_t                                maxAllocationSize;
  size_t                                currentAllocationSize;
} SkAllocationStatisticsIMPL;

typedef struct SkDebugAllocatorDataIMPL {
  SkAllocationCallbacks const*          pAllocator;
  SkAllocationStatisticsIMPL            statistics[SK_SYSTEM_ALLOCATION_SCOPE_RANGE_SIZE];
  SkAllocationStatisticsIMPL            overallStatistics;
  int                                   debugPattern;
  uint32_t                              underrunProtection;
  uint32_t                              overrunProtection;
  uint32_t                              maxBacktraceCount;
  void**                                pBacktraceArray;
  uint32_t                              locationCapacity;
  uint32_t                              locationCount;
  SkAllocationLocationIMPL**            locations;
} SkDebugAllocatorDataIMPL;

static int ptncmp(unsigned char const* mem, int byte, size_t n) {
  while (n) {
    if ((int)*mem != byte) return *mem - byte;
    ++mem;
    --n;
  }
  return 0;
}

static void skPrintStackTrace(
  void**                                stackTrace,
  uint32_t                              stackDepth
) {
  int err;
  uint32_t idx;
  char** pSymbols;
  char buffer[1024];

  // Note: Skip the top two calls: (pAllocator, skAllocate)
  pSymbols = backtrace_symbols(stackTrace, stackDepth);
  for (idx = 2; idx < stackDepth; ++idx) {
    sprintf(buffer, "eu-addr2line -p %u %p", getpid(), stackTrace[idx]);
    err = system(buffer);
    if (err != 0) {
      fputs("Failed to run eu-addr2line tool - results may be invalid.\n", stderr);
    }
  }
  printf("\n");
  free(pSymbols);
}

static void skAddDebugMemoryIMPL(
  SkDebugAllocatorDataIMPL*             pUserData,
  SkSystemAllocationScope               allocationScope,
  size_t                                size
) {
  SkAllocationStatisticsIMPL *pStatistics;

  // Scoped statistics
  pStatistics = &pUserData->statistics[allocationScope];
  pStatistics->currentAllocationSize += size;
  if (pStatistics->currentAllocationSize > pStatistics->maxAllocationSize) {
    pStatistics->maxAllocationSize = pStatistics->currentAllocationSize;
  }

  // Overall statistics
  pStatistics = &pUserData->overallStatistics;
  pStatistics->currentAllocationSize += size;
  if (pStatistics->currentAllocationSize > pStatistics->maxAllocationSize) {
    pStatistics->maxAllocationSize = pStatistics->currentAllocationSize;
  }

}

static void skRemoveDebugMemoryIMPL(
  SkDebugAllocatorDataIMPL*             pUserData,
  SkSystemAllocationScope               allocationScope,
  size_t                                size
) {
  pUserData->overallStatistics.currentAllocationSize -= size;
  pUserData->statistics[allocationScope].currentAllocationSize -= size;
}

static SkAllocationLocationIMPL* skGetDebugAllocationLocation(
  SkDebugAllocatorDataIMPL*             pUserData,
  intptr_t                              stackHash,
  void**                                stackTrace,
  uint32_t                              stackDepth
) {
  uint32_t idx;
  uint32_t ddx;
  SkAllocationLocationIMPL* pLocation;
  SkAllocationLocationIMPL** ppLocation;

  for (idx = 0; idx < pUserData->locationCount; ++idx) {
    pLocation = pUserData->locations[idx];

    // If the stack hash or stack depth aren't the same - skip.
    if (pLocation->stackHash != stackHash || pLocation->stackDepth != stackDepth) {
      continue;
    }

    // Otherwise, this may be a match, see if the backtrace is the same.
    for (ddx = 0; ddx < pLocation->stackDepth; ++ddx) {
      if (pLocation->stackTrace[ddx] != stackTrace[ddx]) {
        break;
      }
    }

    // If we have iterated through the whole stack, this was a match.
    if (ddx == stackDepth) {
      pLocation->lastAccessed = time(NULL);
      return pLocation;
    }
  }

  // If we didn't find the allocation location, this is a new location.
  // See if we need to grow the allocation location array or not.
  if (pUserData->locationCount == pUserData->locationCapacity) {

    if (pUserData->locationCapacity == 0) {
      pUserData->locationCapacity = 1;
    }
    pUserData->locationCapacity *= 2;

    ppLocation = skReallocate(
      pUserData->pAllocator,
      pUserData->locations,
      sizeof(SkAllocationLocationIMPL*) * pUserData->locationCapacity,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!ppLocation) {
      return NULL;
    }

    pUserData->locations = ppLocation;
  }

  // Create the allocation location object and initialize it
  pLocation = skClearAllocate(
    pUserData->pAllocator,
    sizeof(SkAllocationLocationIMPL) + sizeof(void*) * stackDepth,
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pLocation) {
    return NULL;
  }
  pLocation->stackHash = stackHash;
  pLocation->stackDepth = stackDepth;
  pLocation->creationTime = time(NULL);
  pLocation->lastAccessed = pLocation->creationTime;
  pLocation->lastModified = pLocation->creationTime;
  for (idx = 0; idx < stackDepth; ++idx) {
    pLocation->stackTrace[idx] = stackTrace[idx];
  }

  // Add it to the location array
  pUserData->locations[pUserData->locationCount] = pLocation;
  ++pUserData->locationCount;

  return pLocation;
}

static SkAllocationInfoIMPL* skGetAllocationInfoIMPL(
  SkDebugAllocatorDataIMPL*             pUserData,
  char*                                 memory
) {
  return (SkAllocationInfoIMPL*)(memory - (pUserData->underrunProtection + sizeof(SkAllocationInfoIMPL) - 4));
}

static void* SKAPI_CALL skAllocationFunction_Debug(
  SkDebugAllocatorDataIMPL*             pUserData,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  int iValue;
  uint32_t idx;
  uint32_t stackDepth;
  intptr_t stackHash;
  SkAllocationInfoIMPL* pInfo;
  SkAllocationLocationIMPL* pLocation;

  // Grab an initial array for backtrace information
  // The value of backtrace should not be less-than zero, check if it is.
  iValue = backtrace(pUserData->pBacktraceArray, pUserData->maxBacktraceCount);
  if (iValue < 0) {
    return NULL;
  }
  stackDepth = (uint32_t)iValue;

  // Get the full backtrace (grab backtrace until we get all of it)
  while (stackDepth == pUserData->maxBacktraceCount) {

    // Grow the backtrace array by x2.
    if (!pUserData->maxBacktraceCount) {
      pUserData->maxBacktraceCount = 1;
    }
    pUserData->maxBacktraceCount *= 2;
    pUserData->pBacktraceArray = skReallocate(
      pUserData->pAllocator,
      pUserData->pBacktraceArray,
      sizeof(void*) * pUserData->maxBacktraceCount,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pUserData->pBacktraceArray) {
      return NULL;
    }

    // Request the backtrace again
    iValue = backtrace(pUserData->pBacktraceArray, pUserData->maxBacktraceCount);
    if (iValue < 0) {
      return NULL;
    }
    stackDepth = (uint32_t)iValue;
  }

  // Calculate the allocation hash by XORing the backtrace
  stackHash = 0x5452;
  for (idx = 0; idx < stackDepth; ++idx) {
    stackHash ^= (intptr_t)pUserData->pBacktraceArray[idx];
  }

  // Find the location entry from within the allocator info.
  pLocation = skGetDebugAllocationLocation(
    pUserData,
    stackHash,
    pUserData->pBacktraceArray,
    (uint32_t)stackDepth
  );
  if (!pLocation) {
    return NULL;
  }

  // Grow the allocation array if needed.
  if (pLocation->allocationCount == pLocation->allocationCapacity) {
    if (pLocation->allocationCapacity == 0) {
      pLocation->allocationCapacity = 1;
    }
    pLocation->allocationCapacity *= 2;
    pLocation->allocations = skReallocate(
      pUserData->pAllocator,
      pLocation->allocations,
      sizeof(SkAllocationInfoIMPL*) * pLocation->allocationCapacity,
      1,
      SK_SYSTEM_ALLOCATION_SCOPE_LOADER
    );
    if (!pLocation->allocations) {
      return NULL;
    }
  }

  // Actually allocate the requested memory.
  pInfo = skClearAllocate(
    pUserData->pAllocator,
    sizeof(SkAllocationInfoIMPL) + size + alignment +
    pUserData->underrunProtection + pUserData->overrunProtection,
    1,
    allocationScope
  );
  if (!pInfo) {
    return NULL;
  }

  // Configure the allocation information.
  skAddDebugMemoryIMPL(pUserData, allocationScope, size);
  pInfo->pUserMemory = pInfo->pMemory + pUserData->underrunProtection;
  memset(pInfo->pMemory, pUserData->debugPattern, pUserData->underrunProtection);
  memset(&pInfo->pUserMemory[size], pUserData->debugPattern, pUserData->overrunProtection);
  pInfo->allocationScope = allocationScope;
  pInfo->allocationSize = size;
  pInfo->pLocation = pLocation;
  pInfo->allocationTime = time(NULL);

  // Add the allocation to the location structure.
  pLocation->allocations[pLocation->allocationCount] = pInfo;
  ++pLocation->allocationCount;

  return pInfo->pUserMemory;
}

static void SKAPI_CALL skFreeFunction_Debug(
  SkDebugAllocatorDataIMPL*             pUserData,
  unsigned char*                        memory
) {
  uint32_t idx;
  uint32_t ddx;
  SkAllocationInfoIMPL *pInfo;
  SkAllocationLocationIMPL* pLocation;

  // Passing in NULL is valid, we should check for this case.
  if (!memory) {
    return;
  }

  // Check to see if we are freeing something not allocated by this allocator.
  for (idx = 0; idx < pUserData->locationCount; ++idx) {
    pLocation = pUserData->locations[idx];
    for (ddx = 0; ddx < pLocation->allocationCount; ++ddx) {
      pInfo = pLocation->allocations[ddx];
      if (pInfo->pUserMemory == memory) {
        break;
      }
      pInfo = NULL;
    }
    if (pInfo) {
      break;
    }
  }
  if (!pInfo) {
    fprintf(stderr, "Allocator Mismatch detected!\n");
    fprintf(stderr, "This happens when memory is freed which belongs to a different allocator.\n");
    raise(SIGINT);
  }

  // Check for double-free.
  if (pInfo->deallocationTime) {
    fprintf(stderr, "Double-Free detected:\n");
    skPrintStackTrace(pInfo->pLocation->stackTrace, pInfo->pLocation->stackDepth);
    raise(SIGINT);
  }
  pInfo->deallocationTime = time(NULL);

  // Note: we don't deallocate, we keep the data around to see if it's used
  //       after the user has deallocated. We do change the statistics set.
  skRemoveDebugMemoryIMPL(pUserData, pInfo->allocationScope, pInfo->allocationSize);
}

static void* SKAPI_CALL skReallocationFunction_Debug(
  SkDebugAllocatorDataIMPL*             pUserData,
  void*                                 pOriginal,
  size_t                                size,
  size_t                                alignment,
  SkSystemAllocationScope               allocationScope
) {
  void *pNew;
  size_t copyLength;
  SkAllocationInfoIMPL *pInfo;
  pNew = skAllocationFunction_Debug(pUserData, size, alignment, allocationScope);
  if (pOriginal) {
    pInfo = skGetAllocationInfoIMPL(pUserData, pOriginal);
    if (pNew) {
      copyLength = (size > pInfo->allocationSize) ? pInfo->allocationSize : size;
      memcpy(pNew, pOriginal, copyLength);
    }
    skFreeFunction_Debug(pUserData, pOriginal);
  }
  return pNew;
}

SKAPI_ATTR SkResult SKAPI_CALL skCreateDebugAllocatorUTL(
  SkDebugAllocatorCreateInfoUTL*        pCreateInfo,
  SkAllocationCallbacks const*          pAllocator,
  SkDebugAllocatorUTL*                  pDebugAllocator
) {
  SkAllocationCallbacks* debugAllocator;
  SkDebugAllocatorDataIMPL* pUserData;

  // Allocate the allocator and callbacks.
  debugAllocator = skClearAllocate(
    pAllocator,
    sizeof(SkAllocationCallbacks) + sizeof(SkDebugAllocatorDataIMPL),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!debugAllocator) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Initialize the allocator callbacks
  debugAllocator->pUserData = &debugAllocator[1];
  debugAllocator->pfnAllocation = (PFN_skAllocationFunction)skAllocationFunction_Debug;
  debugAllocator->pfnReallocation = (PFN_skReallocationFunction)skReallocationFunction_Debug;
  debugAllocator->pfnFree = (PFN_skFreeFunction)skFreeFunction_Debug;

  // Initialize the allocator data
  pUserData = debugAllocator->pUserData;
  pUserData->pAllocator = pAllocator;
  pUserData->debugPattern = 0xBA;
  pUserData->underrunProtection = pCreateInfo->underrunBufferSize;
  pUserData->overrunProtection = pCreateInfo->overrunBufferSize;

  *pDebugAllocator = debugAllocator;
  return SK_SUCCESS;
}

SKAPI_ATTR uint32_t SKAPI_CALL skDestroyDebugAllocatorUTL(
  SkDebugAllocatorUTL                   debugAllocator,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;
  uint32_t ddx;
  uint32_t memoryIssues;
  SkDebugAllocatorDataIMPL* pData;
  SkAllocationLocationIMPL* pLocation;
  SkAllocationInfoIMPL* pAllocation;

  // Test all of the allocations and make sure they were not invalidated.
  memoryIssues = 0;
  pData = (SkDebugAllocatorDataIMPL*)debugAllocator->pUserData;
  for (idx = 0; idx < pData->locationCount; ++idx) {
    pLocation = pData->locations[idx];
    for (ddx = 0; ddx < pLocation->allocationCount; ++ddx) {
      pAllocation = pLocation->allocations[ddx];

      // If the allocation is still active, show the allocation in question.
      if (pAllocation->deallocationTime == 0) {
        ++memoryIssues;
        fprintf(stderr, "Memory Leak Detected (%u bytes):\n", (unsigned)pAllocation->allocationSize);
        skPrintStackTrace(pLocation->stackTrace, pLocation->stackDepth);
      }

      // Check if there was a buffer underrun.
      if (ptncmp(pAllocation->pMemory, pData->debugPattern, pData->underrunProtection)) {
        ++memoryIssues;
        fprintf(stderr, "Buffer Underrun Detected:\n");
        skPrintStackTrace(pLocation->stackTrace, pLocation->stackDepth);
      }

      // Check if there was a buffer overrun.
      if (ptncmp(&pAllocation->pUserMemory[pAllocation->allocationSize], pData->debugPattern, pData->overrunProtection)) {
        ++memoryIssues;
        fprintf(stderr, "Buffer Overrun Detected:\n");
        skPrintStackTrace(pLocation->stackTrace, pLocation->stackDepth);
      }

      skFree(pAllocator, pAllocation);
    }
    skFree(pAllocator, pLocation->allocations);
    skFree(pAllocator, pLocation);
  }
  skFree(pAllocator, pData->pBacktraceArray);
  skFree(pAllocator, pData->locations);
  skFree(pAllocator, debugAllocator);

  return memoryIssues;
}

static void skPrintDebugAllocatorStatisticsIMPL(
  char const*                           scopeName,
  SkAllocationStatisticsIMPL const*     statistics
) {
  printf(
    "%10s : %u/%u\n",
    scopeName,
    (unsigned)statistics->currentAllocationSize,
    (unsigned)statistics->maxAllocationSize
  );
}

SKAPI_ATTR void SKAPI_CALL skPrintDebugAllocatorStatisticsUTL(
  SkDebugAllocatorUTL                   debugAllocator
) {
  SkDebugAllocatorDataIMPL* pUserData;
  pUserData = (SkDebugAllocatorDataIMPL*)debugAllocator->pUserData;
  skPrintDebugAllocatorStatisticsIMPL("Loader", &pUserData->statistics[SK_SYSTEM_ALLOCATION_SCOPE_LOADER]);
  skPrintDebugAllocatorStatisticsIMPL("Instance", &pUserData->statistics[SK_SYSTEM_ALLOCATION_SCOPE_INSTANCE]);
  skPrintDebugAllocatorStatisticsIMPL("Driver", &pUserData->statistics[SK_SYSTEM_ALLOCATION_SCOPE_DRIVER]);
  skPrintDebugAllocatorStatisticsIMPL("Stream", &pUserData->statistics[SK_SYSTEM_ALLOCATION_SCOPE_STREAM]);
  skPrintDebugAllocatorStatisticsIMPL("Command", &pUserData->statistics[SK_SYSTEM_ALLOCATION_SCOPE_COMMAND]);
  skPrintDebugAllocatorStatisticsIMPL("Overall", &pUserData->overallStatistics);
}
