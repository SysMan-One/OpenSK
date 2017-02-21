################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : The main CMake script.
################################################################################

cmake_minimum_required(VERSION 3.5)
include(cmake/submodules.cmake)
include(cmake/functions.cmake)

################################################################################
# Project Initialization
################################################################################
project(OpenSK C)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

################################################################################
# Includes
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/share;${CMAKE_SOURCE_DIR}/cmake/Modules")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include(Common)
include(FullWarnings)
include(Versioning)

################################################################################
# Options
################################################################################
option(OPENSK_UTILS_COLORS "Whether or not to compile utilities with color support." ON)
option(OPENSK_BUILD_SHARED_LIBRARIES "Whether or not to compile OpenSK shared libraries." ON)
option(OPENSK_BUILD_STATIC_LIBRARIES "Whether or not to compile OpenSK static libraries." ON)
option(OPENSK_BUILD_DRIVERS "Whether or not to build the platform's avaialable client drivers." ON)
option(OPENSK_BUILD_LAYERS "Whether or not to build the avaialable layer libraries." ON)
option(OPENSK_BUILD_UTILS "Whether or not to build the default OpenSK utilities." ON)
set(OPENSK_BUILD_UTILS_TYPE "auto" CACHE STRING "Which libraries to link to for OpenSK utilities.")
set_property(CACHE OPENSK_BUILD_UTILS_TYPE PROPERTY STRINGS auto shared static)

# Validate user's configuration, make sure it makes sense.
if(NOT OPENSK_BUILD_SHARED_LIBRARIES AND NOT OPENSK_BUILD_STATIC_LIBRARIES)
  message(FATAL_ERROR "You must select to build shared and/or static libraries!")
endif()
if(NOT OPENSK_BUILD_STATIC_LIBRARIES AND OPENSK_BUILD_UTILS_TYPE STREQUAL "static")
  message(FATAL_ERROR "You have selected to link utilities to static libraries, but not to build static libraries!")
endif()
if(NOT OPENSK_BUILD_SHARED_LIBRARIES AND OPENSK_BUILD_UTILS_TYPE STREQUAL "shared")
  message(FATAL_ERROR "You have selected to link utilities to shared libraries, but not to build shared libraries!")
endif()

# Select an appropriate build type for utilities if "auto".
if(OPENSK_BUILD_UTILS_TYPE STREQUAL "auto")
  if(OPENSK_BUILD_SHARED_LIBRARIES)
    set(OPENSK_BUILD_UTILS_TYPE "shared")
  elseif(OPENSK_BUILD_STATIC_LIBRARIES)
    set(OPENSK_BUILD_UTILS_TYPE "static")
  else()
    message(FATAL_ERROR "You have selected to build utilities, but not to build any libraries!")
  endif()
endif()

# Add required definitions to the build configuration.
if(OPENSK_UTILS_COLORS)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY COMPILE_DEFINITIONS OPENSK_UTILS_COLORS)
endif()

################################################################################
# Core Library
################################################################################
set(OPENSK_SHARED_BINARY_POSTFIX)
set(OPENSK_STATIC_BINARY_POSTFIX _s)

set(OPENSK_LOCAL_SOURCES
  # Developer helpers
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/json.c
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/json.h
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/md5.c
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/md5.h
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/objects.h
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/string.c
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/string.h
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/utils.c
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/utils.h
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/vector.c
  ${CMAKE_SOURCE_DIR}/OpenSK/dev/vector.h
  # Manifest versions
  ${CMAKE_SOURCE_DIR}/OpenSK/man/manifest.c
  ${CMAKE_SOURCE_DIR}/OpenSK/man/manifest.h
  ${CMAKE_SOURCE_DIR}/OpenSK/man/manifest_1_0_0.c
  ${CMAKE_SOURCE_DIR}/OpenSK/man/manifest_1_0_0.h
  # Platform-Specific Code Abstractions
  ${CMAKE_SOURCE_DIR}/OpenSK/plt/platform.h
)

# Set the default sources for the OpenSK library
set(OPENSK_SOURCES
  # External developer files
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_global.c
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_global.h
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_driver.c
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_driver.h
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_layer.c
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_layer.h
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_loader.c
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_loader.h
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_stream.c
  ${CMAKE_SOURCE_DIR}/OpenSK/ext/sk_stream.h
  # Public Standard API
  ${CMAKE_SOURCE_DIR}/OpenSK/opensk.h
  ${CMAKE_SOURCE_DIR}/OpenSK/opensk.c
  ${CMAKE_SOURCE_DIR}/OpenSK/opensk_platform.h
)

