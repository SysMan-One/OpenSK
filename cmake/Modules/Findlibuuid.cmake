################################################################################
# Project: OpenSK
# Legal  : Copyright Trent Reed 2016, All rights reserved.
# Author : Trent Reed
# Usage  : find_package(libuuid) to find libuuid libraries.
################################################################################
# Defines the following variables:
# LIBUUID_FOUND        - True if libuuid found.
# LIBUUID_INCLUDE_DIRS - Where to find libuuid include files.
# LIBUUID_LIBRARIES    - List of libraries when using libuuid.

find_path(
  LIBUUID_INCLUDE_DIRS uuid/uuid.h
  PATH_SUFFIXES include
)

find_library(
  LIBUUID_LIBRARIES
  NAMES uuid
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUUID DEFAULT_MSG LIBUUID_LIBRARIES LIBUUID_INCLUDE_DIRS)
mark_as_advanced(LIBUUID_INCLUDE_DIRS LIBUUID_LIBRARIES)
