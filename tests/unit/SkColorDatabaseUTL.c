/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Unit tests for SkColorDatabaseUTL
 ******************************************************************************/

#include "cmocka.h"
#include <string.h>
#include <OpenSK/utl/color_database.h>
#define SUITE_NAME SkColorDatabaseUTL

////////////////////////////////////////////////////////////////////////////////
// Global State
////////////////////////////////////////////////////////////////////////////////

static SkColorDatabaseUTL colorDatabase;

////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////

UNIT_TEST(skColorDatabaseConfigCountUTL) {
  SkColorKindUTL colorKind;

  // Check that the empty color database is actually empty
  assert_int_equal(skColorDatabaseConfigCountUTL(colorDatabase), 0);
  assert_null(skColorDatabaseConfigNextUTL(colorDatabase, NULL));
  for (colorKind = SKUTL_COLOR_KIND_NORMAL; colorKind != SKUTL_COLOR_KIND_MAX; ++colorKind) {
    assert_null(skColorDatabaseGetCodeUTL(colorDatabase, colorKind, NULL));
  }
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_INVALID, "Test"));

}

UNIT_TEST(skColorDatabaseConfigStringUTL) {

  // Check a simple addition to HOSTAPI
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL));
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "HOSTAPI 01"), SK_SUCCESS);
  assert_non_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "01");
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));

  // Check a simple addition to VIRTUAL DEVICE
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "01");
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "VDEVICE 01;02"), SK_SUCCESS);
  assert_non_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "01");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "01;02");
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));

  // Check an addition with multiple kinds
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "01;02");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "01");
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "HOSTAPI 03 PDEVICE 04;05"), SK_SUCCESS);
  assert_non_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "03");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "01;02");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL), "04;05");
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));

  // Check an addition with comment
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_COMPONENT, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL), "04;05");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "01;02");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "03");
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "PCOMPONENT 06 # HOSTAPI 07"), SK_SUCCESS);
  assert_non_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_COMPONENT, NULL));
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "03");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "01;02");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL), "04;05");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_COMPONENT, NULL), "06");
  assert_null(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_NORMAL, NULL));

  // Check that a comment line compiles
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "# This is a comment"), SK_SUCCESS);

  // Check that an empty line compiles
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, ""), SK_SUCCESS);

  // Check something that shouldn't compile
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "HOSTAPI"), SK_ERROR_INVALID);
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "asdsad asdasdasd"), SK_ERROR_INVALID);
  assert_int_equal(skColorDatabaseConfigStringUTL(colorDatabase, "01;02"), SK_ERROR_INVALID);
}

UNIT_TEST(skColorDatabaseConfigFileUTL) {
  size_t idx;
  SkBool32 found;
  SkResult result;
  char const *loadedConfig;
  char const* filenames[] = {
    "res/SKCOLORS.empty",
    "res/SKCOLORS.comment",
    "res/SKCOLORS.single",
    "res/SKCOLORS.multi",
    "res/SKCOLORS.long"
  };

  // Test loading different kinds of files
  assert_int_equal(skColorDatabaseConfigFileUTL(colorDatabase, "SKCOLORS.invalid"), SK_ERROR_NOT_FOUND);
  for (idx = 0; idx < sizeof(filenames) / sizeof(filenames[0]); ++idx) {
    result = skColorDatabaseConfigFileUTL(colorDatabase, filenames[idx]);
    if (result != SK_SUCCESS) {
      fail_msg("Failed to load configuration: %s (%s)\n", filenames[idx], skGetResultString(result));
    }
  }

  // Check that the metadata is valid, and files are loaded
  assert_int_equal(skColorDatabaseConfigCountUTL(colorDatabase), 5);
  for (idx = 0; idx < sizeof(filenames) / sizeof(filenames[0]); ++idx) {
    found = SK_FALSE;
    loadedConfig = NULL;
    while ((loadedConfig = skColorDatabaseConfigNextUTL(colorDatabase, loadedConfig))) {
      if (strcmp(loadedConfig, filenames[idx]) == 0) {
        found = SK_TRUE;
      }
    }
    assert_true(found);
  }

  // Check that the files actually change the internal state of the database
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_HOSTAPI, NULL), "01");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_VIRTUAL_DEVICE, NULL), "02");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_DEVICE, NULL), "03;04");
  assert_string_equal(skColorDatabaseGetCodeUTL(colorDatabase, SKUTL_COLOR_KIND_PHYSICAL_COMPONENT, NULL), "05");

}

////////////////////////////////////////////////////////////////////////////////
// Suite Definition
////////////////////////////////////////////////////////////////////////////////

SUITE_INITIALIZE() {
  return 0;
}

SUITE_SETUP() {
  SkResult result;
  result = skCreateColorDatabaseEmptyUTL(NULL, &colorDatabase);
  return (result != SK_SUCCESS);
}

SUITE_TEARDOWN() {
  skDestroyColorDatabaseUTL(colorDatabase);
  return 0;
}

SUITE_DEINITIALIZE() {
  return 0;
}

BEGIN_REGISTER()
  ADD_TEST(skColorDatabaseConfigCountUTL),
  ADD_TEST(skColorDatabaseConfigStringUTL),
  ADD_TEST(skColorDatabaseConfigFileUTL)
END_REGISTER()
