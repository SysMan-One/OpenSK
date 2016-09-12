/*******************************************************************************
 * OpenSK (developer) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK developer header. (Will not be present in final package, internal.)
 ******************************************************************************/
#ifndef   OPENSK_DEV_ASSERT_H
#define   OPENSK_DEV_ASSERT_H 1

// C99
#include <stdio.h> // fprintf

#define SKNOTE(...)     fprintf(stderr, "INFO: " __VA_ARGS__)
#define SKWARN(...)     fprintf(stderr, "WARN: " __VA_ARGS__)
#define SKERROR(...)    do { SKWARN(__VA_ARGS__); abort(); } while (0)
#define SKWARNIF(check, ...)                                                  \
do {                                                                          \
  if (check) {                                                                \
    SKWARN(__VA_ARGS__);                                                      \
  }                                                                           \
} while (0)
#define SKASSERT(check, ...)                                                  \
do {                                                                          \
  if (!(check)) {                                                             \
    SKERROR(__VA_ARGS__);                                                     \
  }                                                                           \
} while (0)

#endif // OPENSK_DEV_ASSERT_H
