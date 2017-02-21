################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Adds the dependencies for Unix-based platforms.
################################################################################

################################################################################
# Platform Configuration
################################################################################
set(OPENSK_DRIVER_DIRECTORY share/opensk/drivers.d)
set(OPENSK_IMPLICIT_LAYER_DIRECTORY share/opensk/implicit.d)
set(OPENSK_EXPLICIT_LAYER_DIRECTORY share/opensk/explicit.d)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY COMPILE_DEFINITIONS OPENSK_UNIX)

################################################################################
# Platform Dependencies
################################################################################
find_package(libuuid REQUIRED)
find_library(MATH_LIBS m REQUIRED)
set(OPENSK_INCLUDE_DIRECTORIES ${LIBUUID_INCLUDE_DIRS})
set(OPENSK_LINK_LIBRARIES ${CMAKE_DL_LIBS} ${MATH_LIBS} ${LIBUUID_LIBRARIES})

################################################################################
# Platform Sources
################################################################################
set(OPENSK_LOCAL_SOURCES ${OPENSK_LOCAL_SOURCES}
  ${CMAKE_SOURCE_DIR}/OpenSK/plt/platform_unix.c
)
