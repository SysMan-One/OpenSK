/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK standard include header. (Platform-specific header information)
 ******************************************************************************/
#ifndef   OPENSK_PLATFORM_H
#define   OPENSK_PLATFORM_H 1

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

// Unix
#define SKAPI_ATTR
#define SKAPI_CALL
#define SKAPI_PTR

// C99
#include <stdint.h>
#include <stddef.h>

#define PRIskQ PRIu64

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_PLATFORM_H
