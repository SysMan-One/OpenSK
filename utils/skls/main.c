/*******************************************************************************
 * Copyright 2016 Trent Reed
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------------------
 * OpenSK sample application (iterates audio devices).
 *------------------------------------------------------------------------------
 * A high-level overview of how this application works is by first resolving
 * all SkObjects passed into the skls utility. Then, for each SkObject we follow
 * the same pattern (display* -> constructLayout -> print*).
 ******************************************************************************/

// OpenSK Headers
#include <OpenSK/opensk.h>
#include <OpenSK/utl/macros.h>
#include <OpenSK/utl/color_config.h>
#include <OpenSK/utl/string.h>

// C99 Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// Non-Standard Headers
#ifdef    OPENSK_UNIX
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#endif // OPENSK_UNIX
#ifdef    OPENSK_WINDOWS
#include <Windows.h>
#endif // OPENSK_WINDOWS

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define INFINITE_WIDTH 0

/*******************************************************************************
 * Type Declarations / Definitions
 ******************************************************************************/
struct Display;
struct DisplayItem;

typedef void (*pfn_print)(struct Display *display, struct DisplayItem *entries, size_t count);
typedef void (*pfn_sort)(struct DisplayItem *entries, size_t count);

typedef struct DisplayItem {
  void*                                 pUserData;
  char*                                 pString;
  size_t                                stringLength;
} DisplayItem;

typedef struct Display {
  char const*                           path;
  uint32_t                              f_longListing:1;
  uint32_t                              f_rowOrder:1;
  uint32_t                              f_color:1;
  uint32_t                              spacing;
  uint32_t                              displayWidth;
  size_t                                maxLength;
  char                                  separator;
  pfn_sort                              sortfnc;
  pfn_print                             printfnc;
  SkColorConfigUTL                      colorConfig;
} Display;

typedef struct Layout {
  size_t                                rowCount;
  size_t                                columnCount;
  size_t*                               columnWidth;
} Layout;

typedef struct SklsExecution {
  char const**                          skPaths;
  uint32_t                              skPathCount;
  uint32_t                              f_devices:1;
  uint32_t                              f_endpoints:1;
  Display                               display;
  SkPlatformPLT                         platform;
  SkColorConfigUTL                      colorConfig;
} SklsExecution;

