/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Unit tests for SkStringUTL
 ******************************************************************************/

#include "cmocka.h"
#include <OpenSK/utl/string.h>
#define SUITE_NAME SkStringUTL

////////////////////////////////////////////////////////////////////////////////
// Global State
////////////////////////////////////////////////////////////////////////////////

static SkStringUTL string;
static SkStringUTL A;
static SkStringUTL B;

static void
generateStringOfLength(char *pBuffer, size_t n) {
  char start = 'A';
  while (n) {
    *pBuffer = start;
    ++start;
    ++pBuffer;
    --n;
  }
  *pBuffer = 0;
}

static void
appendStringOfLength(char *pBuffer, size_t n) {
  while (*pBuffer) {
    ++pBuffer;
  }
  generateStringOfLength(pBuffer, n);
}

////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////

UNIT_TEST(skCheckParamUTL) {
  assert_true(skCStrCompareUTL("", ""));
  assert_true(skCheckParamUTL("A", "A"));
  assert_false(skCheckParamUTL("A", "B"));
  assert_true(skCheckParamUTL("A", "B", "A"));
  assert_false(skCheckParamUTL("A", "B", "AA"));
  assert_true(skCheckParamUTL("A", "B", "AA", "A"));
  assert_false(skCheckParamUTL("A", "B", "AA", "a"));
  assert_true(skCheckParamUTL("A", "B", "AA", "a", "A"));
}

UNIT_TEST(skCheckParamBeginsUTL) {
  assert_true(skCheckParamBeginsUTL("", ""));
  assert_true(skCheckParamBeginsUTL("A", "A"));
  assert_false(skCheckParamBeginsUTL("A", "B"));
  assert_true(skCheckParamBeginsUTL("A", "B", "A"));
  assert_false(skCheckParamBeginsUTL("A", "B", "AA"));
  assert_true(skCheckParamBeginsUTL("A", "B", "AA", "A"));
  assert_false(skCheckParamBeginsUTL("A", "B", "AA", "a"));
  assert_false(skCheckParamBeginsUTL("A", "B", "aa", "a"));
  assert_true(skCheckParamBeginsUTL("ABCD", "ABCD"));
  assert_true(skCheckParamBeginsUTL("ABCD", "AB"));
  assert_false(skCheckParamBeginsUTL("ABCD", "abcd", "ABCd", "aBCD", "BCD"));
}

