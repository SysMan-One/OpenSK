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
 * OpenSK standard include header.
 ******************************************************************************/
#ifndef   OPENSK_PLATFORM_H
#define   OPENSK_PLATFORM_H 1

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

#if defined(_MSC_VER)
# if   defined(SK_EXPORT)
#  define SKAPI_ATTR __declspec(dllexport)
# elif defined(SK_IMPORT)
#  define SKAPI_ATTR __declspec(dllimport)
# else
#  define SKAPI_ATTR extern
# endif
# define SKAPI_LOCAL
# define SKAPI_CALL __stdcall
# define SKAPI_PTR  __stdcall
#elif defined(__GNUC__)
# define SKAPI_ATTR __attribute__((visibility("default")))
# define SKAPI_CALL
# define SKAPI_PTR
#else
# define SKAPI_ATTR
# define SKAPI_CALL
# define SKAPI_PTR
#endif

// C99
#include <stdint.h>
#include <stddef.h>

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_PLATFORM_H
