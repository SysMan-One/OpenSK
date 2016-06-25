/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_ASSERT_H
#define   OPENSK_DEV_ASSERT_H 1

#include <stdio.h> // fprintf

#define SKWARN(...) fputs(__FUNCTION__, stderr); fprintf(stderr, ": " __VA_ARGS__)
#define SKERROR(...) SKWARN(__VA_ARGS__); abort()
#define SKWARNIF(check, ...) do { if (check) { SKWARN(__VA_ARGS__); } } while (0)
#define SKASSERT(check, ...) do { if (!(check)) { SKERROR(__VA_ARGS__); } } while (0)

#endif // OPENSK_DEV_ASSERT_H