UNIT_TEST(skCStrCompareUTL) {
  assert_true(skCStrCompareUTL("", ""));
  assert_false(skCStrCompareUTL("a", ""));
  assert_false(skCStrCompareUTL("", "a"));
  assert_false(skCStrCompareUTL("s", "a"));
  assert_false(skCStrCompareUTL("A", "a"));
  assert_true(skCStrCompareUTL("A", "A"));
  assert_false(skCStrCompareUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMN0PQRSTUVWXYZ"));
  assert_true(skCStrCompareUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
}

UNIT_TEST(skCStrCompareBeginsUTL) {
  assert_true(skCStrCompareBeginsUTL("", ""));
  assert_true(skCStrCompareBeginsUTL("a", ""));
  assert_false(skCStrCompareBeginsUTL("", "a"));
  assert_false(skCStrCompareBeginsUTL("s", "a"));
  assert_false(skCStrCompareBeginsUTL("A", "a"));
  assert_true(skCStrCompareBeginsUTL("A", "A"));
  assert_false(skCStrCompareBeginsUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMN0PQRSTUVWXYZ"));
  assert_true(skCStrCompareBeginsUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
  assert_false(skCStrCompareBeginsUTL("ABCDEFGHIJKLMN", "ABCDEFGHIJKLMN0PQRSTUVWXYZ"));
  assert_false(skCStrCompareBeginsUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMN0"));
  assert_true(skCStrCompareBeginsUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNO"));
}

UNIT_TEST(skCStrCompareCaseInsensitiveUTL) {
  assert_true(skCStrCompareCaseInsensitiveUTL("", ""));
  assert_false(skCStrCompareCaseInsensitiveUTL("a", ""));
  assert_false(skCStrCompareCaseInsensitiveUTL("", "a"));
  assert_false(skCStrCompareCaseInsensitiveUTL("s", "a"));
  assert_true(skCStrCompareCaseInsensitiveUTL("A", "a"));
  assert_true(skCStrCompareCaseInsensitiveUTL("A", "A"));
  assert_false(skCStrCompareCaseInsensitiveUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMN0PQRSTUVWXYZ"));
  assert_true(skCStrCompareCaseInsensitiveUTL("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
}

UNIT_TEST(skCreateStringUTL) {

  // Check the default settings of a SkStringUTL object...
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_non_null(skStringDataUTL(string));
  assert_true(skStringEmptyUTL(string));

}

UNIT_TEST(skStringResizeUTL) {

  // Non-allocation resize
  // Note: String resize has no effect on length unless we're shrinking
  assert_int_equal(skStringResizeUTL(string, SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);

  // Fill with data
  assert_int_equal(skStringNCopyUTL(string, "TestTest", 8), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 8);
  assert_string_equal(skStringDataUTL(string), "TestTest");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);

  // Resize to a smaller size
  // Note: Since we are "shrinking", the length is changed here.
  assert_int_equal(skStringResizeUTL(string, 6), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 6);
  assert_string_equal(skStringDataUTL(string), "TestTe");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);

  // Resize to an allocated amount
  assert_int_equal(skStringResizeUTL(string, SKUTL_STRING_DEFAULT_CAPACITY + 1), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 6);
  assert_string_equal(skStringDataUTL(string), "TestTe");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY + 1);

  // Resize to a larger allocated amount
  assert_int_equal(skStringResizeUTL(string, 10 * SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 6);
  assert_string_equal(skStringDataUTL(string), "TestTe");
  assert_int_equal(skStringCapacityUTL(string), 10 * SKUTL_STRING_DEFAULT_CAPACITY);

  // Resize back to a non-allocated amount
  // Note: The resize doesn't affect the capacity, only the string length.
  assert_int_equal(skStringResizeUTL(string, 4), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), 10 * SKUTL_STRING_DEFAULT_CAPACITY);

}

UNIT_TEST(skStringReserveUTL) {

  // Reserve sizes too small for resizing...
  skStringReserveUTL(string, 5);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_true(skStringEmptyUTL(string));

  // Reserve size exactly the size of an allocation...
  skStringReserveUTL(string, SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_non_null(skStringDataUTL(string));
  assert_true(skStringEmptyUTL(string));

  // Reserve size that requires allocation...
  skStringReserveUTL(string, SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_non_null(skStringDataUTL(string));
  assert_true(skStringEmptyUTL(string));

  // Reserve size that requires more allocation...
  skStringReserveUTL(string, 10 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringCapacityUTL(string), 10 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_non_null(skStringDataUTL(string));
  assert_true(skStringEmptyUTL(string));

  // Attempt to reserve a smaller size (should not change capacity)...
  skStringReserveUTL(string, SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringCapacityUTL(string), 10 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_non_null(skStringDataUTL(string));
  assert_true(skStringEmptyUTL(string));

}

UNIT_TEST(skStringNCopyUTL) {
  char pBuffer[10*SKUTL_STRING_DEFAULT_CAPACITY];

  // Copy an empty string
  assert_int_equal(skStringNCopyUTL(string, "", 0), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_true(skStringEmptyUTL(string));

  // Copy a string with data
  assert_int_equal(skStringNCopyUTL(string, "Test", 4), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy a string with partial data
  assert_int_equal(skStringNCopyUTL(string, "Test", 2), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 2);
  assert_string_equal(skStringDataUTL(string), "Te");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy a string requiring an allocation
  generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_int_equal(skStringNCopyUTL(string, pBuffer, SKUTL_STRING_DEFAULT_CAPACITY + 1), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_string_equal(skStringDataUTL(string), pBuffer);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_false(skStringEmptyUTL(string));

  // Copy a string requiring a larger allocation
  generateStringOfLength(pBuffer, 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringNCopyUTL(string, pBuffer, 2 * SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_string_equal(skStringDataUTL(string), pBuffer);
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy a string to a smaller allocation
  generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_int_equal(skStringNCopyUTL(string, pBuffer, SKUTL_STRING_DEFAULT_CAPACITY + 1), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), SKUTL_STRING_DEFAULT_CAPACITY + 1);
  assert_string_equal(skStringDataUTL(string), pBuffer);
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy a string to a size which traditionally doesn't require allocation
  assert_int_equal(skStringNCopyUTL(string, "Test", 4), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy an empty string
  assert_int_equal(skStringNCopyUTL(string, "", 0), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_true(skStringEmptyUTL(string));

  // Copy a string with less data than stated
  assert_int_equal(skStringNCopyUTL(string, "Test", 10), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy a string to a smaller allocation
  generateStringOfLength(pBuffer, 4 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringNCopyUTL(string, pBuffer, 3 * SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 3 * SKUTL_STRING_DEFAULT_CAPACITY);
  generateStringOfLength(pBuffer, 3 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_string_equal(skStringDataUTL(string), pBuffer);
  assert_int_equal(skStringCapacityUTL(string), 3 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

}

// Most test cases are covered by skStringNCopyUTL()
UNIT_TEST(skStringCopyUTL) {

  // Copy a string to a size which traditionally doesn't require allocation
  assert_int_equal(skStringCopyUTL(string, "Test"), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy an empty string
  assert_int_equal(skStringCopyUTL(string, ""), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_true(skStringEmptyUTL(string));

}
#include <stdio.h>

UNIT_TEST(skStringNAppendUTL) {
  char pBuffer[10*SKUTL_STRING_DEFAULT_CAPACITY];
  char pBufferAppend[10*SKUTL_STRING_DEFAULT_CAPACITY];

  // Append nothing
  assert_int_equal(skStringNAppendUTL(string, "", 0), SK_SUCCESS);
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringLengthUTL(string), 0);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_true(skStringEmptyUTL(string));

  // Append a small test string
  generateStringOfLength(pBuffer, 1);
  generateStringOfLength(pBufferAppend, 1);
  assert_int_equal(skStringNAppendUTL(string, pBuffer, 1), SK_SUCCESS);
  assert_string_equal(skStringDataUTL(string), pBufferAppend);
  assert_int_equal(skStringLengthUTL(string), 1);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Append another small test string
  generateStringOfLength(pBuffer, 1);
  appendStringOfLength(pBufferAppend, 1);
  assert_int_equal(skStringNAppendUTL(string, pBuffer, 1), SK_SUCCESS);
  assert_string_equal(skStringDataUTL(string), pBufferAppend);
  assert_int_equal(skStringLengthUTL(string), 2);
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Append another string which will cause an allocation
  generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY);
  appendStringOfLength(pBufferAppend, SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringNAppendUTL(string, pBuffer, SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_string_equal(skStringDataUTL(string), pBufferAppend);
  assert_int_equal(skStringLengthUTL(string), 2 + SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringCapacityUTL(string), 2 + SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Append a partial string to the dynamic buffer
  generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY);
  appendStringOfLength(pBufferAppend, SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringNAppendUTL(string, pBuffer, SKUTL_STRING_DEFAULT_CAPACITY), SK_SUCCESS);
  assert_string_equal(skStringDataUTL(string), pBufferAppend);
  assert_int_equal(skStringLengthUTL(string), 2 + 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_int_equal(skStringCapacityUTL(string), 2 + 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

}

// Most test cases are covered by skStringNCopyUTL()
UNIT_TEST(skStringAppendUTL) {

  // Copy a string to a size which traditionally doesn't require allocation
  assert_int_equal(skStringAppendUTL(string, "Test"), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

  // Copy an empty string
  assert_int_equal(skStringAppendUTL(string, ""), SK_SUCCESS);
  assert_int_equal(skStringLengthUTL(string), 4);
  assert_string_equal(skStringDataUTL(string), "Test");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);
  assert_false(skStringEmptyUTL(string));

}

UNIT_TEST(skStringSwapUTL) {
  char pBuffer[10*SKUTL_STRING_DEFAULT_CAPACITY];

  // Simple swap
  {
    skStringCopyUTL(A, "A");
    skStringCopyUTL(B, "B");
    skStringSwapUTL(A, B);

    // Check string A
    assert_int_equal(skStringLengthUTL(A), 1);
    assert_string_equal(skStringDataUTL(A), "B");
    assert_int_equal(skStringCapacityUTL(A), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(A));

    // Check string B
    assert_int_equal(skStringLengthUTL(B), 1);
    assert_string_equal(skStringDataUTL(B), "A");
    assert_int_equal(skStringCapacityUTL(B), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(B));
  }

  // Differing size swap
  {
    skStringCopyUTL(A, "A");
    skStringCopyUTL(B, "BB");
    skStringSwapUTL(A, B);

    // Check string A
    assert_int_equal(skStringLengthUTL(A), 2);
    assert_string_equal(skStringDataUTL(A), "BB");
    assert_int_equal(skStringCapacityUTL(A), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(A));

    // Check string B
    assert_int_equal(skStringLengthUTL(B), 1);
    assert_string_equal(skStringDataUTL(B), "A");
    assert_int_equal(skStringCapacityUTL(B), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(B));
  }

  // Allocation-DefaultAllocation size swap
  {
    generateStringOfLength(pBuffer, 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    skStringCopyUTL(A, "A");
    skStringCopyUTL(B, pBuffer);
    skStringSwapUTL(A, B);

    // Check string A
    assert_int_equal(skStringLengthUTL(A), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    assert_string_equal(skStringDataUTL(A), pBuffer);
    assert_int_equal(skStringCapacityUTL(A), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(A));

    // Check string B
    assert_int_equal(skStringLengthUTL(B), 1);
    assert_string_equal(skStringDataUTL(B), "A");
    assert_int_equal(skStringCapacityUTL(B), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(B));
  }

  // Allocation-Allocation size swap
  {
    generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY);
    skStringCopyUTL(A, pBuffer);
    generateStringOfLength(pBuffer, 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    skStringCopyUTL(B, pBuffer);
    skStringSwapUTL(A, B);

    // Check string A
    assert_int_equal(skStringLengthUTL(A), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    assert_string_equal(skStringDataUTL(A), pBuffer);
    assert_int_equal(skStringCapacityUTL(A), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(A));

    // Check string B
    generateStringOfLength(pBuffer, SKUTL_STRING_DEFAULT_CAPACITY);
    assert_int_equal(skStringLengthUTL(B), SKUTL_STRING_DEFAULT_CAPACITY);
    assert_string_equal(skStringDataUTL(B), pBuffer);
    assert_int_equal(skStringCapacityUTL(B), 2 * SKUTL_STRING_DEFAULT_CAPACITY);
    assert_false(skStringEmptyUTL(B));
  }

}

UNIT_TEST(skStringDataUTL) {

  // Test default-allocated string
  assert_non_null(skStringDataUTL(string));

  // Test allocated string
  skStringReserveUTL(string, 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  assert_non_null(skStringDataUTL(string));

}

UNIT_TEST(skStringClearUTL) {
  char pBuffer[10*SKUTL_STRING_DEFAULT_CAPACITY];

  // Test on default-allocated string
  skStringCopyUTL(string, "A");
  skStringClearUTL(string);
  assert_true(skStringEmptyUTL(string));
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringCapacityUTL(string), SKUTL_STRING_DEFAULT_CAPACITY);

  // Test on allocated string
  generateStringOfLength(pBuffer, 2 * SKUTL_STRING_DEFAULT_CAPACITY);
  skStringCopyUTL(string, pBuffer);
  skStringClearUTL(string);
  assert_true(skStringEmptyUTL(string));
  assert_string_equal(skStringDataUTL(string), "");
  assert_int_equal(skStringCapacityUTL(string), 2 * SKUTL_STRING_DEFAULT_CAPACITY);

}

////////////////////////////////////////////////////////////////////////////////
// Suite Definition
////////////////////////////////////////////////////////////////////////////////

SUITE_INITIALIZE() {
  return 0;
}

SUITE_SETUP() {
  skCreateStringUTL(NULL, &string);
  skCreateStringUTL(NULL, &A);
  skCreateStringUTL(NULL, &B);
  return 0;
}

SUITE_TEARDOWN() {
  skDestroyStringUTL(string);
  skDestroyStringUTL(A);
  skDestroyStringUTL(B);
  return 0;
}

SUITE_DEINITIALIZE() {
  return 0;
}

BEGIN_REGISTER()
  ADD_TEST(skCheckParamUTL),
  ADD_TEST(skCheckParamBeginsUTL),
  ADD_TEST(skCStrCompareUTL),
  ADD_TEST(skCStrCompareCaseInsensitiveUTL),
  ADD_TEST(skCStrCompareBeginsUTL),
  ADD_TEST(skCreateStringUTL),
  ADD_TEST(skStringResizeUTL),
  ADD_TEST(skStringReserveUTL),
  ADD_TEST(skStringNCopyUTL),
  ADD_TEST(skStringCopyUTL),
  ADD_TEST(skStringNAppendUTL),
  ADD_TEST(skStringAppendUTL),
  ADD_TEST(skStringSwapUTL),
  ADD_TEST(skStringDataUTL),
  ADD_TEST(skStringClearUTL)
END_REGISTER()
