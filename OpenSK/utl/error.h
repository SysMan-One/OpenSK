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
#ifndef   OPENSK_ERROR_H
#define   OPENSK_ERROR_H 1

#include <OpenSK/opensk.h>

SkResult SKAPI_CALL skCheckCreateInstanceUTL(
  SkResult                              result
);

SkResult SKAPI_CALL skCheckResolveObjectUTL(
  SkEndpoint                            endpoint,
  SkObjectType                          expected,
  char const*                           pPath
);

SkResult SKAPI_CALL skCheckRequestPcmStreamUTL(
  SkResult                              result
);

SkResult SKAPI_CALL skCheckPcmStreamInfoCompatibilityUTL(
  SkPcmStreamInfo const*                streamInfoA,
  SkPcmStreamInfo const*                streamInfoB
);

SkResult SKAPI_CALL skCheckGetPcmStreamInfoUTL(
  SkResult                              result
);

SkResult SKAPI_CALL skCheckClosePcmStreamUTL(
  SkResult                              result
);

SkResult SKAPI_CALL skCheckWritePcmStreamUTL(
  int64_t                               result,
  uint32_t                              expected
);

#endif //OPENSK_ERROR_H
