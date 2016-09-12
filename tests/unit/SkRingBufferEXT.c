/*******************************************************************************
 * OpenSK (tests) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * Unit tests for SkRingBufferEXT
 ******************************************************************************/

#include "cmocka.h"
#include <OpenSK/ext/ring_buffer.h>
#include <string.h>

#define SUITE_NAME SkRingBufferEXT

////////////////////////////////////////////////////////////////////////////////
// Global State
////////////////////////////////////////////////////////////////////////////////

static size_t ringBufferSizes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 48, 96, 1024, 2048, 4096 };
#define MAX_RING_BUFFER_SIZE 4096
#define DEFAULT_RING_BUFFER_COUNT (sizeof(ringBufferSizes) / sizeof(ringBufferSizes[0]))
static SkRingBufferEXT ringBuffer[DEFAULT_RING_BUFFER_COUNT];

static void
fillBufferAlphaAscending(char* pBuffer, size_t size) {
  char currChar = 'A';
  while (size) {
    *pBuffer = currChar;
    if (currChar == 'Z') {
      currChar = 'A';
    }
    else {
      ++currChar;
    }
    ++pBuffer;
    --size;
  }
  *pBuffer = 0;
}

static SkBool32
checkBufferAlphaAscending(char const* pBuffer, size_t size) {
  char currChar = 'A';
  while (size) {
    if (*pBuffer != currChar) {
      return SK_FALSE;
    }
    if (currChar == 'Z') {
      currChar = 'A';
    }
    else {
      ++currChar;
    }
    ++pBuffer;
    --size;
  }
  return SK_TRUE;
}

////////////////////////////////////////////////////////////////////////////////
// Unit Tests
////////////////////////////////////////////////////////////////////////////////

UNIT_TEST(skRingBufferCapacityEXT) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    assert_int_equal(skRingBufferCapacityEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferAdvanceWriteLocationEXT_writeOneElement) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 0) {
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_false(debugInfoBefore.dataIsWrapping);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      if (skRingBufferCapacityEXT(ringBuffer[idx]) == 1) {
        assert_true(debugInfoAfter.dataIsWrapping);
        assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      }
      else {
        assert_false(debugInfoAfter.dataIsWrapping);
        assert_ptr_not_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      }
    }
  }
}

UNIT_TEST(skRingBufferAdvanceWriteLocationEXT_writeNElements) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 0) {
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_false(debugInfoBefore.dataIsWrapping);
      assert_true(debugInfoAfter.dataIsWrapping);
    }
  }
}

UNIT_TEST(skRingBufferAdvanceWriteLocationEXT_splitData) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 2) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_not_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_false(debugInfoBefore.dataIsWrapping);
      assert_true(debugInfoAfter.dataIsWrapping);
    }
  }
}

UNIT_TEST(skRingBufferAdvanceReadLocationEXT_readOneElement) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      if (skRingBufferCapacityEXT(ringBuffer[idx]) == 1) {
        assert_true(debugInfoBefore.dataIsWrapping);
        assert_false(debugInfoAfter.dataIsWrapping);
        assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      }
      else {
        assert_false(debugInfoBefore.dataIsWrapping);
        assert_false(debugInfoAfter.dataIsWrapping);
        assert_ptr_not_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      }
    }
  }
}

UNIT_TEST(skRingBufferAdvanceReadLocationEXT_readNElements) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_true(debugInfoBefore.dataIsWrapping);
      assert_false(debugInfoAfter.dataIsWrapping);
    }
  }
}

UNIT_TEST(skRingBufferAdvanceReadLocationEXT_splitData) {
  size_t idx;
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 2) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_ptr_not_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_true(debugInfoBefore.dataIsWrapping);
      assert_false(debugInfoAfter.dataIsWrapping);
    }
  }
}

UNIT_TEST(skRingBufferWriteEXT_oneElement) {
  size_t idx;
  size_t written;
  char buffer[1] = { 'X' };
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    written = skRingBufferWriteEXT(ringBuffer[idx], buffer, 1);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      assert_int_equal(written, 1);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_not_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_int_equal(*(char*)debugInfoAfter.pDataBegin, *buffer);
    }
    else if (skRingBufferCapacityEXT(ringBuffer[idx]) == 1) {
      assert_int_equal(written, 1);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
      assert_int_equal(*(char*)debugInfoAfter.pDataBegin, *buffer);
    }
    else {
      assert_int_equal(written, 0);
      assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferWriteEXT_NElements) {
  size_t idx;
  size_t written;
  char buffer[MAX_RING_BUFFER_SIZE + 1];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, MAX_RING_BUFFER_SIZE);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    written = skRingBufferWriteEXT(ringBuffer[idx], buffer, ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    assert_int_equal(written, ringBufferSizes[idx]);
    assert_false(debugInfoBefore.dataIsWrapping);
    assert_true(debugInfoAfter.dataIsWrapping);
    assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
    assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    assert_true(checkBufferAlphaAscending(debugInfoBefore.pDataBegin, ringBufferSizes[idx]));
  }
}

