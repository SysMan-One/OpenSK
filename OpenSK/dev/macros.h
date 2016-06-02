/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_MACROS_H
#define   OPENSK_DEV_MACROS_H 1

#define SKALLOC(size, scope) pAllocator->pfnAllocation(pAllocator->pUserData, size, 0, scope)
#define _SKFREE(in, ptr) in->pfnFree(in->pUserData, ptr)
#define SKFREE(ptr) _SKFREE(pAllocator, ptr)

#endif // OPENSK_DEV_MACROS_H