/*******************************************************************************
 * Static Helper Functions
 ******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// UNIX Colors
////////////////////////////////////////////////////////////////////////////////
#ifdef    OPENSK_UNIX
static void
endcolor() {
  fputs("\x1B[0m", stdout);
}

static void
colorquit(int sig) {
  endcolor();
  (void)signal(sig, SIG_DFL);
  (void)kill(getpid(), sig);
}

static void
printColorStart(Display *display, DisplayItem *item) {
  int colorSetFlag;
  SkColorFormatUTL format;

  // Initialize default values.
  colorSetFlag = 0;
  format = skGetColorConfigFormatUTL(display->colorConfig, item->pUserData);
  fputs("\x1B[", stdout);

#define fputs(v, f) if (colorSetFlag) fputc(';', f); colorSetFlag = 1; fputs(v, f)

  // Set the supported foreground color
  switch (format & SK_COLOR_COLOR_MASK_FG) {
    case SK_COLOR_BLACK_FG:
      fputs("30", stdout);
      break;
    case SK_COLOR_RED_FG:
      fputs("31", stdout);
      break;
    case SK_COLOR_GREEN_FG:
      fputs("32", stdout);
      break;
    case SK_COLOR_YELLOW_FG:
      fputs("33", stdout);
      break;
    case SK_COLOR_BLUE_FG:
      fputs("34", stdout);
      break;
    case SK_COLOR_MAGENTA_FG:
      fputs("35", stdout);
      break;
    case SK_COLOR_CYAN_FG:
      fputs("36", stdout);
      break;
    case SK_COLOR_GRAY_FG:
      fputs("37", stdout);
      break;
    default:
      // Unsupported - ignore!
      break;
  }

  // Set the supported background color
  switch (format & SK_COLOR_COLOR_MASK_BG) {
    case SK_COLOR_BLACK_BG:
      fputs("40", stdout);
      break;
    case SK_COLOR_RED_BG:
      fputs("41", stdout);
      break;
    case SK_COLOR_GREEN_BG:
      fputs("42", stdout);
      break;
    case SK_COLOR_YELLOW_BG:
      fputs("43", stdout);
      break;
    case SK_COLOR_BLUE_BG:
      fputs("44", stdout);
      break;
    case SK_COLOR_MAGENTA_BG:
      fputs("45", stdout);
      break;
    case SK_COLOR_CYAN_BG:
      fputs("46", stdout);
      break;
    case SK_COLOR_GRAY_BG:
      fputs("47", stdout);
      break;
    default:
      // Unsupported - ignore!
      break;
  }

  // Set the supported effects parameters
  if (format & SK_COLOR_BOLD_BIT) {
    fputs("1", stdout);
  }
  if (format & SK_COLOR_FAINT_BIT) {
    fputs("2", stdout);
  }
  if (format & SK_COLOR_ITALIC_BIT) {
    fputs("3", stdout);
  }
  if (format & SK_COLOR_UNDERLINE_BIT) {
    fputs("4", stdout);
  }
  if (format & SK_COLOR_BLINK_BIT) {
    fputs("5", stdout);
  }
  if (format & SK_COLOR_NEGATIVE_BIT) {
    fputs("7", stdout);
  }

#undef fputs

  fputc('m', stdout);
}

static void
printColorEnd(Display *display, DisplayItem *item) {
  (void)item;
  (void)display;
  fputs("\x1B[0m", stdout);
}

static int
isConsole() {
  return isatty(STDOUT_FILENO);
}

static int
getConsoleWidth() {
  char const* pCharConst;
  struct winsize winSize;

  pCharConst = getenv("COLUMNS");
  if (pCharConst != NULL && *pCharConst != '\0') {
    return (uint32_t)atoi(pCharConst);
  }
  else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) != -1) {
    if (winSize.ws_col > 0) {
      return winSize.ws_col;
    }
  }
  return 1;
}

#endif // OPENSK_UNIX

////////////////////////////////////////////////////////////////////////////////
// Windows Colors
////////////////////////////////////////////////////////////////////////////////
#ifdef    OPENSK_WINDOWS
#define strdup _strdup

static void
endcolor() {
  // Unsupported
}

static void
colorquit(int sig) {
  (void)sig;
  // Unsupported
}

static void
printColorStart(Display *display, DisplayItem *item) {
  (void)display;
  (void)item;
  // Unsupported
}

static void
printColorEnd(Display *display, DisplayItem *item) {
  (void)item;
  (void)display;
  // Unsupported
}

static int
isConsole() {
  return (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE);
}

static int
getConsoleWidth() {
  CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bufferInfo)) {
    return bufferInfo.srWindow.Right - bufferInfo.srWindow.Left + 1;
  }
  return 1;
}

#endif // OPENSK_WINDOWS

static void
skInitializeDisplayItem(DisplayItem *pDisplayItem, char const* pString, void* pUserData) {
  pDisplayItem->pString = strdup(pString);
  pDisplayItem->pUserData = pUserData;
  pDisplayItem->stringLength = strlen(pString);
}

static void
skDeinitializeDisplayItem(DisplayItem *pDisplayItem) {
  free(pDisplayItem->pString);
}

static void
skDeinitializeDisplayItems(DisplayItem *pDisplayItems, uint32_t count) {
  uint32_t idx;
  for (idx = 0; idx < count; ++idx) {
    skDeinitializeDisplayItem(&pDisplayItems[idx]);
  }
}

static void
printItem(Display *display, DisplayItem *item, size_t width) {

#ifdef   OPENSK_UTILS_COLORS
  // Print the color prefix
  if (display->f_color) {
    printColorStart(display, item);
  }
#endif // OPENSK_UTILS_COLORS

  // Print the string itself
  fputs(item->pString, stdout);

#ifdef   OPENSK_UTILS_COLORS
  // Print the color postfix
  if (display->f_color) {
    printColorEnd(display, item);
  }
#endif // OPENSK_UTILS_COLORS

  // Add the appropriate spacing
  if (width > item->stringLength) {
    width -= item->stringLength;
    while (width--) {
      fputc(' ', stdout);
    }
  }
}

/*******************************************************************************
 * Layout Functions
 ******************************************************************************/

static Layout*
constructLayout(Display *display, DisplayItem *items, size_t itemCount) {
  size_t idx;
  size_t columnIdx;
  size_t maxWidth;
  size_t maxColumns;
  Layout *layout;

  // Allocate required data
  layout = calloc(1, sizeof(Layout));

  // Calculate the maximum number of columns possible for the display
  maxWidth = 0;
  for (maxColumns = 0; maxColumns < itemCount; ++maxColumns) {
    maxWidth += items[maxColumns].stringLength + display->spacing;
    if (maxWidth >= display->displayWidth) {
      break;
    }
  }

  // Find the layout configuration that best fits the display
  // Case: We need to print a single column with all of the content
  if (maxColumns == 0) {
    layout->rowCount = itemCount;
    layout->columnCount = 1;
    layout->columnWidth = malloc(sizeof(size_t) * 1);
    for (idx = 0; idx < itemCount; ++idx) {
      if (layout->columnWidth[0] < items[idx].stringLength) {
        layout->columnWidth[0] = items[idx].stringLength;
      }
    }
  }
  // Case: We need to print a single row with all of the content
  else if (maxColumns == itemCount) {
    layout->rowCount = 1;
    layout->columnCount = itemCount;
    layout->columnWidth = malloc(sizeof(size_t) * itemCount);
    for (idx = 0; idx < itemCount; ++idx) {
      layout->columnWidth[idx] = items[idx].stringLength;
    }
  }
  // Case: We need to print N columns, where we don't know what N is
  else {
    ++maxColumns; // Note: We must make maxColumns start from 1 for this algorithm.
    layout->columnWidth = malloc(sizeof(size_t) * maxColumns);
    for (uint32_t layoutIdx = 0; layoutIdx < maxColumns; ++layoutIdx) {

      // Configure the layout that we're testing
      layout->columnCount = maxColumns - layoutIdx;
      layout->rowCount = itemCount / layout->columnCount;
      if (itemCount % layout->columnCount) ++layout->rowCount;
      memset(layout->columnWidth, 0, sizeof(size_t) * layout->columnCount);

      // Calculate the maximum layout column sizes
      for (idx = 0; idx < itemCount; ++idx) {
        if (display->f_rowOrder) {
          columnIdx = idx / layout->rowCount;
        } else {
          columnIdx = idx % layout->columnCount;
        }
        layout->columnWidth[columnIdx] = SKMAX(
          layout->columnWidth[columnIdx],
          items[idx].stringLength
        );
      }

      // Test if this layout configuration fits within the display
      maxWidth = 0;
      for (idx = 0; idx < layout->columnCount; ++idx) {
        maxWidth += layout->columnWidth[idx];
      }
      maxWidth += display->spacing * (layout->columnCount - 1);
      if (maxWidth < display->displayWidth) {
        break;
      }
    }
  }

  return layout;
}

