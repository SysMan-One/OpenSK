/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Unit tests for SkStringHeapEXT
 ******************************************************************************/

#include "cmocka.h"
#include <OpenSK/ext/string_heap.h>
#define SUITE_NAME SkStringHeapEXT

////////////////////////////////////////////////////////////////////////////////
// Global State
////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_STRING_HEAP_ELEMENT_SIZE (1 + SKEXT_STRING_HEAP_ELEMENT_OVERHEAD)
#define DEFAULT_STRING_HEAP_ELEMENT_COUNT 10
#define DEFAULT_STRING_HEAP_CAPACITY (DEFAULT_STRING_HEAP_ELEMENT_COUNT * DEFAULT_STRING_HEAP_ELEMENT_SIZE)
static SkStringHeapEXT stringHeap;

////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////

UNIT_TEST(skCreateStringHeapEXT) {
  SkStringHeapEXT localStringHeap;

  // Create a string heap with no hint (note: uses minimum capacity as defined by SKEXT_STRING_HEAP_MINIMUM_CAPACITY)
  assert_int_equal(skCreateStringHeapEXT(NULL, &localStringHeap, 0), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(localStringHeap), SKEXT_STRING_HEAP_MINIMUM_CAPACITY);
  skDestroyStringHeapEXT(localStringHeap);

  // Create a string heap with a hint of the minimum capacity
  assert_int_equal(skCreateStringHeapEXT(NULL, &localStringHeap, SKEXT_STRING_HEAP_MINIMUM_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(localStringHeap), SKEXT_STRING_HEAP_MINIMUM_CAPACITY);
  skDestroyStringHeapEXT(localStringHeap);

  // Create a string heap with a hint of a larger capacity
  assert_int_equal(skCreateStringHeapEXT(NULL, &localStringHeap, 10 * SKEXT_STRING_HEAP_MINIMUM_CAPACITY), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(localStringHeap), 10 * SKEXT_STRING_HEAP_MINIMUM_CAPACITY);
  skDestroyStringHeapEXT(localStringHeap);

}

UNIT_TEST(skStringHeapReserveEXT) {

  // Reserve the default string heap to be the same size (should be no change)
  assert_int_equal(skStringHeapReserveEXT(stringHeap, DEFAULT_STRING_HEAP_ELEMENT_SIZE * DEFAULT_STRING_HEAP_ELEMENT_COUNT), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY);

  // Reserve the string heap to be a little larger
  assert_int_equal(skStringHeapReserveEXT(stringHeap, 2 * DEFAULT_STRING_HEAP_ELEMENT_SIZE * DEFAULT_STRING_HEAP_ELEMENT_COUNT), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), 2 * DEFAULT_STRING_HEAP_CAPACITY);

  // Reserve the string heap to be a little smaller (Shouldn't change anything)
  assert_int_equal(skStringHeapReserveEXT(stringHeap, DEFAULT_STRING_HEAP_ELEMENT_SIZE * DEFAULT_STRING_HEAP_ELEMENT_COUNT), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), 2 * DEFAULT_STRING_HEAP_CAPACITY);

  // Add some items to the list for the next tests
  assert_int_equal(skStringHeapAddEXT(stringHeap, "A"), SK_SUCCESS);
  assert_int_equal(skStringHeapAddEXT(stringHeap, "B"), SK_SUCCESS);
  assert_int_equal(skStringHeapAddEXT(stringHeap, "C"), SK_SUCCESS);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 3);
  assert_true(skStringHeapContainsEXT(stringHeap, "A"));
  assert_true(skStringHeapContainsEXT(stringHeap, "B"));
  assert_true(skStringHeapContainsEXT(stringHeap, "C"));
  assert_false(skStringHeapContainsEXT(stringHeap, "D"));

  // Reserve the string heap to be a little bigger (Should still have elements)
  assert_int_equal(skStringHeapReserveEXT(stringHeap, 3 * DEFAULT_STRING_HEAP_ELEMENT_SIZE * DEFAULT_STRING_HEAP_ELEMENT_COUNT), SK_SUCCESS);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), 3 * DEFAULT_STRING_HEAP_CAPACITY);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 3);
  assert_true(skStringHeapContainsEXT(stringHeap, "A"));
  assert_true(skStringHeapContainsEXT(stringHeap, "B"));
  assert_true(skStringHeapContainsEXT(stringHeap, "C"));
  assert_false(skStringHeapContainsEXT(stringHeap, "D"));

}

UNIT_TEST(skStringHeapCapacityEXT) {

  // Check the defaults for our string heap
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 0);

  // Add an element, the capacity should reduce by OVERHEAD + strlen(element)
  assert_int_equal(skStringHeapAddEXT(stringHeap, "A"), SK_SUCCESS);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 1);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY - 1 * SKEXT_STRING_HEAP_ELEMENT_OVERHEAD);

  // Add another element, the capacity should reduce by OVERHEAD + strlen(element)
  assert_int_equal(skStringHeapAddEXT(stringHeap, "B"), SK_SUCCESS);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 2);
  assert_int_equal(skStringHeapCapacityEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY - 2 * SKEXT_STRING_HEAP_ELEMENT_OVERHEAD);

}

