/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK sample application (iterates audio devices).
 *------------------------------------------------------------------------------
 * A high-level overview of how this application works is by first resolving
 * all SkObjects passed into the skls utility. Then, for each SkObject we follow
 * the same pattern (display* -> constructLayout -> print*).
 ******************************************************************************/

// OpenSK Headers
#include <OpenSK/opensk.h>
#include <OpenSK/ext/utils.h>

// C99 Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// Non-Standard Headers
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define ERR(...) fprintf(stderr, __VA_ARGS__); return -1
#define INFINITE_WIDTH 0

/*******************************************************************************
 * Type Declarations / Definitions
 ******************************************************************************/
struct Display;
struct DisplayItem;

typedef void (*pfn_print)(struct Display *display, struct DisplayItem *entries, size_t count);
typedef void (*pfn_sort)(struct DisplayItem *entries, size_t count);

typedef struct DisplayItem {
  void *data;
  SkStringUTL displayName;
  SkStringUTL colorCode;
} DisplayItem;

typedef struct Display {
  char const *path;
  uint32_t f_longListing:1;
  uint32_t f_rowOrder:1;
  uint32_t f_color:1;
  uint32_t f_physical:1;
  uint32_t f_virtual:1;
  uint32_t f_devices:1;
  uint32_t f_components:1;
  uint32_t spacing;
  uint32_t displayWidth;
  size_t maxLength;
  uint32_t extraSpacing[2];
  char separator;
  pfn_sort sortfnc;
  pfn_print printfnc;
  SkColorDatabaseUTL colorDatabase;
} Display;

typedef struct Layout {
  size_t rowCount;
  size_t columnCount;
  size_t *columnWidth;
} Layout;

/*******************************************************************************
 * Static Helper Functions
 ******************************************************************************/
static void
endcolor() {
  fputs("\x1B[0m", stdout);
}

static void
colorquit(int sig)
{
  endcolor();
  (void)signal(sig, SIG_DFL);
  (void)kill(getpid(), sig);
}

static void
printItem(Display *display, DisplayItem *item, size_t width) {
  if (display->f_color && !skStringEmptyUTL(item->colorCode)) {
    printf("\x1B[%sm", skStringDataUTL(item->colorCode));
  }
  printf("%s", skStringDataUTL(item->displayName));
  if (display->f_color && item->colorCode) {
    fputs("\x1B[0m", stdout);
  }
  if (width) {
    printf("%*s", (int)(width - skStringLengthUTL(item->displayName)), "");
  }
}