static void
destructLayout(Layout *layout) {
  free(layout->columnWidth);
  free(layout);
}

/*******************************************************************************
 * Sort Functions
 ******************************************************************************/

#define less(a, b) (strcmp(a.pString, b.pString) < 0)
#define swap(a, b) tmp = a; a = b; b = tmp
static size_t
partitionStrcmp(DisplayItem *items, size_t lowIdx, size_t highIdx) {
  DisplayItem* pivot;
  DisplayItem tmp;

  // Note: Even though lowIdx might underflow, the next operation corrects it.
  //       This makes the unchecked --lowIdx acceptable (likewise with highIdx overflow).
  //       Another point is that the pivot will always change after the first swap.
  //       This isn't terrible because the sorting will still happen - just not "quicksort".
  pivot = &items[lowIdx];
  --lowIdx;
  ++highIdx;
  for (;;) {
    do {
      ++lowIdx;
    } while (less(items[lowIdx], (*pivot)));
    do {
      --highIdx;
    } while (less((*pivot), items[highIdx]));
    if (lowIdx >= highIdx) return highIdx;
    swap(items[lowIdx], items[highIdx]);
  }
}
#undef less
#undef swap

static void
quicksortStrcmp(DisplayItem *items, size_t lowIdx, size_t highIdx) {
  size_t pivotIdx;
  if (lowIdx < highIdx) {
    pivotIdx = partitionStrcmp(items, lowIdx, highIdx);
    quicksortStrcmp(items, lowIdx, pivotIdx);
    quicksortStrcmp(items, pivotIdx + 1, highIdx);
  }
}

static void
sortStrcmp(DisplayItem *items, size_t itemCount) {
  if (itemCount > 1) {
    quicksortStrcmp(items, 0, itemCount - 1);
  }
}

static void
sortNone(DisplayItem *items, size_t itemCount) {
  (void)items;
  (void)itemCount;
}

/*******************************************************************************
 * Print Functions
 ******************************************************************************/

static void
printscol(Display *display, DisplayItem *items, size_t itemCount) {
  (void)display;
  for (uint32_t idx = 0; idx < itemCount; ++idx) {
    printItem(display, &items[idx], 0);
    fputc('\n', stdout);
  }
}

static void
printsrow(Display *display, DisplayItem *items, size_t itemCount) {
  for (uint32_t idx = 0; idx < itemCount; ++idx) {
    if (idx != 0) printf("%c ", display->separator);
    printItem(display, &items[idx], 0);
  }
  fputs("\n", stdout);
}

static void
printrow(Display *display, DisplayItem *items, size_t itemCount) {
  for (uint32_t idx = 0; idx < itemCount; ++idx) {
    printItem(display, &items[idx], 0);
    printf("%*s ", display->spacing, "");
  }
  fputs("\n", stdout);
}

static void
printcol(Display *display, DisplayItem *items, size_t itemCount) {
  size_t columnIdx, rowIdx, charIdx, itemIdx;
  Layout *layout = constructLayout(display, items, itemCount);

  if (layout->columnCount == 1) {
    printscol(display, items, itemCount);
    return;
  }

  for (rowIdx = 0; rowIdx < layout->rowCount; ++rowIdx) {
    for (columnIdx = 0; columnIdx < layout->columnCount; ++columnIdx) {
      for (charIdx = (columnIdx) ? 0 : display->spacing; charIdx < display->spacing; ++charIdx) {
        fputc(display->separator, stdout);
      }
      if (display->f_rowOrder) {
        itemIdx = columnIdx * layout->rowCount + rowIdx;
      }
      else {
        itemIdx = rowIdx * layout->columnCount + columnIdx;
      }
      if (itemIdx >= itemCount) break;
      printItem(display, &items[itemIdx], layout->columnWidth[columnIdx]);
    }
    fputs("\n", stdout);
  }
  destructLayout(layout);
}

/*******************************************************************************
 * Print Functions (long-form)
 ******************************************************************************/

#define CHECK_STREAM_SUPPORT(id, idx, s) streamSupport[idx] = (features.supportedStreams & s) ? #id[0] : '-'
static void
printlong_driver(Display *display, DisplayItem *item) {
  SkDriverFeatures features;
  SkDriverProperties properties;
  char streamSupport[] = "---\0";

  // Format: {Support} {API-Support} {identifier} {Description}
  skGetDriverFeatures((SkDriver)item->pUserData, &features);
  skGetDriverProperties((SkDriver)item->pUserData, &properties);
  CHECK_STREAM_SUPPORT(p, 0, SK_STREAM_PCM_MASK);
  CHECK_STREAM_SUPPORT(m, 1, SK_STREAM_MIDI_MASK);
  CHECK_STREAM_SUPPORT(v, 2, SK_STREAM_VIDEO_MASK);
  printf("%s v%d.%d ", streamSupport, SK_VERSION_MAJOR(properties.apiVersion), SK_VERSION_MINOR(properties.apiVersion));
  skPrintUuidUTL(properties.driverUuid);
  putc(' ', stdout);
  printItem(display, item, display->maxLength);
  printf(" %s\n", properties.description);
}

