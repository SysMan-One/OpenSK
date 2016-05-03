################################################################################
# Project: SelKie
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : The main CMake script
################################################################################

cmake_minimum_required(VERSION 3.5)

################################################################################
# Project Initialization
################################################################################
project(SelKie C)

################################################################################
# Includes
################################################################################
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake/Modules")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

################################################################################
# Options
################################################################################
option(SELKIE_COMPILE_SAMPLES "Whether or not to compile sample projects." OFF)

################################################################################
# Packages
################################################################################
find_package(ALSA)
find_package(UDev)

################################################################################
# Library
################################################################################
include_directories(${ALSA_INCLUDE_DIRS} ${UDEV_INCLUDE_DIRS})
add_library(selkie
  selkie/selkie.h
  selkie/selkie_platform.h
  selkie/selkie_alsa.c
)
target_link_libraries(selkie
  ${ALSA_LIBRARIES}
  ${UDEV_LIBRARIES}
)

if(SELKIE_COMPILE_SAMPLES)
  add_subdirectory(samples)
endif()