UNIT_TEST(skRingBufferWriteEXT_MElements) {
  size_t idx;
  size_t written;
  char buffer[2 * MAX_RING_BUFFER_SIZE + 1];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, 2 * MAX_RING_BUFFER_SIZE);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    written = skRingBufferWriteEXT(ringBuffer[idx], buffer, 2 * MAX_RING_BUFFER_SIZE);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    assert_int_equal(written, ringBufferSizes[idx]);
    assert_false(debugInfoBefore.dataIsWrapping);
    assert_true(debugInfoAfter.dataIsWrapping);
    assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
    assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    assert_true(checkBufferAlphaAscending(debugInfoBefore.pDataBegin, ringBufferSizes[idx]));
  }
}

UNIT_TEST(skRingBufferWriteEXT_splitData) {
  size_t idx;
  size_t written;
  size_t expected;
  char buffer[3];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, 2);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (ringBufferSizes[idx] > 0) {
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
    }
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    written = skRingBufferWriteEXT(ringBuffer[idx], buffer, 2);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    expected = (1 < ringBufferSizes[idx]) ? 1 : ringBufferSizes[idx];
    assert_int_equal(written, expected);
    assert_false(debugInfoBefore.dataIsWrapping);
    assert_true(debugInfoAfter.dataIsWrapping);
  }
}

UNIT_TEST(skRingBufferReadEXT_oneElement) {
  size_t idx;
  size_t read;
  char buffer[1] = { 'X' };
  char inBuffer[2] = { 0 };
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 0) {
      skRingBufferWriteEXT(ringBuffer[idx], buffer, 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      read = skRingBufferReadEXT(ringBuffer[idx], inBuffer, 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      assert_int_equal(read, 1);
      if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
        assert_ptr_not_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      }
      else {
        assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
      }
      assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferReadEXT_NElements) {
  size_t idx;
  size_t read;
  char buffer[MAX_RING_BUFFER_SIZE + 1];
  char inBuffer[MAX_RING_BUFFER_SIZE + 1];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, MAX_RING_BUFFER_SIZE);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferWriteEXT(ringBuffer[idx], buffer, ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    read = skRingBufferReadEXT(ringBuffer[idx], inBuffer, ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    assert_int_equal(read, ringBufferSizes[idx]);
    assert_true(debugInfoBefore.dataIsWrapping);
    assert_false(debugInfoAfter.dataIsWrapping);
    assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
    assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    assert_true(checkBufferAlphaAscending(inBuffer, ringBufferSizes[idx]));
  }
}

UNIT_TEST(skRingBufferReadEXT_MElements) {
  size_t idx;
  size_t read;
  char buffer[2 * MAX_RING_BUFFER_SIZE + 1];
  char inBuffer[2 * MAX_RING_BUFFER_SIZE + 1];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, 2 * MAX_RING_BUFFER_SIZE);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferWriteEXT(ringBuffer[idx], buffer, 2 * MAX_RING_BUFFER_SIZE);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
    read = skRingBufferReadEXT(ringBuffer[idx], inBuffer, 2 * MAX_RING_BUFFER_SIZE);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
    assert_int_equal(read, ringBufferSizes[idx]);
    assert_true(debugInfoBefore.dataIsWrapping);
    assert_false(debugInfoAfter.dataIsWrapping);
    assert_ptr_equal(debugInfoBefore.pDataBegin, debugInfoAfter.pDataBegin);
    assert_ptr_equal(debugInfoBefore.pDataEnd, debugInfoAfter.pDataEnd);
    assert_true(checkBufferAlphaAscending(inBuffer, ringBufferSizes[idx]));
  }
}

UNIT_TEST(skRingBufferReadEXT_splitData) {
  size_t idx;
  size_t written;
  size_t read;
  size_t expected;
  char buffer[3];
  char inBuffer[3];
  SkRingBufferDebugInfoEXT debugInfoBefore;
  SkRingBufferDebugInfoEXT debugInfoAfter;

  fillBufferAlphaAscending(buffer, 2);
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (ringBufferSizes[idx] > 0) {
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      written = skRingBufferWriteEXT(ringBuffer[idx], buffer, 2);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoBefore);
      read = skRingBufferReadEXT(ringBuffer[idx], inBuffer, 2);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfoAfter);
      expected = (1 < ringBufferSizes[idx]) ? 1 : ringBufferSizes[idx];
      assert_int_equal(written, read);
      assert_int_equal(read, expected);
      assert_true(debugInfoBefore.dataIsWrapping);
      assert_false(debugInfoAfter.dataIsWrapping);
    }
  }
}

