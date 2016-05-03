/*******************************************************************************
 * selkie - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * SelKie standard include header.
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
      printf("Device Added\n");
      break;
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

  uint32_t requiredExtensionCount = 0;
  char const *requiredExtensions[EXTENSION_COUNT];
  if (checkForExtension(SK_SEA_DEVICE_POLLING_EXTENSION_NAME, extensionCount, extensions)) {
    requiredExtensions[requiredExtensionCount] = SK_SEA_DEVICE_POLLING_EXTENSION_NAME;
    ++requiredExtensionCount;
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
    printf("Failed to fill the array with the properties of physical components: %d\n", result);
    abort();
  }

  for (uint32_t idx = 0; idx < deviceCount; ++idx) {
    printPhysicalDeviceInfo(devices[idx]);
  }

  // Register for device callbacks
  skRegisterDeviceCallbackSEA(instance, SK_DEVICE_EVENT_ALL_SEA, &deviceEventCallbackSEA, NULL);

  //////////////////////////////////////////////////////////////////////////////
  // Cleanup
  //////////////////////////////////////////////////////////////////////////////
  skDestroyInstance(instance);

  return 0;
}
