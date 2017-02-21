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

#ifndef   OPENSK_DEV_JSON_H
#define   OPENSK_DEV_JSON_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/dev/utils.h>

////////////////////////////////////////////////////////////////////////////////
// JSON Types
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkJsonValue);
SK_DEFINE_HANDLE(SkJsonObject);
SK_DEFINE_HANDLE(SkJsonArray);

typedef enum SkJsonType {
  SK_JSON_TYPE_UNKNOWN = 0,
  SK_JSON_TYPE_OBJECT = 1,
  SK_JSON_TYPE_ARRAY = 2,
  SK_JSON_TYPE_NUMBER = 3,
  SK_JSON_TYPE_STRING = 4,
  SK_JSON_TYPE_TRUE = 5,
  SK_JSON_TYPE_FALSE = 6,
  SK_JSON_TYPE_NULL = 7,
  SK_JSON_TYPE_BEGIN_RANGE = SK_JSON_TYPE_UNKNOWN,
  SK_JSON_TYPE_END_RANGE = SK_JSON_TYPE_NULL,
  SK_JSON_TYPE_RANGE_SIZE = (SK_JSON_TYPE_NULL - SK_JSON_TYPE_UNKNOWN + 1),
  SK_JSON_TYPE_MAX_ENUM = 0x7FFFFFFF
} SkJsonType;

typedef struct SkJsonProperty {
  char const*                           pPropertyName;
  SkJsonValue                           propertyValue;
} SkJsonProperty;

////////////////////////////////////////////////////////////////////////////////
// JSON Functions
////////////////////////////////////////////////////////////////////////////////

extern SkResult SKAPI_CALL skCreateJsonObjectFromString(
  char const*                           pString,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkJsonObject*                         pObject
);

extern SkResult SKAPI_CALL skCreateJsonObjectFromFile(
  char const*                           pFilepath,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkJsonObject*                         pObject
);

extern void SKAPI_CALL skDestroyJsonObject(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          object
);

extern SkJsonType SKAPI_CALL skGetJsonType(
  SkJsonValue                           value
);

extern uint32_t SKAPI_CALL skGetJsonArrayLength(
  SkJsonArray                           array
);

extern uint32_t SKAPI_CALL skGetJsonObjectPropertyCount(
  SkJsonObject                          object
);

extern SkJsonProperty SKAPI_CALL skGetJsonObjectPropertyElement(
  SkJsonObject                          object,
  uint32_t                              element
);

// Try-Getters

extern SkBool32 SKAPI_CALL skTryGetJsonValueProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  SkJsonValue*                          pValue
);

extern SkBool32 SKAPI_CALL skTryGetJsonObjectProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  SkJsonObject*                         pObject
);

extern SkBool32 SKAPI_CALL skTryGetJsonArrayProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  SkJsonArray*                          pArray
);

extern SkBool32 SKAPI_CALL skTryGetJsonNumberProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  double*                               pNumber
);

extern SkBool32 SKAPI_CALL skTryGetJsonStringProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  char const**                          pString
);

extern SkBool32 SKAPI_CALL skTryGetJsonBooleanProperty(
  SkJsonObject                          object,
  char const*                           pProperty,
  SkBool32*                             pBoolean
);

// Getters

extern SkJsonValue SKAPI_CALL skGetJsonValueProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

extern SkJsonObject SKAPI_CALL skGetJsonObjectProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

extern SkJsonArray SKAPI_CALL skGetJsonArrayProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

extern double SKAPI_CALL skGetJsonNumberProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

extern char const* SKAPI_CALL skGetJsonStringProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

extern SkBool32 SKAPI_CALL skGetJsonBooleanProperty(
  SkJsonObject                          object,
  char const*                           pProperty
);

// Try-Casts

extern SkBool32 SKAPI_CALL skTryCastJsonObject(
  SkJsonValue                           value,
  SkJsonObject*                         pObject
);

extern SkBool32 SKAPI_CALL skTryCastJsonArray(
  SkJsonValue                           value,
  SkJsonArray*                          pArray
);

extern SkBool32 SKAPI_CALL skTryCastJsonNumber(
  SkJsonValue                           value,
  double*                               pNumber
);

extern SkBool32 SKAPI_CALL skTryCastJsonString(
  SkJsonValue                           value,
  char const**                          pString
);

extern SkBool32 SKAPI_CALL skTryCastJsonBoolean(
  SkJsonValue                           value,
  SkBool32*                             pBoolean
);

// Casts

extern SkJsonObject SKAPI_CALL skCastJsonObject(
  SkJsonValue                           value
);

extern SkJsonArray SKAPI_CALL skCastJsonArray(
  SkJsonValue                           value
);

extern double SKAPI_CALL skCastJsonNumber(
  SkJsonValue                           value
);

extern char const* SKAPI_CALL skCastJsonString(
  SkJsonValue                           value
);

extern SkBool32 SKAPI_CALL skCastJsonBoolean(
  SkJsonValue                           value
);

// Try Element Access

extern SkBool32 SKAPI_CALL skTryGetJsonValueElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonValue*                          pValue
);

extern SkBool32 SKAPI_CALL skTryGetJsonObjectElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonObject*                         pObject
);

extern SkBool32 SKAPI_CALL skTryGetJsonArrayElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonArray*                          pArray
);

extern SkBool32 SKAPI_CALL skTryGetJsonNumberElement(
  SkJsonArray                           array,
  uint32_t                              index,
  double*                               pNumber
);

extern SkBool32 SKAPI_CALL skTryGetJsonStringElement(
  SkJsonArray                           array,
  uint32_t                              index,
  char const**                          pString
);

extern SkBool32 SKAPI_CALL skTryGetJsonBooleanElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkBool32*                             pBoolean
);

// Element Access

SkJsonValue SKAPI_CALL skGetJsonValueElement(
  SkJsonArray                           array,
  uint32_t                              index
);

SkJsonObject SKAPI_CALL skGetJsonObjectElement(
  SkJsonArray                           array,
  uint32_t                              index
);

SkJsonArray SKAPI_CALL skGetJsonArrayElement(
  SkJsonArray                           array,
  uint32_t                              index
);

double SKAPI_CALL skGetJsonNumberElement(
  SkJsonArray                           array,
  uint32_t                              index
);

char const* SKAPI_CALL skGetJsonStringElement(
  SkJsonArray                           array,
  uint32_t                              index
);

SkBool32 SKAPI_CALL skGetJsonBooleanElement(
  SkJsonArray                           array,
  uint32_t                              index
);

#endif // OPENSK_DEV_JSON_H
