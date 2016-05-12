/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK sample application (iterates audio devices).
 ******************************************************************************/
#include <OpenSK/opensk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  printf(" |_ Channels   : [%7u , %7u ]\n",    limits.minChannels,         limits.maxChannels);
  printf(" |_ Frame Size : [%7u , %7u ]\n",    limits.minFrameSize,        limits.maxFrameSize);
  printf(" |_ Rate       : [%7u%s, %7u%s]\n",  limits.minRate.value,       toRangeModifier(limits.minRate.direction),       limits.maxRate.value,       toRangeModifier(limits.maxRate.direction));
  printf(" |_ Buffer Time: [%7u%s, %7u%s]\n",  limits.minBufferTime.value, toRangeModifier(limits.minBufferTime.direction), limits.maxBufferTime.value, toRangeModifier(limits.maxBufferTime.direction));
  printf(" |_ Periods    : [%7u%s, %7u%s]\n",  limits.minPeriods.value,    toRangeModifier(limits.minPeriods.direction),    limits.maxPeriods.value,    toRangeModifier(limits.maxPeriods.direction));
  printf(" |_ Period Size: [%7u%s, %7u%s]\n",  limits.minPeriodSize.value, toRangeModifier(limits.minPeriodSize.direction), limits.maxPeriodSize.value, toRangeModifier(limits.maxPeriodSize.direction));
  printf(" \\_ Period Time: [%7u%s, %7u%s]\n", limits.minPeriodTime.value, toRangeModifier(limits.minPeriodTime.direction), limits.maxPeriodTime.value, toRangeModifier(limits.maxPeriodTime.direction));
}

void printPhysicalComponentInfo(SkPhysicalComponent component) {
  SkComponentProperties properties;

  skGetPhysicalComponentProperties(component, &properties);

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
    printPhysicalComponentLimits(component, SK_STREAM_TYPE_PLAYBACK);
  }
  if (properties.supportedStreams & SK_STREAM_TYPE_CAPTURE) {
    printf(" + Capture Limits\n");
    printPhysicalComponentLimits(component, SK_STREAM_TYPE_CAPTURE);
  }
}

void printVirtualComponentInfo(SkVirtualComponent component) {
  SkComponentProperties properties;

  skGetVirtualComponentProperties(component, &properties);

  SkResult result = SK_INCOMPLETE;
  SkPhysicalComponent physicalComponent = SK_NULL_HANDLE;
  char const *support = "N/A";
  switch (properties.supportedStreams) {
    case SK_STREAM_TYPE_PLAYBACK:
      support = "PLAYBACK";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_PLAYBACK, &physicalComponent);
      break;
    case SK_STREAM_TYPE_CAPTURE:
      support = "CAPTURE";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_CAPTURE, &physicalComponent);
      break;
    case SK_STREAM_TYPE_CAPTURE | SK_STREAM_TYPE_PLAYBACK:
      support = "PLAYBACK|CAPTURE";
      result = skResolvePhysicalComponent(component, SK_STREAM_TYPE_PLAYBACK | SK_STREAM_TYPE_CAPTURE, &physicalComponent);
      break;
  }

  printf("=> %s (%s)\n", properties.componentName, support);
  if (result == SK_SUCCESS) {
    SkPhysicalDevice physicalDevice;
    skResolvePhysicalDevice(physicalComponent, &physicalDevice);

    SkDeviceProperties deviceProperties;
    SkComponentProperties componentProperties;
    skGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    skGetPhysicalComponentProperties(physicalComponent, &componentProperties);

    printf(" \\_ %s (%s)\n", deviceProperties.deviceName, componentProperties.componentName);
  }
}

void printPhysicalDeviceInfo(SkPhysicalDevice device) {
  SkResult result;
  SkDeviceProperties properties;

  skGetPhysicalDeviceProperties(device, &properties);
  printf("==================================\n");
  printf("Device : %s\n", properties.deviceName);
  printf("Driver : %s\n", properties.driverName);
  printf("Mixer  : %s\n", properties.mixerName );
  printf("----------------------------------\n");

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

  for (uint32_t idx = 0; idx < componentCount; ++idx) {
    printPhysicalComponentInfo(components[idx]);
  }
}

void printVirtualDeviceInfo(SkVirtualDevice vDevice) {
  SkResult result;
  SkDeviceProperties properties;

  skGetVirtualDeviceProperties(vDevice, &properties);
  printf("Device : %s\n", properties.deviceName);

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

  for (uint32_t idx = 0; idx < componentCount; ++idx) {
    printVirtualComponentInfo(components[idx]);
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
  // Check OpenSK extensions
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
  // Create OpenSK instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "device";
  applicationInfo.pEngineName = "selkie";
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

  printf(": PHYSICAL DEVICES :::::::::::::::\n");
  for (uint32_t idx = 0; idx < deviceCount; ++idx) {
    printPhysicalDeviceInfo(devices[idx]);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Enumerate virtual devices
  //////////////////////////////////////////////////////////////////////////////
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

  printf("\n: VIRTUAL  DEVICES :::::::::::::::\n");
  for (uint32_t idx = 0; idx < vDeviceCount; ++idx) {
    printVirtualDeviceInfo(vDevices[idx]);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Cleanup
  //////////////////////////////////////////////////////////////////////////////
  skDestroyInstance(instance);

  return 0;
}
