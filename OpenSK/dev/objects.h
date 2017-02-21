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

#ifndef   OPENSK_DEV_OBJECTS_H
#define   OPENSK_DEV_OBJECTS_H 1

// OpenSK
#include <OpenSK/ext/sk_global.h>

////////////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkInstance_T {
  SK_INTERNAL_OBJECT_BASE;
} SkInstance_T;

typedef struct SkInstanceLayer_T {
  SK_INTERNAL_OBJECT_BASE;
} SkInstanceLayer_T;

typedef struct SkDriver_T {
  SK_INTERNAL_OBJECT_BASE;
} SkDriver_T;

typedef struct SkDriverLayer_T {
  SK_INTERNAL_OBJECT_BASE;
} SkDriverLayer_T;

typedef struct SkDevice_T {
  SK_INTERNAL_OBJECT_BASE;
} SkDevice_T;

typedef struct SkEndpoint_T {
  SK_INTERNAL_OBJECT_BASE;
} SkEndpoint_T;

typedef struct SkPcmStream_T {
  SK_INTERNAL_OBJECT_BASE;
} SkPcmStream_T;

typedef struct SkPcmStreamLayer_T {
  SK_INTERNAL_OBJECT_BASE;
} SkPcmStreamLayer_T;

#endif // OPENSK_DEV_OBJECTS_H