UNIT_TEST(skStringHeapCapacityRemainingEXT) {

  // Check the defaults for our string heap
  assert_int_equal(skStringHeapCapacityRemainingEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 0);

  // Add an element, the capacity should reduce by OVERHEAD + strlen(element)
  assert_int_equal(skStringHeapAddEXT(stringHeap, "A"), SK_SUCCESS);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 1);
  assert_int_equal(skStringHeapCapacityRemainingEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY - 1 * (SKEXT_STRING_HEAP_ELEMENT_OVERHEAD + 1));

  // Add another element, the capacity should reduce by OVERHEAD + strlen(element)
  assert_int_equal(skStringHeapAddEXT(stringHeap, "B"), SK_SUCCESS);
  assert_int_equal(skStringHeapCountEXT(stringHeap), 2);
  assert_int_equal(skStringHeapCapacityRemainingEXT(stringHeap), DEFAULT_STRING_HEAP_CAPACITY - 2 * (SKEXT_STRING_HEAP_ELEMENT_OVERHEAD + 1));

}

UNIT_TEST(skStringHeapAddEXT) {
  char const* pData;
  char pBuffer[2];
  size_t countAdded;
  size_t oldCapacity;

  pBuffer[0] = 'A';
  pBuffer[1] = 0;
  countAdded = 0;

  // Add elements until there is no more capacity
  while (skStringHeapCapacityRemainingEXT(stringHeap) > 0) {
    assert_int_equal(skStringHeapAddEXT(stringHeap, pBuffer), SK_SUCCESS);
    ++countAdded;
    ++pBuffer[0];
    assert_int_equal(skStringHeapCountEXT(stringHeap), countAdded);
  }

  // Add one more element that will put the heap over capacity
  // Note: In this case the string heap should resize automatically
  oldCapacity = skStringHeapCapacityEXT(stringHeap);
  assert_int_equal(skStringHeapAddEXT(stringHeap, pBuffer), SK_SUCCESS);
  ++countAdded;
  assert_true(skStringHeapCapacityEXT(stringHeap) > oldCapacity);
  assert_int_equal(skStringHeapCountEXT(stringHeap), countAdded);

  // Check all of the elements to ensure that data is preserved
  pData = NULL;
  pBuffer[0] = 'A';
  while ((pData = skStringHeapNextEXT(stringHeap, pData))) {
    assert_string_equal(pData, pBuffer);
    ++pBuffer[0];
  }
}

UNIT_TEST(skStringHeapAtEXT) {
  size_t idx;
  size_t countAdded;
  char const* pData;
  char pBuffer[2];

  pBuffer[0] = 'A';
  pBuffer[1] = 0;

  // Test getting data when there is nothing to get
  assert_null(skStringHeapAtEXT(stringHeap, 0));

  // Add elements until there is no more capacity
  countAdded = 0;
  while (skStringHeapCapacityRemainingEXT(stringHeap) > 0) {
    assert_int_equal(skStringHeapAddEXT(stringHeap, pBuffer), SK_SUCCESS);
    ++countAdded;
    ++pBuffer[0];
    assert_int_equal(skStringHeapCountEXT(stringHeap), countAdded);
  }

  // Test getting all of the elements
  pData = NULL;
  pBuffer[0] = 'A';
  for (idx = 0; idx < countAdded; ++idx) {
    pData = skStringHeapAtEXT(stringHeap, idx);
    assert_string_equal(pData, pBuffer);
    ++pBuffer[0];
  }

  // Test getting an element outside of the valid range
  assert_null(skStringHeapAtEXT(stringHeap, countAdded));
}

////////////////////////////////////////////////////////////////////////////////
// Suite Definition
////////////////////////////////////////////////////////////////////////////////

SUITE_INITIALIZE() {
  return 0;
}

SUITE_SETUP() {
  skCreateStringHeapEXT(NULL, &stringHeap, DEFAULT_STRING_HEAP_CAPACITY);
  return 0;
}

SUITE_TEARDOWN() {
  skDestroyStringHeapEXT(stringHeap);
  return 0;
}

SUITE_DEINITIALIZE() {
  return 0;
}

BEGIN_REGISTER()
  ADD_TEST(skCreateStringHeapEXT),
  ADD_TEST(skStringHeapReserveEXT),
  ADD_TEST(skStringHeapCapacityEXT),
  ADD_TEST(skStringHeapCapacityRemainingEXT),
  ADD_TEST(skStringHeapAddEXT),
  ADD_TEST(skStringHeapAtEXT)
END_REGISTER()
