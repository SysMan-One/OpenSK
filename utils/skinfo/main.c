/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK sample application (iterates audio devices).
 ******************************************************************************/
#include <OpenSK/opensk.h>
#include <common/common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * User settings/properties and defaults
 ******************************************************************************/
static int printLong = 0;
static int printVirtual = 0;
static int printPhysical = 0;
static int printExtensions = 0;
static int printPlayback = 0;
static int printCapture = 0;
static int selectDevice = -1;
static int selectComponent = -1;

char const *toRangeModifier(SkRangeDirection dir) {
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

void printPhysicalComponentLimits(SkPhysicalComponent component, SkStreamType streamType) {
  SkComponentLimits limits;
  skGetPhysicalComponentLimits(component, streamType, &limits);
  printf("  |_ Channels   : [%7u , %9u ]\n",    limits.minChannels,         limits.maxChannels);
  printf("  |_ Frame Size : [%7u , %9u ]\n",    limits.minFrameSize,        limits.maxFrameSize);
  printf("  |_ Rate       : [%7u%s, %9u%s]\n",  limits.minRate.value,       toRangeModifier(limits.minRate.direction),       limits.maxRate.value,       toRangeModifier(limits.maxRate.direction));
  printf("  |_ Buffer Time: [%7u%s, %9u%s]\n",  limits.minBufferTime.value, toRangeModifier(limits.minBufferTime.direction), limits.maxBufferTime.value, toRangeModifier(limits.maxBufferTime.direction));
  printf("  |_ Periods    : [%7u%s, %9u%s]\n",  limits.minPeriods.value,    toRangeModifier(limits.minPeriods.direction),    limits.maxPeriods.value,    toRangeModifier(limits.maxPeriods.direction));
  printf("  |_ Period Size: [%7u%s, %9u%s]\n",  limits.minPeriodSize.value, toRangeModifier(limits.minPeriodSize.direction), limits.maxPeriodSize.value, toRangeModifier(limits.maxPeriodSize.direction));
  printf("  \\_ Period Time: [%7u%s, %9u%s]\n", limits.minPeriodTime.value, toRangeModifier(limits.minPeriodTime.direction), limits.maxPeriodTime.value, toRangeModifier(limits.maxPeriodTime.direction));
}

void printPhysicalComponentInfo(uint32_t devidx, uint32_t idx, SkPhysicalComponent component) {
  if (selectComponent >= 0 && idx != selectComponent) return;
  SkComponentProperties properties;

  skGetPhysicalComponentProperties(component, &properties);
  char const *streamDirection = "--";
  switch (properties.supportedStreams) {
    case SK_STREAM_TYPE_PLAYBACK:
      streamDirection = "->";
      break;
    case SK_STREAM_TYPE_CAPTURE:
      streamDirection = "<-";
      break;
    case SK_STREAM_TYPE_CAPTURE | SK_STREAM_TYPE_PLAYBACK:
      streamDirection = "<>";
      break;
  }

  if (!printPlayback) {
    properties.supportedStreams &= ~SK_STREAM_TYPE_PLAYBACK;
  }
  if (!printCapture) {
    properties.supportedStreams &= ~SK_STREAM_TYPE_CAPTURE;
  }
  if (!properties.supportedStreams) {
    return;
  }

  printf("%s:%2u.%-2u \"%s\"\n", streamDirection, devidx, idx, properties.componentName);
  if (printLong) {
    if (properties.supportedStreams & SK_STREAM_TYPE_PLAYBACK) {
      printf("  + Playback Limits\n");
      printPhysicalComponentLimits(component, SK_STREAM_TYPE_PLAYBACK);
    }
    if (properties.supportedStreams & SK_STREAM_TYPE_CAPTURE) {
      printf("  + Capture Limits\n");
      printPhysicalComponentLimits(component, SK_STREAM_TYPE_CAPTURE);
    }
  }
}

void printVirtualComponentInfo(uint32_t devidx, uint32_t idx, SkVirtualComponent component) {
  if (selectComponent >= 0 && idx != selectComponent) return;
  SkComponentProperties properties;

  skGetVirtualComponentProperties(component, &properties);

  SkResult result = SK_INCOMPLETE;
  SkPhysicalComponent physicalComponent = SK_NULL_HANDLE;
  char const *streamDirection = "--";
  switch (properties.supportedStreams) {
    case SK_STREAM_TYPE_PLAYBACK:
      streamDirection = "->";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_PLAYBACK, &physicalComponent);
      break;
    case SK_STREAM_TYPE_CAPTURE:
      streamDirection = "<-";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_CAPTURE, &physicalComponent);
      break;
    case SK_STREAM_TYPE_CAPTURE | SK_STREAM_TYPE_PLAYBACK:
      streamDirection = "<>";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_PLAYBACK | SK_STREAM_TYPE_CAPTURE, &physicalComponent);
      break;
  }

  if (!printPlayback) {
    properties.supportedStreams &= ~SK_STREAM_TYPE_PLAYBACK;
  }
  if (!printCapture) {
    properties.supportedStreams &= ~SK_STREAM_TYPE_CAPTURE;
  }
  if (!properties.supportedStreams) {
    return;
  }

  printf("%s:%2u.%-2u \"%s\"\n", streamDirection, devidx, idx, properties.componentName);
  if (result == SK_SUCCESS) {
    if (printLong) {
      SkPhysicalDevice physicalDevice;
      skResolvePhysicalDevice(physicalComponent, &physicalDevice);

      SkDeviceProperties deviceProperties;
      SkComponentProperties componentProperties;
      skGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
      skGetPhysicalComponentProperties(physicalComponent, &componentProperties);
      printf("  \\_ %s (%s)\n", deviceProperties.deviceName, componentProperties.componentName);
    }
  }
}

