/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK test application (runs unit tests to check functionality).
 ******************************************************************************/

// C99
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>

// cmocka
#include <cmocka.h>

#define SK_TEST(prettyName, identifier) \
int suite_##identifier##_Runner(int argc, char const* argv[]);
#include "unit_suites.inc"
#undef SK_TEST

static int should_run(char const* suite, char const* name) {
  return (!name || strcmp(suite, name) == 0);
}

#define SK_TEST(prettyName, identifier) \
if (should_run(prettyName, name)) {\
  print_message("[##########] Begin Suite: %s\n", prettyName);\
  *errors += suite_##identifier##_Runner(argc, argv);\
  print_message("[##########] End Suite\n");\
  ++*suites;\
}
static void run_selective_tests(char const* name, int* errors, int* suites, int argc, char const* argv[]) {
#include "unit_suites.inc"
}
#undef SK_TEST

#define SK_TEST(prettyName, identifier) printf("%s\n", prettyName);
static void print_list() {
#include "unit_suites.inc"
}
#undef SK_TEST

static void print_help() {
  printf(
    "Usage: sktest [options] [testName1,...,testNameN]\n"
    " Runs OpenSK unit test to ensure the build stability/validity on the host machine.\n"
    " By not providing a test name, sktest will simply run all of the possible tests.\n"
    "\n"
    "Options:\n"
    "  -h, --help   Prints this help documentation.\n"
    "  -l, --list   Lists all of the possible tests to run.\n"
  );
}

int main(int argc, char const* argv[]) {
  int idx;
  int errors = 0;
  int suites = 0;
  int previous;
  if (argc <= 1) {
    run_selective_tests(NULL, &errors, &suites, argc, argv);
  }
  else {
    for (idx = 1; idx < argc; ++idx) {
      if (argv[idx][0] == '-') {
        if (strcmp(argv[idx], "-h") == 0 || strcmp(argv[idx], "--help") == 0) {
          print_help();
          return 0;
        }
        else if (strcmp(argv[idx], "-l") == 0 || strcmp(argv[idx], "--list") == 0) {
          print_list();
          return 0;
        }
        else {
          print_error("ERROR: Invalid flag provided! (%s)\n", argv[idx]);
          print_help();
          return -1;
        }
      }
      else {
        previous = suites;
        run_selective_tests(argv[idx], &errors, &suites, argc, argv);
        if (suites == previous) {
          print_error("WARNING: No suites/tests were ran for '%s'! (Check the filter string)\n", argv[idx]);
        }
      }
    }
  }
  if (!suites) {
    return -1;
  }
  return errors;
}
