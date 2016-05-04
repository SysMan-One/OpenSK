/*******************************************************************************
 * selkie - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * SelKie sample application (iterates audio devices).
 ******************************************************************************/
#include <selkie/selkie.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXTENSION_COUNT 1

void deviceEventCallbackSEA(void* pUserData, SkDeviceEventSEA* event) {
  (void)pUserData;
  switch (event->type) {
    case SK_DEVICE_EVENT_ADDED_SEA:
      printf("Device Added\n");
      break;
    case SK_DEVICE_EVENT_REMOVED_SEA:
      printf("Device Removed\n");
      break;
  }
}

char const *toRangeModifier(SkRangeDirection dir) {
  switch (dir) {
    case SK_RANGE_DIRECTION_LESS:
      return "-";
    case SK_RANGE_DIRECTION_EQUAL:
      return " ";
    case SK_RANGE_DIRECTION_GREATER:
      return "+";
  }
  return "";
}

void printDeviceComponentLimits(SkDeviceComponent component, SkStreamType streamType) {
  SkDeviceComponentLimits limits;
  skGetDeviceComponentLimits(component, streamType, &limits);
  printf(" |_ Channels   : [%7u , %7u ]\n",    limits.minChannels,         limits.maxChannels);
  printf(" |_ Frame Size : [%7u , %7u ]\n",    limits.minFrameSize,        limits.maxFrameSize);
  printf(" |_ Rate       : [%7u%s, %7u%s]\n",  limits.minRate.value,       toRangeModifier(limits.minRate.direction),       limits.maxRate.value,       toRangeModifier(limits.maxRate.direction));
  printf(" |_ Buffer Time: [%7u%s, %7u%s]\n",  limits.minBufferTime.value, toRangeModifier(limits.minBufferTime.direction), limits.maxBufferTime.value, toRangeModifier(limits.maxBufferTime.direction));
  printf(" |_ Periods    : [%7u%s, %7u%s]\n",  limits.minPeriods.value,    toRangeModifier(limits.minPeriods.direction),    limits.maxPeriods.value,    toRangeModifier(limits.maxPeriods.direction));
  printf(" |_ Period Size: [%7u%s, %7u%s]\n",  limits.minPeriodSize.value, toRangeModifier(limits.minPeriodSize.direction), limits.maxPeriodSize.value, toRangeModifier(limits.maxPeriodSize.direction));
  printf(" \\_ Period Time: [%7u%s, %7u%s]\n", limits.minPeriodTime.value, toRangeModifier(limits.minPeriodTime.direction), limits.maxPeriodTime.value, toRangeModifier(limits.maxPeriodTime.direction));
}

void printDeviceComponentInfo(SkDeviceComponent component) {
  SkDeviceComponentProperties properties;

  skGetDeviceComponentProperties(component, &properties);

  char const *support = "N/A";
  switch (properties.supportedStreams) {
    case SK_STREAM_TYPE_PLAYBACK:
      support = "PLAYBACK";
      break;
    case SK_STREAM_TYPE_CAPTURE:
      support = "CAPTURE";
      break;
    case SK_STREAM_TYPE_CAPTURE | SK_STREAM_TYPE_PLAYBACK:
      support = "PLAYBACK|CAPTURE";
      break;
  }

  printf("=> %s (%s)\n", properties.componentName, support);
  if (properties.supportedStreams & SK_STREAM_TYPE_PLAYBACK) {
    printf(" + Playback Limits\n");
    printDeviceComponentLimits(component, SK_STREAM_TYPE_PLAYBACK);
  }
  if (properties.supportedStreams & SK_STREAM_TYPE_CAPTURE) {
    printf(" + Capture Limits\n");
    printDeviceComponentLimits(component, SK_STREAM_TYPE_CAPTURE);
  }
}

void printPhysicalDeviceInfo(SkPhysicalDevice device) {
  SkResult result;
  SkPhysicalDeviceProperties properties;

  skGetPhysicalDeviceProperties(device, &properties);
  printf("==================================\n");
  printf("Device : %s\n", properties.deviceName);
  printf("Driver : %s\n", properties.driverName);
  printf("Mixer  : %s\n", properties.mixerName );
  if (properties.available == SK_FALSE)
    printf("WARNING: Device is no longer available!");
  printf("----------------------------------\n");

  uint32_t componentCount;
  result = skEnumerateDeviceComponents(device, &componentCount, NULL);
  if (result != SK_SUCCESS) {
    printf("Failed to query the number of device components: %d\n", result);
    abort();
  }

  SkDeviceComponent *components = (SkDeviceComponent*)malloc(sizeof(SkDeviceComponent) * componentCount);
  result = skEnumerateDeviceComponents(device, &componentCount, components);
  if (result != SK_SUCCESS) {
    printf("Failed to fill the array with the properties of device components: %d\n", result);
    abort();
  }

  for (uint32_t idx = 0; idx < componentCount; ++idx) {
    printDeviceComponentInfo(components[idx]);
  }
}

SkBool32 checkForExtension(char const *extensionName, uint32_t extensionCount, SkExtensionProperties *pExtensions) {
  for (uint32_t idx = 0; idx < extensionCount; ++idx) {
    if (strcmp(pExtensions[idx].extensionName, extensionName) == 0) {
      return SK_TRUE;
    }
  }
  return SK_FALSE;
}

int main() {
  //////////////////////////////////////////////////////////////////////////////
  // Check SelKie extensions
  //////////////////////////////////////////////////////////////////////////////
  SkResult result;
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
    printf("\n");
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create SelKie instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "Sample";
  applicationInfo.pEngineName = "Engine";
  applicationInfo.applicationVersion = 1;
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkBool32 devicePollingExt = SK_FALSE;
  uint32_t requiredExtensionCount = 0;
  char const *requiredExtensions[EXTENSION_COUNT];
  if (checkForExtension(SK_SEA_DEVICE_POLLING_EXTENSION_NAME, extensionCount, extensions)) {
    requiredExtensions[requiredExtensionCount] = SK_SEA_DEVICE_POLLING_EXTENSION_NAME;
    ++requiredExtensionCount;
    devicePollingExt = SK_TRUE;
  }

  SkInstanceCreateInfo instanceInfo;
  instanceInfo.flags = 0;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledExtensionCount = requiredExtensionCount;
  instanceInfo.ppEnabledExtensionNames = requiredExtensions;

  SkInstance instance;
  result = skCreateInstance(&instanceInfo, NULL, &instance);
  if (result != SK_SUCCESS) {
    printf("Failed to create SelKie instance: %d\n", result);
    abort();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Enumerate physical devices
  //////////////////////////////////////////////////////////////////////////////
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
    printPhysicalDeviceInfo(devices[idx]);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Register for device callbacks
  //////////////////////////////////////////////////////////////////////////////
  if (devicePollingExt) {
    skRegisterDeviceCallbackSEA(instance, SK_DEVICE_EVENT_ALL_SEA, &deviceEventCallbackSEA, NULL);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cleanup
  //////////////////////////////////////////////////////////////////////////////
  skDestroyInstance(instance);

  return 0;
}