void printPhysicalDeviceInfo(uint32_t idx, SkPhysicalDevice device) {
  if (selectDevice >= 0 && idx != selectDevice) return;
  SkResult result;
  SkDeviceProperties properties;

  uint32_t componentCount;
  result = skEnumeratePhysicalComponents(device, &componentCount, NULL);
  if (result != SK_SUCCESS) {
    printf("Failed to query the number of device components: %d\n", result);
    abort();
  }

  SkPhysicalComponent *components = (SkPhysicalComponent*)malloc(sizeof(SkPhysicalComponent) * componentCount);
  result = skEnumeratePhysicalComponents(device, &componentCount, components);
  if (result != SK_SUCCESS) {
    printf("Failed to fill the array with the properties of device components: %d\n", result);
    abort();
  }
  if (selectComponent >= 0 && componentCount <= selectComponent) return;

  uint32_t capture = 0;
  uint32_t playback = 0;
  SkComponentProperties cProperties;
  for (uint32_t cidx = 0; cidx < componentCount; ++cidx) {
    skGetPhysicalComponentProperties(components[cidx], &cProperties);
    if (cProperties.supportedStreams & SK_STREAM_TYPE_PLAYBACK) {
      ++playback;
    }
    if (cProperties.supportedStreams & SK_STREAM_TYPE_CAPTURE) {
      ++capture;
    }
  }

  if (!printPlayback && playback) {
    playback = 0;
  }
  if (!printCapture && capture) {
    capture = 0;
  }
  if (!playback && !capture) {
    return;
  }

  skGetPhysicalDeviceProperties(device, &properties);
  if (printLong) {
    printf("= PD:%2u ================================\n", idx);
    printf("Device : %s\n", properties.deviceName);
    printf("Driver : %s\n", properties.driverName);
    printf("Mixer  : %s\n", properties.mixerName );
    printf("----------------------------------------\n");
  }
  else {
    printf("PD:%2u    \"%s\"\n", idx, properties.deviceName);
  }

  for (uint32_t cidx = 0; cidx < componentCount; ++cidx) {
    printPhysicalComponentInfo(idx, cidx, components[cidx]);
  }
}

