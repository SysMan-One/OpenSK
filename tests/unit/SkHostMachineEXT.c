/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Unit tests for SkHostMachineEXT
 ******************************************************************************/

#include "cmocka.h"
#include <string.h>
#include <stdlib.h>
#include <OpenSK/ext/host_machine.h>
#define SUITE_NAME SkHostMachineEXT

////////////////////////////////////////////////////////////////////////////////
// Global State
////////////////////////////////////////////////////////////////////////////////

static SkHostMachineEXT hostMachine;

////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////

UNIT_TEST(skNextHostMachinePropertyEXT) {
  char const* propertyName;

  // Test cycling through the properties.
  propertyName = NULL;
  while ((propertyName = skNextHostMachinePropertyEXT(hostMachine, propertyName))) {
    assert_non_null(skGetHostMachinePropertyEXT(hostMachine, propertyName));
  }

}

#define skHostMachineCleanPathTest(path, result)                              \
strcpy(pBuffer, path);                                                        \
  assert_string_equal(skCleanHostMachinePathEXT(hostMachine, pBuffer), result)

UNIT_TEST(skCleanHostMachinePathEXT) {
  char pBuffer[512];

  // Test Batch: Single-Dot => Empty Path
  skHostMachineCleanPathTest("", "");
  skHostMachineCleanPathTest(".", "");
  skHostMachineCleanPathTest("./", "");
  skHostMachineCleanPathTest("./.", "");
  skHostMachineCleanPathTest("././", "");
  skHostMachineCleanPathTest("././.", "");

  // Test Batch: Simple Path => Simple Path
  skHostMachineCleanPathTest("path", "path");
  skHostMachineCleanPathTest("path/", "path");
  skHostMachineCleanPathTest("path/to", "path/to");
  skHostMachineCleanPathTest("path/to/", "path/to");
  skHostMachineCleanPathTest("path/to/path", "path/to/path");
  skHostMachineCleanPathTest("path/to/path/", "path/to/path");

  // Test Batch: Dot Path => Dot Path
  skHostMachineCleanPathTest(".path", ".path");
  skHostMachineCleanPathTest(".path/", ".path");
  skHostMachineCleanPathTest(".path/to.", ".path/to.");
  skHostMachineCleanPathTest(".path/to./", ".path/to.");
  skHostMachineCleanPathTest(".path/to./.path.", ".path/to./.path.");
  skHostMachineCleanPathTest(".path/to./.path./", ".path/to./.path.");

  // Test Batch: Double-Dot => Empty Path
  skHostMachineCleanPathTest("path/..", "");
  skHostMachineCleanPathTest("path/../", "");
  skHostMachineCleanPathTest("path/to/../..", "");
  skHostMachineCleanPathTest("path/to/../../", "");
  skHostMachineCleanPathTest("path/to/path/../../..", "");
  skHostMachineCleanPathTest("path/to/path/../../../", "");

  // Test Batch: Single-Dot, Double-Dot => Empty Path
  skHostMachineCleanPathTest("./path/..", "");
  skHostMachineCleanPathTest("./path/../", "");
  skHostMachineCleanPathTest("path/./..", "");
  skHostMachineCleanPathTest("path/./../", "");
  skHostMachineCleanPathTest("path/../.", "");
  skHostMachineCleanPathTest("path/.././", "");
  skHostMachineCleanPathTest("./path/to/../..", "");
  skHostMachineCleanPathTest("./path/to/../../", "");
  skHostMachineCleanPathTest("path/./to/../..", "");
  skHostMachineCleanPathTest("path/./to/../../", "");
  skHostMachineCleanPathTest("path/to/./../..", "");
  skHostMachineCleanPathTest("path/to/./../../", "");
  skHostMachineCleanPathTest("path/to/.././..", "");
  skHostMachineCleanPathTest("path/to/.././../", "");
  skHostMachineCleanPathTest("path/to/../../.", "");
  skHostMachineCleanPathTest("path/to/../.././", "");

  // Test Batch: Single-Dot, Double-Dot, Dot-Directories => Empty-Path
  skHostMachineCleanPathTest("./.path/..", "");
  skHostMachineCleanPathTest("./.path/../", "");
  skHostMachineCleanPathTest(".path/./..", "");
  skHostMachineCleanPathTest(".path/./../", "");
  skHostMachineCleanPathTest(".path/../.", "");
  skHostMachineCleanPathTest(".path/.././", "");
  skHostMachineCleanPathTest("./.path/to./../..", "");
  skHostMachineCleanPathTest("./.path/to./../../", "");
  skHostMachineCleanPathTest(".path/./to./../..", "");
  skHostMachineCleanPathTest(".path/./to./../../", "");
  skHostMachineCleanPathTest(".path/to././../..", "");
  skHostMachineCleanPathTest(".path/to././../../", "");
  skHostMachineCleanPathTest(".path/to./.././..", "");
  skHostMachineCleanPathTest(".path/to./.././../", "");
  skHostMachineCleanPathTest(".path/to./../../.", "");
  skHostMachineCleanPathTest(".path/to./../.././", "");

  // Test Batch: Double-Dot => Double-Dot
  skHostMachineCleanPathTest("..", "..");
  skHostMachineCleanPathTest("../", "..");
  skHostMachineCleanPathTest("../..", "../..");
  skHostMachineCleanPathTest("../../", "../..");
  skHostMachineCleanPathTest("../../..", "../../..");
  skHostMachineCleanPathTest("../../../", "../../..");

  // Test Batch: Path, Double-Dot => Double-Dot
  skHostMachineCleanPathTest("path/../..", "..");
  skHostMachineCleanPathTest("path/../../", "..");
  skHostMachineCleanPathTest("path/../../..", "../..");
  skHostMachineCleanPathTest("path/../../../", "../..");
  skHostMachineCleanPathTest("path/../../../..", "../../..");
  skHostMachineCleanPathTest("path/../../../../", "../../..");
  skHostMachineCleanPathTest("path/../to/../..", "..");
  skHostMachineCleanPathTest("path/../to/../../", "..");
  skHostMachineCleanPathTest("path/../to/../../..", "../..");
  skHostMachineCleanPathTest("path/../to/../../../", "../..");
  skHostMachineCleanPathTest("path/../to/../../../..", "../../..");
  skHostMachineCleanPathTest("path/../to/../../../../", "../../..");
  skHostMachineCleanPathTest("path/../to/path/../../..", "..");
  skHostMachineCleanPathTest("path/../to/path/../../../", "..");
  skHostMachineCleanPathTest("path/../to/path/../../../..", "../..");
  skHostMachineCleanPathTest("path/../to/path/../../../../", "../..");
  skHostMachineCleanPathTest("path/../to/path/../../../../..", "../../..");
  skHostMachineCleanPathTest("path/../to/path/../../../../../", "../../..");

  // Test Batch: Absolute Path => Absolute Path
  skHostMachineCleanPathTest("/", "/");
  skHostMachineCleanPathTest("/.", "/");
  skHostMachineCleanPathTest("/./", "/");
  skHostMachineCleanPathTest("/./.", "/");
  skHostMachineCleanPathTest("/././", "/");

  // Test Batch: Absolute Path, Double-Dot => Absolute Path, Double-Dot
  skHostMachineCleanPathTest("/..", "/..");
  skHostMachineCleanPathTest("/../", "/..");
  skHostMachineCleanPathTest("/../..", "/../..");
  skHostMachineCleanPathTest("/../../", "/../..");
  skHostMachineCleanPathTest("/../../..", "/../../..");
  skHostMachineCleanPathTest("/../../../", "/../../..");

  // Test Batch: Absolute Dot Path, Single-Dot => Absolute Dot Path
  skHostMachineCleanPathTest("/.path/.", "/.path");
  skHostMachineCleanPathTest("/.path/./", "/.path");
  skHostMachineCleanPathTest("/.path/./.to", "/.path/.to");
  skHostMachineCleanPathTest("/.path/./.to/.", "/.path/.to");
  skHostMachineCleanPathTest("/.path/./.to/././path.", "/.path/.to/path.");
  skHostMachineCleanPathTest("/.path/./.to/././path./", "/.path/.to/path.");
  skHostMachineCleanPathTest("/.path/./.to/././path./.", "/.path/.to/path.");
  skHostMachineCleanPathTest("/.path/./.to/././path././", "/.path/.to/path.");

  // Test Batch: Absolute Path, Double-Dot => Absolute Path
  skHostMachineCleanPathTest("/path/to/..", "/path");
  skHostMachineCleanPathTest("/path/to/../", "/path");
  skHostMachineCleanPathTest("/path/to/path/..", "/path/to");
  skHostMachineCleanPathTest("/path/to/path/../", "/path/to");
  skHostMachineCleanPathTest("/path/to/../path", "/path/path");
  skHostMachineCleanPathTest("/path/to/../path/", "/path/path");
  skHostMachineCleanPathTest("/path/../to/path", "/to/path");
  skHostMachineCleanPathTest("/path/../to/path/", "/to/path");
  skHostMachineCleanPathTest("/path/../to/path/..", "/to");
  skHostMachineCleanPathTest("/path/../to/path/../", "/to");

  // Test Batch: Absolute Path, Double-Dot => Absolute Path/Double-Dot
  skHostMachineCleanPathTest("/../path/to/..", "/../path");
  skHostMachineCleanPathTest("/../path/to/../", "/../path");
  skHostMachineCleanPathTest("/path/../../to/path/..", "/../to");
  skHostMachineCleanPathTest("/path/../../to/path/../", "/../to");
  skHostMachineCleanPathTest("/path/../../../to/path/..", "/../../to");
  skHostMachineCleanPathTest("/path/../../../to/path/../", "/../../to");
  skHostMachineCleanPathTest("/path/to/../path/../../..", "/..");
  skHostMachineCleanPathTest("/path/to/../path/../../../", "/..");
  skHostMachineCleanPathTest("/path/to/../path/../../../..", "/../..");
  skHostMachineCleanPathTest("/path/to/../path/../../../../", "/../..");

  // Test Batch: Server Path => Server Path
  skHostMachineCleanPathTest("//", "//");
  skHostMachineCleanPathTest("//.", "//");
  skHostMachineCleanPathTest("//./", "//");
  skHostMachineCleanPathTest("//./.", "//");
  skHostMachineCleanPathTest("//././", "//");

  // Test Batch: Server Path, Double-Dot => Server Path, Double-Dot
  skHostMachineCleanPathTest("//..", "//..");
  skHostMachineCleanPathTest("//../", "//..");
  skHostMachineCleanPathTest("//../..", "//../..");
  skHostMachineCleanPathTest("//../../", "//../..");
  skHostMachineCleanPathTest("//../../..", "//../../..");
  skHostMachineCleanPathTest("//../../../", "//../../..");

  // Test Batch: Server Dot Path, Single-Dot => Server Dot Path
  skHostMachineCleanPathTest("//.path/.", "//.path");
  skHostMachineCleanPathTest("//.path/./", "//.path");
  skHostMachineCleanPathTest("//.path/./.to", "//.path/.to");
  skHostMachineCleanPathTest("//.path/./.to/.", "//.path/.to");
  skHostMachineCleanPathTest("//.path/./.to/././path.", "//.path/.to/path.");
  skHostMachineCleanPathTest("//.path/./.to/././path./", "//.path/.to/path.");
  skHostMachineCleanPathTest("//.path/./.to/././path./.", "//.path/.to/path.");
  skHostMachineCleanPathTest("//.path/./.to/././path././", "//.path/.to/path.");

  // Test Batch: Server Path, Double-Dot => Server Path
  skHostMachineCleanPathTest("//path/to/..", "//path");
  skHostMachineCleanPathTest("//path/to/../", "//path");
  skHostMachineCleanPathTest("//path/to/path/..", "//path/to");
  skHostMachineCleanPathTest("//path/to/path/../", "//path/to");
  skHostMachineCleanPathTest("//path/to/../path", "//path/path");
  skHostMachineCleanPathTest("//path/to/../path/", "//path/path");
  skHostMachineCleanPathTest("//path/../to/path", "//to/path");
  skHostMachineCleanPathTest("//path/../to/path/", "//to/path");
  skHostMachineCleanPathTest("//path/../to/path/..", "//to");
  skHostMachineCleanPathTest("//path/../to/path/../", "//to");

  // Test Batch: Server Path, Double-Dot => Server Path/Double-Dot
  skHostMachineCleanPathTest("//../path/to/..", "//../path");
  skHostMachineCleanPathTest("//../path/to/../", "//../path");
  skHostMachineCleanPathTest("//path/../../to/path/..", "//../to");
  skHostMachineCleanPathTest("//path/../../to/path/../", "//../to");
  skHostMachineCleanPathTest("//path/../../../to/path/..", "//../../to");
  skHostMachineCleanPathTest("//path/../../../to/path/../", "//../../to");
  skHostMachineCleanPathTest("//path/to/../path/../../..", "//..");
  skHostMachineCleanPathTest("//path/to/../path/../../../", "//..");
  skHostMachineCleanPathTest("//path/to/../path/../../../..", "//../..");
  skHostMachineCleanPathTest("//path/to/../path/../../../../", "//../..");

  // Test Batch: Device Dot Path, Single-Dot => Device Dot Path
  skHostMachineCleanPathTest("sk:/.path/.", "sk:/.path");
  skHostMachineCleanPathTest("sk:/.path/./", "sk:/.path");
  skHostMachineCleanPathTest("sk:/.path/./.to", "sk:/.path/.to");
  skHostMachineCleanPathTest("sk:/.path/./.to/.", "sk:/.path/.to");
  skHostMachineCleanPathTest("sk:/.path/./.to/././path.", "sk:/.path/.to/path.");
  skHostMachineCleanPathTest("sk:/.path/./.to/././path./", "sk:/.path/.to/path.");
  skHostMachineCleanPathTest("sk:/.path/./.to/././path./.", "sk:/.path/.to/path.");
  skHostMachineCleanPathTest("sk:/.path/./.to/././path././", "sk:/.path/.to/path.");

  // Test Batch: Device Path, Double-Dot => Device Path
  skHostMachineCleanPathTest("sk:/path/to/..", "sk:/path");
  skHostMachineCleanPathTest("sk:/path/to/../", "sk:/path");
  skHostMachineCleanPathTest("sk:/path/to/path/..", "sk:/path/to");
  skHostMachineCleanPathTest("sk:/path/to/path/../", "sk:/path/to");
  skHostMachineCleanPathTest("sk:/path/to/../path", "sk:/path/path");
  skHostMachineCleanPathTest("sk:/path/to/../path/", "sk:/path/path");
  skHostMachineCleanPathTest("sk:/path/../to/path", "sk:/to/path");
  skHostMachineCleanPathTest("sk:/path/../to/path/", "sk:/to/path");
  skHostMachineCleanPathTest("sk:/path/../to/path/..", "sk:/to");
  skHostMachineCleanPathTest("sk:/path/../to/path/../", "sk:/to");

  // Test Batch: Device Path, Double-Dot => Absolute Device Path/Double-Dot
  skHostMachineCleanPathTest("sk:/../path/to/..", "sk:/../path");
  skHostMachineCleanPathTest("sk:/../path/to/../", "sk:/../path");
  skHostMachineCleanPathTest("sk:/path/../../to/path/..", "sk:/../to");
  skHostMachineCleanPathTest("sk:/path/../../to/path/../", "sk:/../to");
  skHostMachineCleanPathTest("sk:/path/../../../to/path/..", "sk:/../../to");
  skHostMachineCleanPathTest("sk:/path/../../../to/path/../", "sk:/../../to");
  skHostMachineCleanPathTest("sk:/path/to/../path/../../..", "sk:/..");
  skHostMachineCleanPathTest("sk:/path/to/../path/../../../", "sk:/..");
  skHostMachineCleanPathTest("sk:/path/to/../path/../../../..", "sk:/../..");
  skHostMachineCleanPathTest("sk:/path/to/../path/../../../../", "sk:/../..");

  // Test Batch: Absolute Device Dot Path, Single-Dot => Absolute Device Dot Path
  skHostMachineCleanPathTest("sk://.path/.", "sk://.path");
  skHostMachineCleanPathTest("sk://.path/./", "sk://.path");
  skHostMachineCleanPathTest("sk://.path/./.to", "sk://.path/.to");
  skHostMachineCleanPathTest("sk://.path/./.to/.", "sk://.path/.to");
  skHostMachineCleanPathTest("sk://.path/./.to/././path.", "sk://.path/.to/path.");
  skHostMachineCleanPathTest("sk://.path/./.to/././path./", "sk://.path/.to/path.");
  skHostMachineCleanPathTest("sk://.path/./.to/././path./.", "sk://.path/.to/path.");
  skHostMachineCleanPathTest("sk://.path/./.to/././path././", "sk://.path/.to/path.");

  // Test Batch: Absolute Device Path, Double-Dot => Absolute Device Path
  skHostMachineCleanPathTest("sk://path/to/..", "sk://path");
  skHostMachineCleanPathTest("sk://path/to/../", "sk://path");
  skHostMachineCleanPathTest("sk://path/to/path/..", "sk://path/to");
  skHostMachineCleanPathTest("sk://path/to/path/../", "sk://path/to");
  skHostMachineCleanPathTest("sk://path/to/../path", "sk://path/path");
  skHostMachineCleanPathTest("sk://path/to/../path/", "sk://path/path");
  skHostMachineCleanPathTest("sk://path/../to/path", "sk://to/path");
  skHostMachineCleanPathTest("sk://path/../to/path/", "sk://to/path");
  skHostMachineCleanPathTest("sk://path/../to/path/..", "sk://to");
  skHostMachineCleanPathTest("sk://path/../to/path/../", "sk://to");

  // Test Batch: Absolute Device Path, Double-Dot => Absolute Device Path/Double-Dot
  skHostMachineCleanPathTest("sk://../path/to/..", "sk://../path");
  skHostMachineCleanPathTest("sk://../path/to/../", "sk://../path");
  skHostMachineCleanPathTest("sk://path/../../to/path/..", "sk://../to");
  skHostMachineCleanPathTest("sk://path/../../to/path/../", "sk://../to");
  skHostMachineCleanPathTest("sk://path/../../../to/path/..", "sk://../../to");
  skHostMachineCleanPathTest("sk://path/../../../to/path/../", "sk://../../to");
  skHostMachineCleanPathTest("sk://path/to/../path/../../..", "sk://..");
  skHostMachineCleanPathTest("sk://path/to/../path/../../../", "sk://..");
  skHostMachineCleanPathTest("sk://path/to/../path/../../../..", "sk://../..");
  skHostMachineCleanPathTest("sk://path/to/../path/../../../../", "sk://../..");

}