static void
printlong_endpoint(Display *display, DisplayItem *item) {
  SkResult result;
  SkEndpointFeatures features;
  SkEndpointProperties properties;
  char streamSupport[] = "---\0";

  // If we cannot get the endpoint properties, skip the endpoint.
  result = skQueryEndpointProperties((SkEndpoint)item->pUserData, &properties);
  if (result != SK_SUCCESS) {
    return;
  }

  // If we cannot get the endpoint features, assume all false.
  result = skQueryEndpointFeatures((SkEndpoint)item->pUserData, &features);
  if (result == SK_SUCCESS) {
    CHECK_STREAM_SUPPORT(p, 0, SK_STREAM_PCM_MASK);
    CHECK_STREAM_SUPPORT(m, 1, SK_STREAM_MIDI_MASK);
    CHECK_STREAM_SUPPORT(v, 2, SK_STREAM_VIDEO_MASK);
  }

  // Format: {Support} {API-Support} {identifier} {Description}
  printf("%s ", streamSupport);
  skPrintUuidUTL(properties.endpointUuid);
  putc(' ', stdout);
  printItem(display, item, display->maxLength);
  printf(" %s\n", properties.displayName);
}

static void
printlong_device(Display *display, DisplayItem *item) {
  SkResult result;
  SkDeviceFeatures features;
  SkDeviceProperties properties;
  char streamSupport[] = "---\0";

  // If we cannot get the device properties, skip the device.
  result = skQueryDeviceProperties((SkDevice)item->pUserData, &properties);
  if (result != SK_SUCCESS) {
    return;
  }

  // If we cannot get the device features, assume all false.
  result = skQueryDeviceFeatures((SkDevice)item->pUserData, &features);
  if (result == SK_SUCCESS) {
    CHECK_STREAM_SUPPORT(p, 0, SK_STREAM_PCM_MASK);
    CHECK_STREAM_SUPPORT(m, 1, SK_STREAM_MIDI_MASK);
    CHECK_STREAM_SUPPORT(v, 2, SK_STREAM_VIDEO_MASK);
  }

  // Format: {Support} {API-Support} {identifier} {Description}
  printf("%s ", streamSupport);
  skPrintUuidUTL(properties.deviceUuid);
  putc(' ', stdout);
  printItem(display, item, display->maxLength);
  printf(" %s\n", properties.deviceName);
}

static void
printlong(Display *display, DisplayItem *items, size_t itemCount) {
  uint32_t idx;

  // Set all of the display items to have an equivalent max length.
  for (idx = 0; idx < itemCount; ++idx) {
    if (items[idx].stringLength > display->maxLength) {
      display->maxLength = items[idx].stringLength;
    }
  }

  // Print the appropriate SkObject in long-form.
  for (idx = 0; idx < itemCount; ++idx) {
    switch (skGetObjectType((SkObject)items[idx].pUserData)) {
      case SK_OBJECT_TYPE_DRIVER:
        printlong_driver(display, &items[idx]);
        break;
      case SK_OBJECT_TYPE_DEVICE:
        printlong_device(display, &items[idx]);
        break;
      case SK_OBJECT_TYPE_ENDPOINT:
        printlong_endpoint(display, &items[idx]);
        break;
      default:
        break;
    }
  }
}
#undef CHECK_STREAM_SUPPORT

/*******************************************************************************
 * Display Logic
 ******************************************************************************/

static int
displayEntries(Display *display, DisplayItem *items, size_t itemCount) {
  if (display->displayWidth == INFINITE_WIDTH) {
    display->printfnc = &printrow;
  }
  if (display->f_longListing) {
    display->printfnc = &printlong;
  }
  display->sortfnc(items, itemCount);
  display->printfnc(display, items, itemCount);
  return 0;
}

static void
displayUsage() {
  printf(
    "Usage: skls [options] [device path]\n"
    " Prints information about data streaming devices and endpoints on the users machine.\n"
    "\n"
    "Options:\n"
    "  -h, --help             Prints this help documentation.\n"
    "      --format=FORMAT    Controls how information is sorted/displayed in columns.\n"
    "                         Mode1 := (default, multi-column, vertical)\n"
    "                         Mode2 := (single-column)\n"
    "                         Mode3 := (across, horizontal)\n"
    "                         Mode4 := (comma, csv)\n"
    "  -w, --width=COLS       Manually configure the number of columns to print into.\n"
    "                         (Note: skls attempts to automatically figure this out.)\n"
#ifdef    OPENSK_UTILS_COLORS
    "      --color=MODE       Print output with colors depending on mode.\n"
    "                         Mode can be one of (auto, always, none), auto is default.\n"
    "                         Auto is only colored if the application is connected to a tty.\n"
#endif // OPENSK_UTILS_COLORS
    "      --strcmp           Instead of sorting by index, sort by strcmp.\n"
    "  -l, --long             Prints the longform (more detailed information).\n"
    "  -d, --devices          Only requests and enumerates over devices (may be paired with -p or -v).\n"
    "  -e, --endpoints        Only requests and enumerates over endpoints (may be paired with -p or -v).\n"
    "\n"
    "Optionally, skls may be passed an OpenSK device path of the following form: sk://HOST/DEVICE\n"
    " In the case of skls, the 'sk://' may be omitted and will be added to the path for the user if not provided.\n"
    " You do not need to provide the full device path if you want to discover which devices exist under a HOST.\n"
    " The most utl use of skls is `skls -l HOST` to list all of the possible devices/endpoints of a HOST API.\n"
    " Running skls without a path will assume that you wish to discover HOST APIs.\n"
  );
}

