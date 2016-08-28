/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_MACROS_H
#define   OPENSK_DEV_MACROS_H 1

#define SKALLOC(in, size, scope) in->pfnAllocation(in->pUserData, size, 0, scope)
#define SKFREE(in, ptr) in->pfnFree(in->pUserData, ptr)
#define SKMIN(a, b) (((a) < (b)) ? (a) : (b))
#define SKMAX(a, b) (((a) > (b)) ? (a) : (b))

#endif // OPENSK_DEV_MACROS_H
