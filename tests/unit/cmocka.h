/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A helper header for defining convenience macros for cmocka.
 ******************************************************************************/
#ifndef OPENSK_CMOCKA_H
#define OPENSK_CMOCKA_H 1

// C99
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

// cmocka
#include <cmocka.h>

#define _GENERATE_FUNCTION_DECL(prefix, suite, postfix) prefix##_##suite##_##postfix

#define GENERATE_FUNCTION_DECL(prefix, suite, postfix) _GENERATE_FUNCTION_DECL(prefix, suite, postfix)

#define _GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix) _##prefix##_##suite##_##postfix

#define GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)             \
_GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)

#define GENERATE_REDIRECTION_FUNCTION(prefix, suite, postfix)                 \
static void GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)();      \
static void GENERATE_FUNCTION_DECL(prefix, suite, postfix)(void**state) {     \
  (void)state; GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)();   \
}                                                                             \
static void GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)()

#define GENERATE_REDIRECTION_FUNCTION_RET(ret, prefix, suite, postfix)        \
static ret GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)();       \
static ret GENERATE_FUNCTION_DECL(prefix, suite, postfix)(void**state) {      \
  (void)state;                                                                \
  return GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)();         \
}                                                                             \
static ret GENERATE_REDIRECTED_FUNCTION_DECL(prefix, suite, postfix)()

#define SUITE_SETUP() GENERATE_REDIRECTION_FUNCTION_RET(int, suite, SUITE_NAME, SetUp)

#define SUITE_TEARDOWN() GENERATE_REDIRECTION_FUNCTION_RET(int, suite, SUITE_NAME, TearDown)

#define SUITE_INITIALIZE() GENERATE_REDIRECTION_FUNCTION_RET(int, suite, SUITE_NAME, Initialize)

#define SUITE_DEINITIALIZE() GENERATE_REDIRECTION_FUNCTION_RET(int, suite, SUITE_NAME, Deinitialize)

#define UNIT_TEST(test) GENERATE_REDIRECTION_FUNCTION(unit, SUITE_NAME, test)

#define ADD_TEST(test)                                                        \
{                                                                             \
  #test,                                                                      \
  GENERATE_FUNCTION_DECL(unit, SUITE_NAME, test),                             \
  GENERATE_FUNCTION_DECL(suite, SUITE_NAME, SetUp),                           \
  GENERATE_FUNCTION_DECL(suite, SUITE_NAME, TearDown)                         \
}

#define BEGIN_REGISTER() static struct CMUnitTest const _tests[] = {

#define END_REGISTER()                                                        \
};                                                                            \
int                                                                           \
GENERATE_FUNCTION_DECL(suite,SUITE_NAME,Runner)(int argc, char const** argv) {\
  _argc = argc; _argv = argv;                                                 \
  return cmocka_run_group_tests(                                              \
    _tests,                                                                   \
    GENERATE_FUNCTION_DECL(suite,SUITE_NAME,Initialize),                      \
    GENERATE_FUNCTION_DECL(suite,SUITE_NAME,Deinitialize)                     \
  );                                                                          \
}

static int _argc;

static char const** _argv;

#endif //OPENSK_CMOCKA_H
