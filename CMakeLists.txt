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
option(OPENSK_COMPILE_UTILITIES "Whether or not to compile utilities." ON)

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
  OpenSK/opensk_platform.h
  OpenSK/opensk_alsa.c
)
target_link_libraries(sk
  ${ALSA_LIBRARIES}
  ${UDEV_LIBRARIES}
)

if(OPENSK_COMPILE_UTILITIES)
  add_subdirectory(utils)
endif()
