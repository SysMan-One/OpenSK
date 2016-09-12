/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * TODO: Fill In
 ******************************************************************************/

// OpenSK
#include <OpenSK/ext/fileio.h>

////////////////////////////////////////////////////////////////////////////////
// File I/O Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skScanFileProcessorsEXT(
  SkInstance                            instance
) {
  (void)instance;
  return SK_ERROR_NOT_IMPLEMENTED;
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateFileProcessorsEXT(
  SkInstance                            instance,
  uint32_t*                             pFileProcessorCount,
  SkFileProcessorEXT*                   pFileProcessor
) {
  (void)instance;
  (void)pFileProcessor;
  (void)pFileProcessorCount;
  return SK_ERROR_NOT_IMPLEMENTED;
}

SKAPI_ATTR SkResult SKAPI_CALL skRequestFileStreamEXT(
  SkFileProcessorEXT                    fileProcessor,
  SkFileStreamInfoEXT*                  pStreamRequest,
  SkStream*                             pStream
) {
  (void)fileProcessor;
  (void)pStream;
  (void)pStreamRequest;
  return SK_ERROR_NOT_IMPLEMENTED;
}