static void
generateCanonicalDeviceName(char *pBuffer, uint32_t idx) {
  char temp;
  char *pBufferEnd;

  // Write the prefix for a physical device.
  pBuffer[0] = 'p';
  pBuffer[1] = 'd';
  pBuffer += 2;

  // Write out the physical device letters.
  // Note: The additions and subtractions here can be confusing.
  //       Basically, for this number base, we say that a = 1,
  //       but we also say that a0 is not a + (base). Instead, it is
  //       treated as a continuation into aa. So, first, we pretend that a = 1.
  // Initial: a = 1, b = 2, ..., z = 26, aa = 28, ab = 29, ...
  //       Then, we shift the final number down by the number of digits.
  //       This will move us into our theoretical number space.
  // Result:  a = 0, b = 1, ..., z = 25, aa = 26, ab = 27, ...
  ++idx;
  pBufferEnd = pBuffer;
  do {
    *pBufferEnd = "zabcdefghijklmnopqrstuvwxy"[idx % ('z' - 'a' + 1)];
    --idx;
    idx /= ('z' - 'a' + 1);
    ++pBufferEnd;
  } while (idx);

  // Write the null-terminator, and go back to the last character.
  *pBufferEnd = 0;
  --pBufferEnd;

  // Reverse the string to get it in proper order.
  // Like regular number-to-string operations, we end up with the value in
  // reverse order. So we must reverse the string in order to display the
  // final physical device name.
  while (pBufferEnd > pBuffer) {
    temp = *pBuffer;
    *pBuffer = *pBufferEnd;
    *pBufferEnd = temp;
    ++pBuffer; --pBufferEnd;
  }
}

static void
generateIntegerString(char *pBuffer, uint32_t n) {
  char temp;
  char *pBufferEnd;

  // Write the endpoint number to the buffer.
  pBufferEnd = pBuffer;
  do {
    *pBufferEnd = "0123456789"[n % 10];
    n /= 10;
    ++pBufferEnd;
  } while (n);

  // Write the null-terminator, and go back to the last character.
  *pBufferEnd = 0;
  --pBufferEnd;

  // Reverse the string to get the proper representation.
  while (pBufferEnd > pBuffer) {
    temp = *pBuffer;
    *pBuffer = *pBufferEnd;
    *pBufferEnd = temp;
    ++pBuffer; --pBufferEnd;
  }
}

static void
generateCanonicalEndpointName(char *pBuffer, uint32_t idx) {
  pBuffer[0] = 'e';
  pBuffer[1] = 'p';
  generateIntegerString(pBuffer + 2, idx);
}

static int
displayEndpoint(SklsExecution* exec, SkEndpoint endpoint) {
  SkResult result;
  SkEndpointFeatures features;
  SkEndpointProperties properties;

  (void)exec;

  // Query for features and properties.
  result = skQueryEndpointProperties(endpoint, &properties);
  if (result != SK_SUCCESS) {
    SKERR("Failed to query the endpoint for properties.");
    return 1;
  }
  result = skQueryEndpointFeatures(endpoint, &features);
  if (result != SK_SUCCESS) {
    SKERR("Failed to query the endpoint for features.");
    return 1;
  }

  // Print information about the endpoint
  printf("Endpoint Information\n");
  printf("Display Name: %s\n", properties.displayName);
  printf("Name: %s\n", properties.endpointName);
  printf("UUID: ");
  skPrintUuidUTL(properties.endpointUuid);
  putc('\n', stdout);

  return 0;
}

static int
displayDevice(SklsExecution* exec, SkDevice device) {
  int err;
  uint32_t idx;
  SkResult result;
  SkEndpoint *pEndpoints;
  DisplayItem *pDisplayItems;
  uint32_t totalObjects;
  uint32_t endpointCount;
  SkEndpointProperties endpointProperties;

  // Count the endpoints available on the provided driver.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, NULL);
  if (result != SK_SUCCESS) {
    SKERR("Failed to count the device's endpoints.");
    return 1;
  }

  // Allocate enough space for the devices.
  pDisplayItems = calloc(endpointCount, sizeof(DisplayItem) + sizeof(SkEndpoint));
  if (endpointCount && !pDisplayItems) {
    SKERR("Failed to allocate enough information for %u endpoints.", endpointCount);
    return 1;
  }
  pEndpoints = (SkEndpoint*)&pDisplayItems[endpointCount];

  // Enumerate into the endpoint array for further processing.
  result = skEnumerateDeviceEndpoints(device, &endpointCount, pEndpoints);
  if (result < SK_SUCCESS) {
    SKERR("Failed to enumerate the device endpoints.");
    return 1;
  }

  // Initialize all display items.
  totalObjects = 0;
  for (idx = 0; idx < endpointCount; ++idx) {
    result = skQueryEndpointProperties(pEndpoints[idx], &endpointProperties);
    if (result == SK_SUCCESS) {
      skInitializeDisplayItem(&pDisplayItems[totalObjects], endpointProperties.endpointName, pEndpoints[idx]);
      ++totalObjects;
    }
  }

  // Display all the items as configured.
  err = displayEntries(&exec->display, pDisplayItems, totalObjects);
  skDeinitializeDisplayItems(pDisplayItems, totalObjects);
  free(pDisplayItems);
  return err;
}