UNIT_TEST(skResolveHostMachinePathEXT) {
  char* pChar;
  size_t sizeT;
  char* pCharComparison;
  char const* pCharConst;

  // Absolute Path
  pCharConst = skGetHostMachinePropertyEXT(hostMachine, SKEXT_HOST_MACHINE_PROPERTY_EXE_PATH);
  assert_int_equal(skResolveHostMachinePathEXT(hostMachine, "/path/shouldnt/exist", &pChar), SK_ERROR_NOT_FOUND);
  assert_int_equal(skResolveHostMachinePathEXT(hostMachine, pCharConst, &pChar), SK_SUCCESS);
  assert_string_equal(pChar, pCharConst);

  // Relative Path (Working Directory)
  pCharConst = skGetHostMachinePropertyEXT(hostMachine, SKEXT_HOST_MACHINE_PROPERTY_EXE_DIRECTORY);
  if (pCharConst) {
    sizeT = strlen(pCharConst);
    pCharComparison = malloc(sizeT + 13 + 1);
    strcpy(pCharComparison, pCharConst);
    strcpy(&pCharComparison[sizeT], "/res/null.sks");
    assert_int_equal(skResolveHostMachinePathEXT(hostMachine, "null.sks", &pChar), SK_ERROR_NOT_FOUND);
    assert_int_equal(skResolveHostMachinePathEXT(hostMachine, "res/null.sks", &pChar), SK_SUCCESS);
    assert_string_equal(pChar, pCharComparison);
    free(pCharComparison);
  }

}

////////////////////////////////////////////////////////////////////////////////
// Suite Definition
////////////////////////////////////////////////////////////////////////////////

SUITE_INITIALIZE() {
  return 0;
}

SUITE_SETUP() {
  skCreateHostMachineEXT(NULL, &hostMachine, _argc, _argv);
  return 0;
}

SUITE_TEARDOWN() {
  skDestroyHostMachineEXT(hostMachine);
  return 0;
}

SUITE_DEINITIALIZE() {
  return 0;
}

BEGIN_REGISTER()
  ADD_TEST(skNextHostMachinePropertyEXT),
  ADD_TEST(skCleanHostMachinePathEXT),
  ADD_TEST(skResolveHostMachinePathEXT)
END_REGISTER()