UNIT_TEST(skRingBufferClearEXT) {
  size_t idx;

  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (ringBufferSizes[idx] > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferClearEXT(ringBuffer[idx]);
      assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx]);
      assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx]);
    }
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: Empty
// |--------------------------------------------------------------------------|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_noData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_noData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_noData) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx]);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_noData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_noData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_noData) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), 0);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: Empty (After Write-Read)
// |--------------------------------------------------------------------------|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_noDataAfterWriteRead) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_noDataAfterWriteRead) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_noDataAfterWriteRead) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx]);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_noDataAfterWriteRead) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_noDataAfterWriteRead) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_noDataAfterWriteRead) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), 0);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: One Element at the Beginning
// |X-------------------------------------------------------------------------|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_oneElementBegin) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx] - 1);
    }
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_oneElementBegin) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx] - 1);
    }
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_oneElementBegin) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx] - 1);
      assert_ptr_equal(pLocation, debugInfo.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_oneElementBegin) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), 1);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_oneElementBegin) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), 1);
    }
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_oneElementBegin) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), 1);
      assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
      assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    }
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: All N Elements Filled
// |XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_NElements) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_NElements) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), 0);
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_NElements) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), 0);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_NElements) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_NElements) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx]);
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_NElements) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
    assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx]);
    assert_ptr_equal(pLocation, debugInfo.pCapacityBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    assert_ptr_equal(pLocation, debugInfo.pDataEnd);
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: One Element at the End
// |-------------------------------------------------------------------------X|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_oneElementEnd) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx] - 1);
    }
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_oneElementEnd) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx] - 1);
    }
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_oneElementEnd) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx] - 1);
      assert_ptr_equal(pLocation, debugInfo.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_oneElementEnd) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
    assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), 1);
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_oneElementEnd) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
    skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
    assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), 1);
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_oneElementEnd) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferWriteRemainingEXT(ringBuffer[idx]) > 0) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), 1);
      assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    }
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: Data at the Beginning and End
// |X------------------------------------------------------------------------X|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_splitData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx] - 2);
    }
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_splitData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx] - 2);
    }
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_splitData) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx] - 2);
      assert_ptr_equal(pLocation, debugInfo.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_splitData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), 2);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_splitData) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), 1);
    }
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_splitData) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx]);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), 1);
      assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    }
  }
}

//------------------------------------------------------------------------------
// Ring Buffer Configuration: Data in the Center
// |-XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX-|
//------------------------------------------------------------------------------
UNIT_TEST(skRingBufferWriteRemainingEXT_splitFree) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingEXT(ringBuffer[idx]), 2);
    }
  }
}

UNIT_TEST(skRingBufferWriteRemainingContiguousEXT_splitFree) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferWriteRemainingContiguousEXT(ringBuffer[idx]), 1);
    }
  }
}

UNIT_TEST(skRingBufferNextWriteLocationEXT_splitFree) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextWriteLocationEXT(ringBuffer[idx], &pLocation), 1);
      assert_ptr_equal(pLocation, debugInfo.pDataEnd);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingEXT_splitFree) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingEXT(ringBuffer[idx]), ringBufferSizes[idx] - 2);
    }
  }
}

UNIT_TEST(skRingBufferReadRemainingContiguousEXT_splitFree) {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      assert_int_equal(skRingBufferReadRemainingContiguousEXT(ringBuffer[idx]), ringBufferSizes[idx] - 2);
    }
  }
}

