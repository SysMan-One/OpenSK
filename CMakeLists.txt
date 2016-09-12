################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : The main CMake script
################################################################################

cmake_minimum_required(VERSION 3.5)
include(cmake/submodules.cmake)

################################################################################
# Project Initialization
################################################################################
project(OpenSK C)

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
option(OPENSK_BUILD_TESTS "Whether or not to compile test applications" OFF)

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
  include_directories(${ALSA_INCLUDE_DIRS})
  target_sources(sk PRIVATE OpenSK/host/alsa.c)
  target_link_libraries(sk ${ALSA_LIBRARIES})
endif()
if(UDEV_FOUND)
  add_definitions(-DPLUGIN_UDEV)
  include_directories(${UDEV_INCLUDE_DIRS})
  target_link_libraries(sk ${UDEV_LIBRARIES})
endif()

add_library(skx
  OpenSK/ext/fileio.h
  OpenSK/ext/fileio.c
  OpenSK/ext/host_machine.h
  OpenSK/ext/host_machine.c
  OpenSK/ext/ring_buffer.h
  OpenSK/ext/ring_buffer.c
  OpenSK/ext/string_heap.h
  OpenSK/ext/string_heap.c
)
target_link_libraries(skx sk)

add_library(sku
  OpenSK/utl/string.h
  OpenSK/utl/string.c
  OpenSK/utl/color_database.h
  OpenSK/utl/color_database.c
)
target_link_libraries(sku skx)

add_subdirectory(utils)

################################################################################
# CTest
################################################################################

if(OPENSK_BUILD_TESTS)
  find_package(cmocka REQUIRED)
  add_subdirectory(tests)
endif()

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
    OpenSK/ext/fileio.h
    OpenSK/ext/host_machine.h
    OpenSK/ext/ring_buffer.h
    OpenSK/ext/string_heap.h
  DESTINATION include/OpenSK/ext
)
install(
  FILES
    OpenSK/utl/string.h
    OpenSK/utl/color_database.h
  DESTINATION include/OpenSK/utl
)
install(
  TARGETS sk skx sku
  ARCHIVE DESTINATION lib
)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The low-level cross-platform stream kit.")
set(CPACK_PACKAGE_VENDOR "Trent Reed")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${OPENSK_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${OPENSK_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${OPENSK_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "OpenSK-${OPENSK_VERSION_MAJOR}.${OPENSK_VERSION_MINOR}")
set(CPACK_PACKAGE_CONTACT "Trent Reed <me@trentreed.net>")
include(CPack)