static char const *
toRangeModifier(SkRangeDirection dir) {
  switch (dir) {
    case SK_RANGE_DIRECTION_LESS:
      return "-";
    case SK_RANGE_DIRECTION_EQUAL:
      return " ";
    case SK_RANGE_DIRECTION_GREATER:
      return "+";
    default:
      break;
  }
  return " ";
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
    maxWidth += skStringLengthUTL(items[maxColumns].displayName) + display->spacing;
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
      if (layout->columnWidth[0] < skStringLengthUTL(items[idx].displayName)) {
        layout->columnWidth[0] = skStringLengthUTL(items[idx].displayName);
      }
    }
  }
  // Case: We need to print a single row with all of the content
  else if (maxColumns == itemCount) {
    layout->rowCount = 1;
    layout->columnCount = itemCount;
    layout->columnWidth = malloc(sizeof(size_t) * itemCount);
    for (idx = 0; idx < itemCount; ++idx) {
      layout->columnWidth[idx] = skStringLengthUTL(items[idx].displayName);
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
        layout->columnWidth[columnIdx] = SK_MAX(
          layout->columnWidth[columnIdx],
          skStringLengthUTL(items[idx].displayName)
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
static void
sortStrcmp(DisplayItem *items, size_t itemCount) {
  DisplayItem tmp;
  uint32_t idx, ddx;
  for (idx = 0; idx < itemCount; ++idx) {
    for (ddx = idx + 1; ddx < itemCount; ++ddx) {
      if (strcmp(skStringDataUTL(items[idx].displayName), skStringDataUTL(items[ddx].displayName)) > 0) {
        tmp = items[ddx];
        items[ddx] = items[idx];
        items[idx] = tmp;
      }
    }
  }
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
    return printscol(display, items, itemCount);
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
#define CHECK_CAPABILITY(id, idx, cap) capabilities[idx] = (properties.capabilities & cap) ? #id[0] : '-'
static void
printlong_host(Display *display, DisplayItem *item) {
  SkHostApiProperties properties;
  char capabilities[] = "----\0";

  skGetHostApiProperties((SkHostApi)item->data, &properties);
  CHECK_CAPABILITY(*, 0, SK_HOST_CAPABILITIES_ALWAYS_PRESENT);
  CHECK_CAPABILITY(p, 1, SK_HOST_CAPABILITIES_PCM);
  CHECK_CAPABILITY(s, 2, SK_HOST_CAPABILITIES_SEQUENCER);
  CHECK_CAPABILITY(v, 3, SK_HOST_CAPABILITIES_VIDEO);
  printf("%s %-*d %-*d ", capabilities,
    display->extraSpacing[0], (display->f_devices)    ? (((display->f_physical) ? properties.physicalDevices : 0) + ((display->f_virtual) ? properties.virtualDevices : 0)) : 0,
    display->extraSpacing[1], (display->f_components) ? (((display->f_physical) ? properties.physicalComponents: 0) + ((display->f_virtual) ? properties.virtualComponents : 0)) : 0
  );
  printItem(display, item, display->maxLength);
  printf(" %s\n", properties.description);
}
#undef CHECK_CAPABILITY

static void
printlong_component(Display *display, DisplayItem *item) {
  SkComponentProperties properties;
  char capabilities[] = "----\0";

  skGetComponentProperties((SkComponent)item->data, &properties);
  printf("%s . %-*s ", capabilities, display->extraSpacing[0], ".");
  printItem(display, item, display->maxLength);
  printf(" %s\n", properties.componentName);
}

static void
printlong_device(Display *display, DisplayItem *item) {
  SkDeviceProperties properties;
  char capabilities[] = "----\0";

  skGetDeviceProperties((SkDevice)item->data, &properties);
  printf("%s 1 %-*d ", capabilities, display->extraSpacing[0], properties.componentCount);
  printItem(display, item, display->maxLength);
  printf(" %s (%s, %s)\n", properties.deviceName, properties.driverName, properties.mixerName);
}

static void
printlong(Display *display, DisplayItem *items, size_t itemCount) {
  uint32_t idx;
  for (idx = 0; idx < itemCount; ++idx) {
    if (skStringLengthUTL(items[idx].displayName) > display->maxLength) {
      display->maxLength = skStringLengthUTL(items[idx].displayName);
    }
  }
  for (idx = 0; idx < itemCount; ++idx) {
    switch (skGetObjectType((SkObject)items[idx].data)) {
      case SK_OBJECT_TYPE_HOST_API:
        printlong_host(display, &items[idx]);
        break;
      case SK_OBJECT_TYPE_DEVICE:
        printlong_device(display, &items[idx]);
        break;
      case SK_OBJECT_TYPE_COMPONENT:
        printlong_component(display, &items[idx]);
        break;
      default:
        break;
    }
  }
}
#undef CHECK_CAPABILITY

/*******************************************************************************
 * Display Logic
 ******************************************************************************/
static void
displayCleanup(DisplayItem *items, size_t itemCount) {
  size_t idx;
  for (idx = 0; idx < itemCount; ++idx) {
    skStringFreeUTL(items[idx].displayName);
    skStringFreeUTL(items[idx].colorCode);
  }
  free(items);
}

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
    " Prints information about data streaming devices and components on the users machine.\n"
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
    "  -l, --long             Prints the longform (more detailed information).\n"
    "  -p, --physical         Only requests and enumerates over physical devices and components.\n"
    "  -v, --virtual          Only requests and enumerates over virtual devices and components.\n"
    "  -d, --devices          Only requests and enumerates over devices (may be paired with -p or -v).\n"
    "  -c, --components       Only requests and enumerates over components (may be paired with -p or -v).\n"
    "\n"
    "Optionally, skls may be passed an OpenSK device path of the following form: sk://HOST/DEVICE\n"
    " In the case of skls, the 'sk://' may be omitted and will be added to the path for the user if not provided.\n"
    " You do not need to provide the full device path if you want to discover which devices exist under a HOST.\n"
    " The most common use of skls is `skls -l HOST` to list all of the possible devices/components of a HOST API.\n"
    " Running skls without a path will assume that you wish to discover HOST APIs.\n"
  );
}

static int
displayLimits(Display* display, SkComponentLimits* limits) {
  printf("|_ Channels   : [%7u , %10u ]\n",      limits->minChannels,         limits->maxChannels);
  printf("|_ Frame Size : [%7" PRIu64 " , %10" PRIu64 " ]\n",                 limits->minFrameSize,                             limits->maxFrameSize);
  printf("|_ Rate       : [%7u%s, %10u%s]\n",    limits->minRate.value,       toRangeModifier(limits->minRate.direction),       limits->maxRate.value,       toRangeModifier(limits->maxRate.direction));
  printf("|_ Buffer Time: [%7u%s, %10u%s]\n",    limits->minBufferTime.value, toRangeModifier(limits->minBufferTime.direction), limits->maxBufferTime.value, toRangeModifier(limits->maxBufferTime.direction));
  printf("|_ Periods    : [%7u%s, %10u%s]\n",    limits->minPeriods.value,    toRangeModifier(limits->minPeriods.direction),    limits->maxPeriods.value,    toRangeModifier(limits->maxPeriods.direction));
  printf("|_ Period Size: [%7u%s, %10u%s]\n",    limits->minPeriodSize.value, toRangeModifier(limits->minPeriodSize.direction), limits->maxPeriodSize.value, toRangeModifier(limits->maxPeriodSize.direction));
  printf("\\_ Period Time: [%7u%s, %10u%s]\n\n", limits->minPeriodTime.value, toRangeModifier(limits->minPeriodTime.direction), limits->maxPeriodTime.value, toRangeModifier(limits->maxPeriodTime.direction));
  printf("Formats:\n");

  SkFormat format;
  uint32_t itemCount = 0;
  DisplayItem *items = calloc(SK_FORMAT_MAX, sizeof(DisplayItem));
  for (format = SK_FORMAT_BEGIN; format != SK_FORMAT_MAX; ++format) {
    if (limits->supportedFormats[format]) {
      skStringCopyUTL(items[itemCount].displayName, skGetFormatString(format));
      ++itemCount;
    }
  }

  printcol(display, items, itemCount);
  displayCleanup(items, itemCount);
  fputc('\n', stdout);
  return 0;
}

#define PRINT_ENUM(e,v) if (e & v) { fputs(#v "\n", stdout); }
static int
displayComponent(Display* display, SkComponent component) {
  SkResult result;
  SkStreamType streamType;
  SkComponentLimits limits;
  SkComponentProperties properties;
  skGetComponentProperties(component, &properties);
  printf("Name: %s\n", properties.componentName);
  if (properties.componentDescription[0] != '\0') {
    printf("Description:\n%s\n", properties.componentDescription);
  }
  fputc('\n', stdout);

  for (streamType = SK_STREAM_TYPE_BEGIN; streamType != SK_STREAM_TYPE_END; streamType <<= 1) {
    if (properties.supportedStreams & streamType) {
      result = skGetComponentLimits(component, streamType, &limits);
      if (result != SK_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to get information about the PCM stream!\n");
        continue;
      }
      printf("*= %s\n", skGetStreamTypeString(streamType));
      displayLimits(display, &limits);
    }
  }
  return 0;
}
#undef PRINT_ENUM

static int
displayDevices(Display* display, SkDevice *devices, uint32_t deviceCount) {
  uint32_t idx;
  SkResult result;
  SkComponent *components;
  SkColorKindUTL colorKind;
  uint32_t componentCount;
  uint32_t totalComponentCount;
  uint32_t objectCount;
  DisplayItem* item;
  DisplayItem* items;
  SkDeviceProperties deviceProperties;
  SkComponentProperties componentProperties;
  uint32_t decimalCount;
  uint32_t totalObjects;
  char const* colorCode;
  int retVal;

  // Get all of the component information
  components = NULL;
  totalComponentCount = 0;
  for (idx = 0 ; idx < deviceCount; ++idx) {
    result = skEnumerateComponents(devices[idx], &componentCount, NULL);
    if (result != SK_SUCCESS) {
      ERR("Failed to query the number of components available on the device.\n");
    }
    components = realloc(components, sizeof(SkComponent) * (totalComponentCount + componentCount));
    if (!components) {
      ERR("Failed to allocate enough information to store components!\n");
    }
    result = skEnumerateComponents(devices[idx], &componentCount, &components[totalComponentCount]);
    if (result != SK_SUCCESS) {
      ERR("Failed to acquire the components available on the device.\n");
    }
    totalComponentCount += componentCount;
  }

  objectCount = deviceCount + totalComponentCount;
  items = calloc(objectCount, sizeof(DisplayItem));

  // Add display items for physical and virtual devices
  for (idx = 0; idx < deviceCount; ++idx) {
    item = &items[idx];
    item->data = devices[idx];
    skGetDeviceProperties(devices[idx], &deviceProperties);
    colorKind = (deviceProperties.isPhysical) ? SKUTL_COLOR_KIND_PHYSICAL_DEVICE : SKUTL_COLOR_KIND_VIRTUAL_DEVICE;
    colorCode = skGetColorCodeUTL(display->colorDatabase, deviceProperties.identifier, colorKind);
    skStringCopyUTL(item->displayName, deviceProperties.identifier);
    if (colorCode) {
      skStringCopyUTL(item->colorCode, colorCode);
    }
    decimalCount = 0;
    totalObjects = deviceProperties.componentCount;
    while (totalObjects > 0) {
      ++decimalCount;
      totalObjects /= 10;
    }
    if (decimalCount > display->extraSpacing[0]) display->extraSpacing[0] = decimalCount;
  }

  // Add display items for physical and virtual components
  for (idx = 0; idx < totalComponentCount; ++idx) {
    item = &items[idx + deviceCount];
    item->data = components[idx];
    skGetComponentProperties(components[idx], &componentProperties);
    colorKind = (componentProperties.isPhysical) ? SKUTL_COLOR_KIND_PHYSICAL_COMPONENT : SKUTL_COLOR_KIND_VIRTUAL_COMPONENT;
    colorCode = skGetColorCodeUTL(display->colorDatabase, deviceProperties.identifier, colorKind);
    skStringCopyUTL(item->displayName, componentProperties.identifier);
    if (colorCode) {
      skStringCopyUTL(item->colorCode, colorCode);
    }
  }

  // Display the proper information depending on user flags
  if (display->f_devices && !display->f_components) {
    retVal = displayEntries(display, items, deviceCount);
  }
  else if (!display->f_devices && display->f_components) {
    retVal = displayEntries(display, &items[deviceCount], objectCount - deviceCount);
  }
  else {
    retVal = displayEntries(display, items, objectCount);
  }

  displayCleanup(items, objectCount);
  free(components);
  return retVal;
}

static int
displayHostApi(Display* display, SkHostApi hostApi) {
  SkResult result;
  SkDevice*devices;
  uint32_t physicalDeviceCount;
  uint32_t virtualDeviceCount;
  int retVal;

  // Get all of the device information
  devices = NULL;
  physicalDeviceCount = 0;
  if (display->f_physical) {
    result = skEnumeratePhysicalDevices(hostApi, &physicalDeviceCount, NULL);
    if (result != SK_SUCCESS) {
      ERR("Failed to query the number of physical devices available on the Host API.\n");
    }
    devices = realloc(devices, sizeof(SkDevice) * physicalDeviceCount);
    if (!devices) {
      ERR("Failed to allocate enough information to store physical devices!\n");
    }
    result = skEnumeratePhysicalDevices(hostApi, &physicalDeviceCount, devices);
    if (result != SK_SUCCESS) {
      ERR("Failed to acquire the physical devices available on the Host API.\n");
    }
  }
  virtualDeviceCount = 0;
  if (display->f_virtual) {
    result = skEnumerateVirtualDevices(hostApi, &virtualDeviceCount, NULL);
    if (result != SK_SUCCESS) {
      ERR("Failed to query the number of virtual devices available on the Host API.\n");
    }
    devices = realloc(devices, sizeof(SkDevice) * (physicalDeviceCount + virtualDeviceCount));
    if (!devices) {
      ERR("Failed to allocate enough information to store virtual devices!\n");
    }
    result = skEnumerateVirtualDevices(hostApi, &virtualDeviceCount, &devices[physicalDeviceCount]);
    if (result != SK_SUCCESS) {
      ERR("Failed to acquire the virtual devices available on the Host API.\n");
    }
  }

  retVal = displayDevices(display, devices, physicalDeviceCount + virtualDeviceCount);
  free(devices);
  return retVal;
}

static int
displayInstance(Display *display, SkInstance instance) {
  SkResult result;
  char const* colorCode;
  uint32_t totalObjects;
  uint32_t decimalCount;
  int retVal;

  uint32_t hostCount;
  result = skEnumerateHostApi(instance, &hostCount, NULL);
  if (result != SK_SUCCESS) {
    ERR("Failed to query the number of host APIs available on the device.\n");
  }

  SkHostApi* hostApi = malloc(sizeof(SkHostApi) * hostCount);
  result = skEnumerateHostApi(instance, &hostCount, hostApi);
  if (result != SK_SUCCESS) {
    ERR("Failed to enumerate the available host APIs on the device.\n");
  }

  SkHostApiProperties properties;
  DisplayItem* items = calloc(hostCount, sizeof(DisplayItem));
  for (uint32_t idx = 0; idx < hostCount; ++idx) {

    // Refresh the API (populate devices)
    result = skScanDevices(hostApi[idx]);
    if (result != SK_SUCCESS) {
      ERR("Failed to refresh device list on the host API.\n");
    }
    skGetHostApiProperties(hostApi[idx], &properties);

    // Count number of devices
    decimalCount = 0;
    totalObjects = 0;
    if (display->f_devices) {
      if (display->f_physical) totalObjects += properties.physicalDevices;
      if (display->f_virtual)  totalObjects += properties.virtualDevices;
    }
    while (totalObjects > 0) {
      ++decimalCount;
      totalObjects /= 10;
    }
    if (decimalCount > display->extraSpacing[0]) display->extraSpacing[0] = decimalCount;

    // Count number of components
    decimalCount = 0;
    totalObjects = 0;
    if (display->f_components) {
      if (display->f_physical) totalObjects += properties.physicalComponents;
      if (display->f_virtual)  totalObjects += properties.virtualComponents;
    }
    while (totalObjects > 0) {
      ++decimalCount;
      totalObjects /= 10;
    }
    if (decimalCount > display->extraSpacing[1]) display->extraSpacing[1] = decimalCount;

    // Configure the DisplayItem
    items[idx].data = hostApi[idx];
    skStringCopyUTL(items[idx].displayName, properties.identifier);
    colorCode = skGetColorCodeUTL(display->colorDatabase, properties.identifier, SKUTL_COLOR_KIND_HOSTAPI + properties.capabilities);
    if (colorCode) {
      skStringCopyUTL(items[idx].colorCode, colorCode);
    }
    if (display->maxLength < skStringLengthUTL(items[idx].displayName)) {
      display->maxLength = skStringLengthUTL(items[idx].displayName);
    }
  }

  retVal = displayEntries(display, items, hostCount);
  displayCleanup(items, hostCount);
  free(hostApi);
  return retVal;
}

static int
printStructure(char const* path, Display* display, SkObject object) {
  if (object == SK_NULL_HANDLE) {
    ERR("Failed to resolve SK Handle '%s'!\n", path);
  }
  switch (skGetObjectType(object)) {
    case SK_OBJECT_TYPE_INSTANCE:
      return displayInstance(display, (SkInstance)object);
    case SK_OBJECT_TYPE_HOST_API:
      return displayHostApi(display, (SkHostApi)object);
    case SK_OBJECT_TYPE_DEVICE:
      return displayDevices(display, (SkDevice*)&object, 1);
    case SK_OBJECT_TYPE_COMPONENT:
      return displayComponent(display, (SkComponent)object);
    default:
      ERR("Handle was resolved, but type is not supported by skls.\n");
  }
}

int main(int argc, char const *argv[]) {
  SkResult result;
  SkObject object;
  char* pChar;
  char const* pCharConst;
  char const** input;
  uint32_t inputCount;
  struct winsize winSize;
  Display display;

  if (argc < 1) {
    displayUsage();
    return 0;
  }

  // Initialize important data
  pChar = NULL;
  input = NULL;
  inputCount = 0;

  // Configure the default Display
  memset(&display, 0, sizeof(Display));
  display.f_rowOrder = 1;
  display.printfnc = &printcol;
  display.sortfnc = &sortStrcmp;
  display.separator = ' ';
  display.spacing = 2;
#ifdef    OPENSK_UTILS_COLORS
  display.f_color = (isatty(STDOUT_FILENO)) ? 1 : 0;
#endif // OPENSK_UTILS_COLORS
  result = skConstructColorDatabaseUTL(&display.colorDatabase, NULL);
  if (result != SK_SUCCESS) {
    ERR("Failed to construct the color database!\n");
  }

  // Check for terminal information for pretty-printing
  if (isatty(STDOUT_FILENO)) {
    pCharConst = getenv("COLUMNS");
    if (pCharConst != NULL && *pCharConst != '\0') {
      display.displayWidth = (uint32_t)atoi(pCharConst);
    }
    else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) != -1) {
      if (winSize.ws_col > 0) display.displayWidth = winSize.ws_col;
    }
    else {
      display.displayWidth = 1;
    }
  }
  else {
    display.displayWidth = 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (int idx = 1; idx < argc; ++idx) {
    char const* param = argv[idx];
    if (skCheckParamUTL(param, "--help")) {
      displayUsage();
      return 0;
    }
    else if (skCheckParamBeginsUTL(param, "--format")) {
      if (param[8] != '=') {
        ERR("Expected --format to be set to a format string (e.g. --format=default)\n");
      }
      param = &param[9];
      if (skCheckParamUTL(param, "default", "multi-column", "vertical")) {
        display.printfnc   = &printcol;
        display.sortfnc    = &sortStrcmp;
        display.f_rowOrder = 1;
        display.spacing    = 2;
        display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "single-column")) {
        display.printfnc   = &printscol;
        display.sortfnc    = &sortStrcmp;
        display.spacing    = 2;
        display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "horizontal", "across")) {
        display.printfnc   = &printcol;
        display.sortfnc    = &sortStrcmp;
        display.f_rowOrder = 0;
        display.spacing    = 2;
        display.separator  = ' ';
      }
      else if (skCheckParamUTL(param, "comma", "csv")) {
        display.printfnc   = &printsrow;
        display.sortfnc    = &sortStrcmp;
        display.separator  = ',';
      }
      else {
        ERR("Unknown format specified '%s'!\n", param);
      }
    }
    else if (skCheckParamUTL(param, "-w")) {
      ++idx;
      if (idx >= argc) {
        ERR("Expected to parse an integer width for column count after -w.\n");
      }
      if (sscanf(argv[idx], "%d", &display.displayWidth) != 1) {
        ERR("Failed to parse the width provided after -w (provided: '%s').\n", argv[idx]);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--width")) {
      if (param[7] != '=') {
        ERR("Expected --width to be set to an integral value (e.g. --width=80)\n");
      }
      param = &param[8];
      if (sscanf(param, "%d", &display.displayWidth) != 1) {
        ERR("Failed to parse the width provided after --width=<value> (provided: '%s').\n", param);
      }
    }
#ifdef    OPENSK_UTILS_COLORS
    else if (skCheckParamBeginsUTL(param, "--color")) {
      if (param[7] != '=') {
        ERR("Expected --color to be set to an integral value (e.g. --color=none)\n");
      }
      param = &param[8];
      if (skCheckParamUTL(param, "none")) {
        display.f_color = 0;
      }
      else if (skCheckParamUTL(param, "auto")) {
        display.f_color = (isatty(STDOUT_FILENO)) ? 1 : 0;
      }
      else if (skCheckParamUTL(param, "always")) {
        display.f_color = 1;
      }
      else {
        ERR("Unknown color option '%s' - valid options are (auto, always, none)\n", param);
      }
    }
#endif // OPENSK_UTILS_COLORS
    else if (skCheckParamUTL(param, "--long")) {
      display.f_longListing = 1;
    }
    else if (skCheckParamUTL(param, "--physical")) {
      display.f_physical = 1;
    }
    else if (skCheckParamUTL(param, "--virtual")) {
      display.f_virtual = 1;
    }
    else if (skCheckParamUTL(param, "--devices")) {
      display.f_devices = 1;
    }
    else if (skCheckParamUTL(param, "--components")) {
      display.f_components = 1;
    }
    else if (param[0] == '-' && param[1] != '-') {
      ++param;
      while (*param) {
        switch (*param) {
          case 'l':
            display.f_longListing = 1;
            break;
          case 'h':
            displayUsage();
            return 0;
          case 'w':
            ERR("Because -w switch must be provided an argument after it, it cannot be within a short option group.\n");
          case 'p':
            display.f_physical = 1;
            break;
          case 'v':
            display.f_virtual = 1;
            break;
          case 'd':
            display.f_devices = 1;
            break;
          case 'c':
            display.f_components = 1;
            break;
          default:
            ERR("Unknown short-option: %c\n", *param);
        }
        ++param;
      }
    }
    else {
      ++inputCount;
      input = realloc(input, sizeof(char const**) * inputCount);
      input[inputCount - 1] = param;
    }
  }

#ifdef    OPENSK_UTILS_COLORS
  if (display.f_color) {
    (void)signal(SIGINT , &colorquit);
    (void)signal(SIGQUIT, &colorquit);
  }
#endif // OPENSK_UTILS_COLORS

  if (!display.f_physical && !display.f_virtual) {
    display.f_physical = display.f_virtual = 1;
  }
  if (!display.f_devices && !display.f_components) {
    display.f_devices = display.f_components = 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "skls";
  applicationInfo.pEngineName = "opensk";
  applicationInfo.applicationVersion = 1;
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  instanceInfo.flags = SK_INSTANCE_DEFAULT;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledExtensionCount = 0;
  instanceInfo.ppEnabledExtensionNames = NULL;

  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (result != SK_SUCCESS) {
    ERR("Failed to create OpenSK instance: %d\n", result);
  }

  int err = 0;
  if (inputCount == 0) {
    displayInstance(&display, instance);
  } else {
    for (uint32_t idx = 0; idx < inputCount; ++idx) {
      if (inputCount > 1) {
        printf("%s:\n", input[idx]);
      }
      if (!skCheckParamBeginsUTL(input[idx], "sk://")) {
        pChar = realloc(pChar, sizeof(char) * (strlen(input[idx]) + 6));
        strcpy(pChar, "sk://");
        strcpy(&pChar[5], input[idx]);
        object = skRequestObject(instance, pChar);
        err += printStructure(pChar, &display, object);
      }
      else {
        object = skRequestObject(instance, input[idx]);
        err += printStructure(input[idx], &display, object);
      }
      if (idx < inputCount - 1) {
        fputc('\n', stdout);
      }
    }
  }
  free(pChar);
  free(input);

  skDestroyInstance(instance);
  return err;
}
