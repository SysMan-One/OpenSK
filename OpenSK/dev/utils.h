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
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/

#ifndef   OPENSK_DEV_UTILS_H
#define   OPENSK_DEV_UTILS_H 1

#include <OpenSK/opensk.h>
#include <OpenSK/dev/vector.h>

////////////////////////////////////////////////////////////////////////////////
// Internal Definitions
////////////////////////////////////////////////////////////////////////////////

typedef enum SkInternalResult {
  SKI_SUCCESS = 0,
  SKI_INCOMPLETE = 1,
  SKI_ERROR_OUT_OF_HOST_MEMORY = -1,
  SKI_ERROR_SYSTEM_INTERNAL = -2,
  SKI_ERROR_FILE_NOT_FOUND = -3,
  SKI_ERROR_LIBRARY_FAILED_LOADING = -4,
  SKI_ERROR_MANIFEST_LAYER_NOT_FOUND = -5,
  SKI_ERROR_MANIFEST_DRIVER_NOT_FOUND = -5,
  SKI_ERROR_JSON_INVALID = -6,
  SKI_ERROR_MANIFEST_INVALID = -7,
  SKI_ERROR_MANIFEST_OVERFLOW = -8,
  SKI_ERROR_MANIFEST_UNSUPPORTED = -9,
  SKI_ERROR_MANIFEST_UNEXPECTED_TYPE = -10,
  SKI_ERROR_MANIFEST_DUPLICATE = -11
} SkInternalResult;

extern SkBool32 SKAPI_CALL skIsIntegerBigEndian();

extern SkBool32 SKAPI_CALL skIsFloatBigEndian();

#endif // OPENSK_DEV_UTILS_H
