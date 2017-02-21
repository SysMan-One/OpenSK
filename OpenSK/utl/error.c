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
 * Common test conditions for OpenSK results and objects.
 ******************************************************************************/

#include <OpenSK/utl/error.h>
#include <OpenSK/utl/macros.h>

SKAPI_ATTR SkResult SKAPI_CALL skCheckCreateInstanceUTL(
  SkResult                              result
) {
  switch (result) {
    case SK_SUCCESS:
      break;
    default:
      SKERR("Unexpected result returned from skCreateInstance()!");
      break;
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckResolveObjectUTL(
  SkEndpoint                            endpoint,
  SkObjectType                          expected,
  char const*                           pPath
) {
  if (!endpoint) {
    SKERR("Failed to resolve the OpenSK object path: %s", pPath);
    return SK_ERROR_INVALID;
  }
  if (skGetObjectType(endpoint) != expected) {
    SKERR("The object returned was not the correct endpoint type - aborting!");
    return SK_ERROR_INVALID;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckRequestPcmStreamUTL(
  SkResult                              result
) {
  switch (result) {
    case SK_SUCCESS:
      break;
    case SK_ERROR_BUSY:
      SKERR("The requested stream is at capacity, and cannot be acquired.");
      break;
    case SK_ERROR_INVALID:
      SKERR("The provided stream request is invalid and cannot be supported by the host.");
      break;
    case SK_ERROR_NOT_SUPPORTED:
      SKERR("The requested stream does not support the requested configuration.");
      break;
    case SK_ERROR_OUT_OF_HOST_MEMORY:
      SKERR("The host failed to allocate the memory necessary to process the request.");
      break;
    case SK_ERROR_SYSTEM_INTERNAL:
      SKERR("The requested stream could not be acquired for unknown reasons.");
      break;
    default:
      SKERR("Unexpected result returned from skRequestPcmStream()!");
      break;
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckPcmStreamInfoCompatibilityUTL(
  SkPcmStreamInfo const*                streamInfoA,
  SkPcmStreamInfo const*                streamInfoB
) {
  if (streamInfoA->sType != streamInfoB->sType) {
    SKERR("The resultant stream info types do not match - aborting!");
    return SK_ERROR_INVALID;
  }
  if (streamInfoA->formatType != streamInfoB->formatType) {
    SKERR("The resultant stream infos are not compatible due to format difference: %d vs. %d", streamInfoA->formatType, streamInfoB->formatType);
    return SK_ERROR_INVALID;
  }
  if (streamInfoA->channels != streamInfoB->channels) {
    SKERR("The resultant stream infos are not compatible due to channel difference: %u vs. %u", streamInfoA->channels, streamInfoB->channels);
    return SK_ERROR_INVALID;
  }
  if (streamInfoA->sampleRate != streamInfoB->sampleRate) {
    SKERR("The resultant stream infos are not compatible due to sample rate difference: %u vs. %u", streamInfoA->sampleRate, streamInfoB->sampleRate);
    return SK_ERROR_INVALID;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckGetPcmStreamInfoUTL(
  SkResult                              result
) {
  switch (result) {
    case SK_SUCCESS:
      break;
    default:
      SKERR("Unexpected result returned from skGetPcmStreamInfo()!");
      break;
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckClosePcmStreamUTL(
  SkResult                              result
) {
  switch (result) {
    case SK_SUCCESS:
      break;
    default:
      SKERR("Unexpected result returned from skClosePcmStream()!");
      break;
  }
  return result;
}

SKAPI_ATTR SkResult SKAPI_CALL skCheckWritePcmStreamUTL(
  int64_t                               result,
  uint32_t                              expected
) {
  if (result < SK_SUCCESS) {
    switch (result) {
      case SK_ERROR_BUSY:
        // Not accepting more samples - only in non-blocking mode.
        SKERR("The stream is busy and is not currently accepting more samples.");
        break;
      case SK_ERROR_XRUN:
        SKERR("Underrun - were not able to provide samples fast-enough!");
        break;
      case SK_ERROR_SUSPENDED:
        SKERR("The stream has been suspended, please reconfigure!");
        break;
      default:
        SKERR("Unexpected result returned from skWritePcmStream*()!");
        return SK_ERROR_SYSTEM_INTERNAL;
    }
  }
  if (result != expected) {
    SKERR("Failed to write the expected number of samples: %u (expected: %u)", (uint32_t)result, expected);
    return SK_ERROR_INVALID;
  }
  return SK_SUCCESS;
}