void printVirtualDeviceInfo(uint32_t idx, SkVirtualDevice vDevice) {
  if (selectDevice >= 0 && idx != selectDevice) return;
  SkResult result;
  SkDeviceProperties properties;

  uint32_t componentCount;
  result = skEnumerateVirtualComponents(vDevice, &componentCount, NULL);
  if (result != SK_SUCCESS) {
    printf("Failed to query the number of device components: %d\n", result);
    abort();
  }

  SkVirtualComponent *components = (SkVirtualComponent*)malloc(sizeof(SkVirtualComponent) * componentCount);
  result = skEnumerateVirtualComponents(vDevice, &componentCount, components);
  if (result != SK_SUCCESS) {
    printf("Failed to fill the array with the properties of device components: %d\n", result);
    abort();
  }
  if (selectComponent >= 0 && componentCount <= selectComponent) return;

  uint32_t capture = 0;
  uint32_t playback = 0;
  SkComponentProperties cProperties;
  for (uint32_t cidx = 0; cidx < componentCount; ++cidx) {
    skGetVirtualComponentProperties(components[cidx], &cProperties);
    if (cProperties.supportedStreams & SK_STREAM_TYPE_PLAYBACK) {
      ++playback;
    }
    if (cProperties.supportedStreams & SK_STREAM_TYPE_CAPTURE) {
      ++capture;
    }
  }

  if (!printPlayback && playback) {
    playback = 0;
  }
  if (!printCapture && capture) {
    capture = 0;
  }
  if (!playback && !capture) {
    return;
  }

  skGetVirtualDeviceProperties(vDevice, &properties);
  if (printLong) {
    printf("= VD:%2u ================================\n", idx);
    printf("Device : %s\n", properties.deviceName);
    printf("Driver : %s\n", properties.driverName);
    printf("Mixer  : %s\n", properties.mixerName );
    printf("----------------------------------------\n");
  }
  else {
    printf("VD:%2u    \"%s\"\n", idx, properties.deviceName);
  }

  for (uint32_t cidx = 0; cidx < componentCount; ++cidx) {
    printVirtualComponentInfo(idx, cidx, components[cidx]);
  }
}

static void
printUsage() {
  printf(
    "Usage: skinfo [options] [device][.component]\n"
    "Options:\n"
    "  -h, --help             Prints this help documentation.\n"
    "  -P, --playback         Prints only devices/components which performs playback.\n"
    "  -C, --capture          Prints only devices/components which performs capture.\n"
    "  -l, --long             Detailed information about the devices and components.\n"
    "  -p, --physical         Prints only devices which are physical.\n"
    "      --physical-devices\n"
    "  -v, --virtual          Prints only devices which are virtual.\n"
    "      --virtual-devices\n"
    "  -e, --extensions       Print the supported extensions for this installation of OpenSK.\n"
    "\n"
    "Optionally, skinfo may be passed a device.component combo to limit which\n"
    " devices and components will be printed. In example, 0 will print the 0th\n"
    " device (physical or virtual). 0.1 will print the 0th device, 1st component.\n"
    " .2 will print the 2nd component in every device.\n"
  );
}

