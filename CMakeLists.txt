################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : The main CMake script
################################################################################

cmake_minimum_required(VERSION 3.5)

################################################################################
# Project Initialization
################################################################################
project(OpenSK C)

################################################################################
# Includes
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/Modules")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Options
################################################################################
option(OPENSK_UTILS_COLORS "Whether or not to compile utilities with color support." ON)

if(OPENSK_UTILS_COLORS)
  add_definitions(-DOPENSK_UTILS_COLORS)
endif()

################################################################################
# Packages
################################################################################
find_package(ALSA)
find_package(UDev)

################################################################################
# Library
################################################################################
include_directories(${ALSA_INCLUDE_DIRS} ${UDEV_INCLUDE_DIRS})
add_library(sk
  OpenSK/opensk.h
  OpenSK/opensk.c
  OpenSK/opensk_platform.h
  OpenSK/dev/assert.h
  OpenSK/dev/hosts.h
  OpenSK/dev/macros.h
)

if(ALSA_FOUND)
  add_definitions(-DHOST_ALSA)
  target_sources(sk PRIVATE OpenSK/host/alsa.c)
  target_link_libraries(sk ${ALSA_LIBRARIES})
endif()
if(UDEV_FOUND)
  add_definitions(-DPLUGIN_UDEV)
  target_link_libraries(sk ${UDEV_LIBRARIES})
endif()

add_library(skx
  OpenSK/ext/utils.h
  OpenSK/ext/utils.c
  OpenSK/ext/ringbuffer.h
  OpenSK/ext/ringbuffer.c
)
add_subdirectory(utils)

################################################################################
# CPack
################################################################################
install(
  FILES
    OpenSK/opensk.h
    OpenSK/opensk_platform.h
  DESTINATION include/OpenSK
)
install(
  FILES
    OpenSK/dev/assert.h
    OpenSK/dev/hosts.h
    OpenSK/dev/macros.h
  DESTINATION include/OpenSK/dev
)
install(
  FILES
    OpenSK/ext/utils.h
    OpenSK/ext/ringbuffer.h
  DESTINATION include/OpenSK/ext
)
install(
  TARGETS sk skx
  ARCHIVE DESTINATION lib
)
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The low-level cross-platform stream kit.")
set(CPACK_PACKAGE_VENDOR "Trent Reed")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "6")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSK-0.0.6")
set(CPACK_PACKAGE_CONTACT "Trent Reed <me@trentreed.net>")
include(CPack)