# Handle platform-specific build instructions
# Modifies OPENSK_SOURCES, OPENSK_INCLUDE_DIRECTORIES, and OPENSK_LINK_LIBRARIES
if(UNIX)
  include(${CMAKE_CURRENT_SOURCE_DIR}/PlatformUnix.cmake)
elseif(WIN32)
  include(${CMAKE_CURRENT_SOURCE_DIR}/PlatformWindows.cmake)
else()
  message(FATAL_ERROR "The platform you are building on is not yet supported!")
endif()

set(OPENSK_OBJECT_LIBRARY skLocal_o)
add_library(${OPENSK_OBJECT_LIBRARY} OBJECT ${OPENSK_LOCAL_SOURCES})
set_target_properties(${OPENSK_OBJECT_LIBRARY} PROPERTIES FOLDER "Libraries")

# Configure shared library
if(OPENSK_BUILD_SHARED_LIBRARIES)
  set(OPENSK_SHARED_LIBRARY sk${OPENSK_SHARED_BINARY_POSTFIX})
  add_library(${OPENSK_SHARED_LIBRARY} SHARED ${OPENSK_SOURCES} $<TARGET_OBJECTS:${OPENSK_OBJECT_LIBRARY}>)
  target_link_libraries(${OPENSK_SHARED_LIBRARY} ${OPENSK_LINK_LIBRARIES})
  target_include_directories(${OPENSK_SHARED_LIBRARY} PRIVATE ${OPENSK_INCLUDE_DIRECTORIES})
  set_target_properties(${OPENSK_SHARED_LIBRARY} PROPERTIES COMPILE_DEFINITIONS "SK_EXPORT;OPENSK_SHARED")
  add_custom_command(TARGET ${OPENSK_SHARED_LIBRARY} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIGURATION>"
    "${CMAKE_CURRENT_BINARY_DIR}/utils/$<CONFIGURATION>"
  )
  set(OPENSK_LIBRARIES ${OPENSK_LIBRARIES} ${OPENSK_SHARED_LIBRARY})
endif()

# Configure static library
if(OPENSK_BUILD_STATIC_LIBRARIES)
  set(OPENSK_STATIC_LIBRARY sk${OPENSK_STATIC_BINARY_POSTFIX})
  add_library(${OPENSK_STATIC_LIBRARY} STATIC ${OPENSK_SOURCES} $<TARGET_OBJECTS:${OPENSK_OBJECT_LIBRARY}>)
  target_link_libraries(${OPENSK_STATIC_LIBRARY} ${OPENSK_LINK_LIBRARIES})
  target_include_directories(${OPENSK_STATIC_LIBRARY} PRIVATE ${OPENSK_INCLUDE_DIRECTORIES})
  set_target_properties(${OPENSK_STATIC_LIBRARY} PROPERTIES COMPILE_DEFINITIONS OPENSK_STATIC)
  set(OPENSK_LIBRARIES ${OPENSK_LIBRARIES} ${OPENSK_STATIC_LIBRARY})
endif()

set_target_properties (${OPENSK_LIBRARIES} PROPERTIES FOLDER "Libraries")

################################################################################
# Misc. Binaries and Drivers
################################################################################
if(OPENSK_BUILD_DRIVERS)
  add_subdirectory(drivers)
endif()
if(OPENSK_BUILD_LAYERS)
  add_subdirectory(layers)
endif()
if(OPENSK_BUILD_UTILS)
  add_subdirectory(utils)
endif()

################################################################################
# CPack
################################################################################

# Install the required API headers
install(
  FILES
    OpenSK/opensk.h
    OpenSK/opensk_platform.h
  DESTINATION include/OpenSK
)

# Install the loader extension headers.
install(
  FILES
    OpenSK/ext/sk_global.h
    OpenSK/ext/sk_driver.h
    OpenSK/ext/sk_layer.h
    OpenSK/ext/sk_loader.h
    OpenSK/ext/sk_stream.h
  DESTINATION include/OpenSK/ext
)

# Install the library files
install(
  TARGETS ${OPENSK_LIBRARIES}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

# Configure the installer
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The open-standard cross-platform stream kit.")
set(CPACK_PACKAGE_VENDOR "Trent Reed")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${OPENSK_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${OPENSK_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${OPENSK_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSK-${OPENSK_VERSION_MAJOR}.${OPENSK_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Trent Reed <me@trentreed.net>")
include(CPack)
