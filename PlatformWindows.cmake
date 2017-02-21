################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Adds the dependencies for Windows-based platforms.
################################################################################

################################################################################
# Platform Configuration
################################################################################
set(OPENSK_DRIVER_DIRECTORY Drivers)
set(OPENSK_IMPLICIT_LAYER_DIRECTORY ImplicitLayers)
set(OPENSK_EXPLICIT_LAYER_DIRECTORY ExplicitLayers)
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} APPEND PROPERTY COMPILE_DEFINITIONS OPENSK_WINDOWS)

################################################################################
# Platform Dependencies
################################################################################
set(OPENSK_INCLUDE_DIRECTORIES)
set(OPENSK_LINK_LIBRARIES Rpcrt4.lib Ws2_32.lib)

################################################################################
# Platform Sources
################################################################################
set(OPENSK_LOCAL_SOURCES ${OPENSK_LOCAL_SOURCES}
  ${CMAKE_SOURCE_DIR}/OpenSK/plt/platform_windows.c
)
