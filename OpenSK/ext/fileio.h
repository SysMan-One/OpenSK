/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * TODO: Fill In
 ******************************************************************************/
#ifndef   OPENSK_EXT_FILEIO_H
#define   OPENSK_EXT_FILEIO_H 1

// OpenSK
#include <OpenSK/opensk.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// File I/O Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkFileProcessorEXT);

typedef enum SkFileAccessModeEXT {
  SKX_FILE_ACCESS_MODE_UNDEFINED,
  SKX_FILE_ACCESS_MODE_READ,
  SKX_FILE_ACCESS_MODE_WRITE
} SkFileAccessModeEXT;

typedef struct SkFileStreamInfoEXT {
  char const*                           fileName;
  SkFileAccessModeEXT                   accessMode;
  SkStreamInfo                          streamInfo;
} SkFileStreamInfoEXT;

////////////////////////////////////////////////////////////////////////////////
// File I/O Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skScanFileProcessorsEXT(
  SkInstance                            instance
);

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateFileProcessorsEXT(
  SkInstance                            instance,
  uint32_t*                             pFileProcessorCount,
  SkFileProcessorEXT*                   pFileProcessor
);

SKAPI_ATTR SkResult SKAPI_CALL skRequestFileStreamEXT(
  SkFileProcessorEXT                    fileProcessor,
  SkFileStreamInfoEXT*                  pStreamRequest,
  SkStream*                             pStream
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_FILEIO_H
