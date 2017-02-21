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
 * List of macros for OpenSK Utilities to utilize.
 ******************************************************************************/

#include <stdio.h>  // fprintf

// Stringizing macros
#define __SKSTR(a) #a
#define _SKSTR(a) __SKSTR(a)
#define SKSTR(a) _SKSTR(a)

// Min/Max macros
#define SKMAX(a, b) (((a) > (b)) ? (a) : (b))
#define SKMIN(a, b) (((a) < (b)) ? (a) : (b))

// Reporting macros
#define SKERR(...) do { fprintf(stderr, "error: " __VA_ARGS__); fputc('\n', stderr); } while (0)
#define SKWRN(...) do { fprintf(stderr, "warn : " __VA_ARGS__); fputc('\n', stderr); } while (0)