UNIT_TEST(skRingBufferNextReadLocationEXT_splitFree) {
  size_t idx;
  void *pLocation;
  SkRingBufferDebugInfoEXT debugInfo;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    if (skRingBufferCapacityEXT(ringBuffer[idx]) > 1) {
      skRingBufferAdvanceWriteLocationEXT(ringBuffer[idx], ringBufferSizes[idx] - 1);
      skRingBufferAdvanceReadLocationEXT(ringBuffer[idx], 1);
      skRingBufferDebugInfoEXT(ringBuffer[idx], &debugInfo);
      assert_int_equal(skRingBufferNextReadLocationEXT(ringBuffer[idx], &pLocation), ringBufferSizes[idx] - 2);
      assert_ptr_equal(pLocation, debugInfo.pDataBegin);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Suite Definition
////////////////////////////////////////////////////////////////////////////////

SUITE_INITIALIZE() {
  return 0;
}

SUITE_SETUP() {
  size_t idx;
  SkResult result;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    result = skCreateRingBufferEXT(NULL, &ringBuffer[idx], ringBufferSizes[idx]);
    if (result != SK_SUCCESS) {
      return -1;
    }
  }
  return 0;
}

SUITE_TEARDOWN() {
  size_t idx;
  for (idx = 0; idx < DEFAULT_RING_BUFFER_COUNT; ++idx) {
    skDestroyRingBufferEXT(ringBuffer[idx]);
  }
  return 0;
}

SUITE_DEINITIALIZE() {
  return 0;
}

BEGIN_REGISTER()
  ADD_TEST(skRingBufferCapacityEXT),
  ADD_TEST(skRingBufferAdvanceWriteLocationEXT_writeOneElement),
  ADD_TEST(skRingBufferAdvanceWriteLocationEXT_writeNElements),
  ADD_TEST(skRingBufferAdvanceWriteLocationEXT_splitData),
  ADD_TEST(skRingBufferAdvanceReadLocationEXT_readOneElement),
  ADD_TEST(skRingBufferAdvanceReadLocationEXT_readNElements),
  ADD_TEST(skRingBufferAdvanceReadLocationEXT_splitData),
  ADD_TEST(skRingBufferWriteEXT_oneElement),
  ADD_TEST(skRingBufferWriteEXT_NElements),
  ADD_TEST(skRingBufferWriteEXT_MElements),
  ADD_TEST(skRingBufferWriteEXT_splitData),
  ADD_TEST(skRingBufferReadEXT_oneElement),
  ADD_TEST(skRingBufferReadEXT_NElements),
  ADD_TEST(skRingBufferReadEXT_MElements),
  ADD_TEST(skRingBufferReadEXT_splitData),
  ADD_TEST(skRingBufferClearEXT),
  // No Data
  ADD_TEST(skRingBufferWriteRemainingEXT_noData),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_noData),
  ADD_TEST(skRingBufferNextWriteLocationEXT_noData),
  ADD_TEST(skRingBufferReadRemainingEXT_noData),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_noData),
  ADD_TEST(skRingBufferNextReadLocationEXT_noData),
  // No Data (After Write-Read)
  ADD_TEST(skRingBufferWriteRemainingEXT_noDataAfterWriteRead),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_noDataAfterWriteRead),
  ADD_TEST(skRingBufferNextWriteLocationEXT_noDataAfterWriteRead),
  ADD_TEST(skRingBufferReadRemainingEXT_noDataAfterWriteRead),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_noDataAfterWriteRead),
  ADD_TEST(skRingBufferNextReadLocationEXT_noDataAfterWriteRead),
  // One Element (Begin)
  ADD_TEST(skRingBufferWriteRemainingEXT_oneElementBegin),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_oneElementBegin),
  ADD_TEST(skRingBufferNextWriteLocationEXT_oneElementBegin),
  ADD_TEST(skRingBufferReadRemainingEXT_oneElementBegin),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_oneElementBegin),
  ADD_TEST(skRingBufferNextReadLocationEXT_oneElementBegin),
  // N-Elements
  ADD_TEST(skRingBufferWriteRemainingEXT_NElements),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_NElements),
  ADD_TEST(skRingBufferNextWriteLocationEXT_NElements),
  ADD_TEST(skRingBufferReadRemainingEXT_NElements),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_NElements),
  ADD_TEST(skRingBufferNextReadLocationEXT_NElements),
  // One Element (End)
  ADD_TEST(skRingBufferWriteRemainingEXT_oneElementEnd),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_oneElementEnd),
  ADD_TEST(skRingBufferNextWriteLocationEXT_oneElementEnd),
  ADD_TEST(skRingBufferReadRemainingEXT_oneElementEnd),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_oneElementEnd),
  ADD_TEST(skRingBufferNextReadLocationEXT_oneElementEnd),
  // Split Data
  ADD_TEST(skRingBufferWriteRemainingEXT_splitData),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_splitData),
  ADD_TEST(skRingBufferNextWriteLocationEXT_splitData),
  ADD_TEST(skRingBufferReadRemainingEXT_splitData),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_splitData),
  ADD_TEST(skRingBufferNextReadLocationEXT_splitData),
  // Split Free
  ADD_TEST(skRingBufferWriteRemainingEXT_splitFree),
  ADD_TEST(skRingBufferWriteRemainingContiguousEXT_splitFree),
  ADD_TEST(skRingBufferNextWriteLocationEXT_splitFree),
  ADD_TEST(skRingBufferReadRemainingEXT_splitFree),
  ADD_TEST(skRingBufferReadRemainingContiguousEXT_splitFree),
  ADD_TEST(skRingBufferNextReadLocationEXT_splitFree)
END_REGISTER()