static int
displayDriver(SklsExecution* exec, SkDriver driver) {
  int err;
  size_t stringLength;
  uint32_t idx;
  uint32_t ddx;
  char pBuffer[32];
  SkResult result;
  SkDevice *pDevices;
  SkEndpoint *pEndpoints;
  SkEndpoint *pDeviceEndpoints;
  DisplayItem *pDisplayItems;
  uint32_t deviceCount;
  uint32_t totalObjects;
  uint32_t endpointCount;
  uint32_t deviceEndpointCount;
  uint32_t totalDeviceEndpointCount;
  SkDeviceProperties deviceProperties;
  SkEndpointProperties endpointProperties;

  deviceCount = 0;
  endpointCount = 0;

  // Count the devices available on the provided driver.
  if (exec->f_devices) {
    result = skEnumerateDriverDevices(driver, &deviceCount, NULL);
    if (result != SK_SUCCESS) {
      SKERR("Failed to count the driver's devices.");
      return 1;
    }
  }

  // Count the endpoints available on the provided driver.
  if (exec->f_endpoints) {
    result = skEnumerateDriverEndpoints(driver, &endpointCount, NULL);
    if (result != SK_SUCCESS) {
      SKERR("Failed to count the driver's endpoints.");
      return 1;
    }
  }

  // Early-out if there are no objects to query.
  totalObjects = deviceCount + endpointCount;
  if (!totalObjects) {
    return 0;
  }

  // Allocate enough space for the devices and endpoints.
  pDevices = calloc(totalObjects, sizeof(SkObject));
  if (!pDevices) {
    SKERR("Failed to allocate enough information for %u objects.", totalObjects);
    return 1;
  }
  pEndpoints = (SkEndpoint*)&pDevices[deviceCount];

  // Enumerate into the device array for further processing.
  if (exec->f_devices) {
    result = skEnumerateDriverDevices(driver, &deviceCount, pDevices);
    if (result < SK_SUCCESS) {
      SKERR("Failed to enumerate the driver devices.");
      return 1;
    }
  }

  // Enumerate into the endpoint array for further processing.
  if (exec->f_endpoints) {
    result = skEnumerateDriverEndpoints(driver, &endpointCount, pEndpoints);
    if (result < SK_SUCCESS) {
      SKERR("Failed to enumerate the driver endpoints.");
      return 1;
    }
  }

  // Count the total number of device endpoints available
  totalDeviceEndpointCount = 0;
  for (idx = 0; idx < deviceCount; ++idx) {
    result = skEnumerateDeviceEndpoints(pDevices[idx], &deviceEndpointCount, NULL);
    if (result != SK_SUCCESS) {
      SKERR("Failed to count the device's endpoints.");
      return 1;
    }
    totalDeviceEndpointCount += deviceEndpointCount;
  }

  // Allocate enough space for the device endpoints and the total object count.
  totalObjects = deviceCount + endpointCount + totalDeviceEndpointCount;
  pDisplayItems = malloc(sizeof(DisplayItem) * totalObjects + sizeof(SkEndpoint) * totalDeviceEndpointCount);
  if (!pDisplayItems) {
    SKERR("Failed to allocate enough space for %u display items.", totalObjects);
    return 1;
  }
  pDeviceEndpoints = (SkEndpoint*)&pDisplayItems[totalObjects];

  // Initialize all display items.
  totalObjects = 0;
  for (idx = 0; idx < deviceCount; ++idx) {
    result = skQueryDeviceProperties(pDevices[idx], &deviceProperties);
    if (result == SK_SUCCESS) {
      generateCanonicalDeviceName(pBuffer, idx);
      skInitializeDisplayItem(&pDisplayItems[totalObjects], pBuffer, pDevices[idx]);
      ++totalObjects;
      deviceEndpointCount = totalDeviceEndpointCount;
      result = skEnumerateDeviceEndpoints(pDevices[idx], &deviceEndpointCount, pDeviceEndpoints);
      if (result < SK_SUCCESS) {
        SKERR("Failed to enumerate the device endpoints.");
        return 1;
      }
      stringLength = strlen(pBuffer);
      for (ddx = 0; ddx < deviceEndpointCount; ++ddx) {
        result = skQueryEndpointProperties(pDeviceEndpoints[idx], &endpointProperties);
        if (result == SK_SUCCESS) {
          generateIntegerString(&pBuffer[stringLength], ddx);
          skInitializeDisplayItem(&pDisplayItems[totalObjects], pBuffer, pDeviceEndpoints[ddx]);
          ++totalObjects;
        }
      }
    }
  }
  for (idx = 0; idx < endpointCount; ++idx) {
    result = skQueryEndpointProperties(pEndpoints[idx], &endpointProperties);
    if (result == SK_SUCCESS) {
      generateCanonicalEndpointName(pBuffer, idx);
      skInitializeDisplayItem(&pDisplayItems[totalObjects], pBuffer, pEndpoints[idx]);
      ++totalObjects;
    }
  }

  // Display all the items as configured.
  err = displayEntries(&exec->display, pDisplayItems, totalObjects);
  skDeinitializeDisplayItems(pDisplayItems, totalObjects);
  free(pDisplayItems);
  return err;
}

