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

#include <OpenSK/dev/utils.h>

static union { uint32_t u32; unsigned char u8[4]; }
_skutl_intEndianCheck = { (uint32_t)0x01234567 };
SkBool32 SKAPI_CALL skIsIntegerBigEndian() {
  return (_skutl_intEndianCheck.u8[3] == 0x01) ? SK_TRUE : SK_FALSE;
}

static union { float f32; unsigned char u8[4]; }
_skutl_floatEndianCheck = { (float)0x01234567 };
SkBool32 SKAPI_CALL skIsFloatBigEndian() {
  return (_skutl_floatEndianCheck.u8[3] == 0x01) ? SK_TRUE : SK_FALSE;
}