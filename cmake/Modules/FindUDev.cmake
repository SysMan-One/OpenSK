################################################################################
# Project: SelKie
# Legal  : Copyright Trent Reed 2016, All rights reserved.
# Author : Trent Reed
# Usage  : find_package(UDev) to find UDev libraries.
################################################################################
# Defines the following variables:
# UDEV_FOUND        - True if UDev found.
# UDEV_INCLUDE_DIRS - Where to find UDev include files.
# UDEV_LIBRARIES    - List of libraries when using UDev.

find_path(
  UDEV_INCLUDE_DIRS libudev.h
  PATH_SUFFIXES include
)

find_library(
  UDEV_LIBRARIES
  NAMES udev
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UDEV DEFAULT_MSG UDEV_LIBRARIES UDEV_INCLUDE_DIRS)
mark_as_advanced(UDEV_INCLUDE_DIRS UDEV_LIBRARIES)