static int
displayInstance(SklsExecution *exec, SkInstance instance) {
  int err;
  uint32_t idx;
  SkResult result;
  SkDriver *pDrivers;
  DisplayItem *pDisplayItems;
  uint32_t driverCount;
  SkDriverProperties properties;

  // Count the drivers available on the system.
  result = skEnumerateInstanceDrivers(instance, &driverCount, NULL);
  if (result != SK_SUCCESS) {
    SKERR("Failed to count the instance drivers.");
    return 1;
  }

  // Allocate enough space for the drivers.
  pDisplayItems = calloc(driverCount, sizeof(DisplayItem) + sizeof(SkDriver));
  if (driverCount && ! pDisplayItems) {
    SKERR("Failed to allocate enough information for %u drivers.", driverCount);
    return 1;
  }
  pDrivers = (SkDriver*)&pDisplayItems[driverCount];

  // Enumerate into the driver array for further processing.
  result = skEnumerateInstanceDrivers(instance, &driverCount, pDrivers);
  if (result < SK_SUCCESS) {
    SKERR("Failed to enumerate the instance drivers.");
    return 1;
  }

  // Initialize all display items.
  for (idx = 0; idx < driverCount; ++idx) {
    skGetDriverProperties(pDrivers[idx], &properties);
    skInitializeDisplayItem(&pDisplayItems[idx], properties.identifier, pDrivers[idx]);
  }

  // Display all the items as configured.
  err = displayEntries(&exec->display, pDisplayItems, driverCount);
  skDeinitializeDisplayItems(pDisplayItems, driverCount);
  free(pDisplayItems);
  return err;
}

static int
printStructure(SklsExecution* exec, char const* path, SkObject object) {
  if (object == SK_NULL_HANDLE) {
    SKERR("Failed to resolve SK Handle '%s'!\n", path);
    return 1;
  }
  switch (skGetObjectType(object)) {
    case SK_OBJECT_TYPE_INSTANCE:
      return displayInstance(exec, object);
    case SK_OBJECT_TYPE_DRIVER:
      return displayDriver(exec, object);
    case SK_OBJECT_TYPE_DEVICE:
      return displayDevice(exec, object);
    case SK_OBJECT_TYPE_ENDPOINT:
      return displayEndpoint(exec, object);
    default:
      SKERR("Handle was resolved, but type is not supported by skls.\n");
      return 1;
  }
}

int main(int argc, char const *argv[]) {
  SklsExecution exec;
  SkObject object;
  uint32_t idx;
  char const* param;
  SkResult result;

  // Configure the default execution policy
  memset(&exec, 0, sizeof(SklsExecution));
  exec.display.f_rowOrder = 1;
  exec.display.printfnc = &printcol;
  exec.display.sortfnc = &sortNone;
  exec.display.separator = ' ';
  exec.display.spacing = 2;
#ifdef    OPENSK_UTILS_COLORS
  exec.display.f_color = (isConsole()) ? 1 : 0;
#endif // OPENSK_UTILS_COLORS

  // Construct the machine information and color configuration objects
  result = skCreatePlatformPLT(NULL, &exec.platform);
  if (result != SK_SUCCESS) {
    SKERR("failed to construct the platform information object.\n");
    return 1;
  }
  result = skCreateColorConfigUTL(exec.platform, NULL, SK_SYSTEM_ALLOCATION_SCOPE_LOADER, &exec.colorConfig);
  if (result != SK_SUCCESS) {
    SKERR("Failed to construct the color configuration object.\n");
    return 1;
  }
  exec.display.colorConfig = exec.colorConfig;

  // Check for terminal information for pretty-printing
  if (isConsole()) {
    exec.display.displayWidth = getConsoleWidth();
  }
  else {
    exec.display.displayWidth = 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (idx = 1; (int64_t)idx < argc; ++idx) {
    param = argv[idx];
    if (skCheckParamUTL(param, "--help")) {
      displayUsage();
      return 0;
    }
    else if (skCheckParamBeginsUTL(param, "--format")) {
      if (param[8] != '=') {
        SKERR("Expected --format to be set to a format string (e.g. --format=default)\n");
        return 1;
      }
      param = &param[9];
      if (skCheckParamUTL(param, "default", "multi-column", "vertical")) {
        exec.display.printfnc   = &printcol;
        exec.display.sortfnc    = &sortNone;
        exec.display.f_rowOrder = 1;
        exec.display.spacing    = 2;
        exec.display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "single-column")) {
        exec.display.printfnc   = &printscol;
        exec.display.sortfnc    = &sortNone;
        exec.display.spacing    = 2;
        exec.display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "horizontal", "across")) {
        exec.display.printfnc   = &printcol;
        exec.display.sortfnc    = &sortNone;
        exec.display.f_rowOrder = 0;
        exec.display.spacing    = 2;
        exec.display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "comma", "csv")) {
        exec.display.printfnc   = &printsrow;
        exec.display.sortfnc    = &sortNone;
        exec.display.separator  = ',';
      }
      else {
        SKERR("Unknown format specified '%s'!\n", param);
        return 1;
      }
    }
    else if (skCheckParamUTL(param, "--strcmp")) {
      exec.display.sortfnc = &sortStrcmp;
    }
    else if (skCheckParamUTL(param, "-w")) {
      ++idx;
      if ((int64_t)idx >= argc) {
        SKERR("Expected to parse an integer width for column count after -w.\n");
        return 1;
      }
      if (sscanf(argv[idx], "%u", &exec.display.displayWidth) != 1) {
        SKERR("Failed to parse the width provided after -w (provided: '%s').\n", argv[idx]);
        return 1;
      }
    }
    else if (skCheckParamBeginsUTL(param, "--width")) {
      if (param[7] != '=') {
        SKERR("Expected --width to be set to an integral value (e.g. --width=80)\n");
        return 1;
      }
      param = &param[8];
      if (sscanf(param, "%u", &exec.display.displayWidth) != 1) {
        SKERR("Failed to parse the width provided after --width=<value> (provided: '%s').\n", param);
        return 1;
      }
    }
