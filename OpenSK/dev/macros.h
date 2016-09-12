/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_MACROS_H
#define   OPENSK_DEV_MACROS_H 1

#include <stdlib.h>

#define SKALLOC(in, size, scope) (in) ? in->pfnAllocation(in->pUserData, size, 0, scope) : malloc(size)
#define SKFREE(in, ptr) if (in) { in->pfnFree(in->pUserData, ptr); } else { free(ptr); }
#define SKMIN(a, b) (((a) < (b)) ? (a) : (b))
#define SKMAX(a, b) (((a) > (b)) ? (a) : (b))
#define SKISNUM(a) (((a) >= '0') && ((a) <= '9'))

#endif // OPENSK_DEV_MACROS_H