int main(int argc, char const *argv[]) {

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (int idx = 1; idx < argc; ++idx) {
    char const* param = argv[idx];
    if (param[0] == '-' && param[1] != '-') {
      char const* op = &param[1];
      while (*op) {
        switch (*op) {
          case 'h': printUsage(); return 0;
          case 'l': printLong = 1; break;
          case 'v': printVirtual = 1; break;
          case 'p': printPhysical = 1; break;
          case 'e': printExtensions = 1; break;
          case 'P': printPlayback = 1; break;
          case 'C': printCapture = 1; break;
          default:
            fprintf(stderr, "Invalid command-line option '-%c'.\n", *op);
            return -1;
        }
        ++op;
      }
    }
    else if (genCheckOptions(param, "--help", NULL)) {
      printUsage();
      return 0;
    }
    else if (genCheckOptions(param, "--playback", NULL)) {
      printPlayback = 1;
    }
    else if (genCheckOptions(param, "--capture", NULL)) {
      printCapture = 1;
    }
    else if (genCheckOptions(param, "--long", NULL)) {
      printLong = 1;
    }
    else if (genCheckOptions(param, "--physical", "--physical-devices", NULL)) {
      printPhysical = 1;
    }
    else if (genCheckOptions(param, "--virtual", "--virtual-devices", NULL)) {
      printVirtual = 1;
    }
    else if (genCheckOptions(param, "--extensions", NULL)) {
      printExtensions = 1;
    }
    else if (param[0] == '-' && param[1] == '-') {
      fprintf(stderr, "Invalid command-line option '%s'.\n", param);
      return -1;
    }
    else {
      if (sscanf(param, "%d.%d", &selectDevice, &selectComponent) > 0) {
        // Parsed successfully!
      }
      else if (sscanf(param, ".%d", &selectComponent) > 0) {
        // Parsed successfully!
      }
      else {
        fprintf(stderr, "Failed to parse selected device/component pair. '%s'.\n", param);
        return -1;
      }
    }
  }

  // Default is to print both (only set default if neither are set)
  if (!printPhysical && !printVirtual && !printExtensions) {
    printPhysical = 1;
    printVirtual = 1;
  }
  if (!printPlayback && !printCapture) {
    printPlayback = 1;
    printCapture = 1;
  }

  //////////////////////////////////////////////////////////////////////////////
  // Check OpenSK extensions
  //////////////////////////////////////////////////////////////////////////////
  SkResult result;
  if (printExtensions) {
    uint32_t extensionCount;
    result = skEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    if (result != SK_SUCCESS) {
      printf("Failed to query the number of extensions available: %d\n", result);
      abort();
    }

    SkExtensionProperties *extensions = (SkExtensionProperties*)malloc(sizeof(SkExtensionProperties) * extensionCount);
    result = skEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    if (result != SK_SUCCESS) {
      printf("Failed to fill the array with the properties of available extensions: %d\n", result);
      abort();
    }

    if (extensionCount > 0) {
      printf("Supported Extensions (%u):\n", extensionCount);
      for (uint32_t idx = 0; idx < extensionCount; ++idx) {
        printf("%s (%d.%d.%d)\n",
          extensions[idx].extensionName,
          SK_VERSION_MAJOR(extensions[idx].specVersion),
          SK_VERSION_MINOR(extensions[idx].specVersion),
          SK_VERSION_PATCH(extensions[idx].specVersion)
        );
      }
    }
    else {
      printf("No supported extensions found.\n");
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "skinfo";
  applicationInfo.pEngineName = "opensk";
  applicationInfo.applicationVersion = 1;
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledExtensionCount = 0;
  instanceInfo.ppEnabledExtensionNames = NULL;

  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (result != SK_SUCCESS) {
    printf("Failed to create OpenSK instance: %d\n", result);
    abort();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Enumerate physical devices
  //////////////////////////////////////////////////////////////////////////////
  if (printPhysical) {
    uint32_t deviceCount;
    result = skEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if (result != SK_SUCCESS) {
      printf("Failed to query the number of physical devices: %d\n", result);
      abort();
    }

    SkPhysicalDevice *devices = (SkPhysicalDevice*)malloc(sizeof(SkPhysicalDevice) * deviceCount);
    result = skEnumeratePhysicalDevices(instance, &deviceCount, devices);
    if (result != SK_SUCCESS) {
      printf("Failed to fill the array with the properties of physical devices: %d\n", result);
      abort();
    }

    for (uint32_t idx = 0; idx < deviceCount; ++idx) {
      printPhysicalDeviceInfo(idx, devices[idx]);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Enumerate virtual devices
  //////////////////////////////////////////////////////////////////////////////
  if (printVirtual) {
    uint32_t vDeviceCount;
    result = skEnumerateVirtualDevices(instance, &vDeviceCount, NULL);
    if (result != SK_SUCCESS) {
      printf("Failed to query the number of physical devices: %d\n", result);
      abort();
    }

    SkVirtualDevice *vDevices = (SkVirtualDevice*)malloc(sizeof(SkVirtualDevice) * vDeviceCount);
    result = skEnumerateVirtualDevices(instance, &vDeviceCount, vDevices);
    if (result != SK_SUCCESS) {
      printf("Failed to fill the array with the properties of physical devices: %d\n", result);
      abort();
    }

    for (uint32_t idx = 0; idx < vDeviceCount; ++idx) {
      printVirtualDeviceInfo(idx, vDevices[idx]);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cleanup
  //////////////////////////////////////////////////////////////////////////////
  skDestroyInstance(instance);

  return 0;
}