#ifdef    OPENSK_UTILS_COLORS
    else if (skCheckParamBeginsUTL(param, "--color")) {
      if (param[7] != '=') {
        SKERR("Expected --color to be set to an integral value (e.g. --color=none)\n");
        return 1;
      }
      param = &param[8];
      if (skCheckParamUTL(param, "none")) {
        exec.display.f_color = 0;
      }
      else if (skCheckParamUTL(param, "auto")) {
        exec.display.f_color = (isConsole()) ? 1 : 0;
      }
      else if (skCheckParamUTL(param, "always")) {
        exec.display.f_color = 1;
      }
      else {
        SKERR("Unknown color option '%s' - valid options are (auto, always, none)\n", param);
        return 1;
      }
    }
#endif // OPENSK_UTILS_COLORS
    else if (skCheckParamUTL(param, "--long")) {
      exec.display.f_longListing = 1;
    }
    else if (skCheckParamUTL(param, "--devices")) {
      exec.f_devices = 1;
    }
    else if (skCheckParamUTL(param, "--endpoints")) {
      exec.f_endpoints = 1;
    }
    else if (param[0] == '-' && param[1] != '-') {
      ++param;
      while (*param) {
        switch (*param) {
          case 'l':
            exec.display.f_longListing = 1;
            break;
          case 'h':
            displayUsage();
            return 0;
          case 'w':
            SKERR("Because -w switch must be provided an argument after it, it cannot be within a short option group.\n");
            return 1;
          case 'd':
            exec.f_devices = 1;
            break;
          case 'e':
            exec.f_endpoints = 1;
            break;
          default:
            SKERR("Unknown short-option: %c\n", *param);
            return 1;
        }
        ++param;
      }
    }
    else {
      ++exec.skPathCount;
      exec.skPaths = realloc(exec.skPaths, sizeof(char const**) * exec.skPathCount);
      exec.skPaths[exec.skPathCount - 1] = param;
    }
  }

#ifdef    OPENSK_UTILS_COLORS
#ifdef    OPENSK_UNIX
  if (exec.display.f_color) {
    (void)signal(SIGINT , &colorquit);
    (void)signal(SIGQUIT, &colorquit);
  }
#endif // OPENSK_UNIX
#endif // OPENSK_UTILS_COLORS

  // Set defaults if no flags are provided for device/endpoint.
  if (!exec.f_devices && !exec.f_endpoints) {
    exec.f_devices = exec.f_endpoints = 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  memset(&applicationInfo, 0, sizeof(SkApplicationInfo));
  applicationInfo.pApplicationName = "skls";
  applicationInfo.pEngineName = "OpenSK";
  applicationInfo.applicationVersion = SK_API_VERSION_0_0;
  applicationInfo.engineVersion = SK_API_VERSION_0_0;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  memset(&instanceInfo, 0, sizeof(SkInstanceCreateInfo));
  instanceInfo.pApplicationInfo = &applicationInfo;

  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (result != SK_SUCCESS) {
    SKERR("Failed to create OpenSK instance: %d\n", result);
    return 1;
  }

  // Print the required object structure
  int err = 0;
  if (exec.skPathCount == 0) {
    displayInstance(&exec, instance);
  } else {
    for (idx = 0; idx < exec.skPathCount; ++idx) {
      if (exec.skPathCount > 1) {
        printf("%s:\n", exec.skPaths[idx]);
      }
      object = skResolveObject(instance, exec.skPaths[idx]);
      err += printStructure(&exec, exec.skPaths[idx], object);
      if (idx < exec.skPathCount - 1) {
        fputc('\n', stdout);
      }
    }
  }

  skDestroyInstance(instance, NULL);
  free(exec.skPaths);
  return err;
}
