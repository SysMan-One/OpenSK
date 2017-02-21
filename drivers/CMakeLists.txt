################################################################################
# Project: OpenSK
# Legal  : Copyright Trent Reed 2016, All rights reserved.
# Author : Trent Reed
# About  : Configures drivers which are valid to build on the target platform.
################################################################################

################################################################################
# ALSA
################################################################################

find_package(ALSA)
find_package(UDev)

if(ALSA_FOUND AND UDEV_FOUND)

  # Add the OpenSK driver using the appropriate extension function
  add_opensk_driver(
    NAME Alsa
    MANIFEST
      ${CMAKE_CURRENT_SOURCE_DIR}/alsa/manifest.json
    INTERFACE
      ${CMAKE_SOURCE_DIR}/OpenSK/icd/alsa.h
    SOURCE
      alsa/alsa.c
      alsa/convert.h
      alsa/udev.c
      alsa/udev.h
      $<TARGET_OBJECTS:${OPENSK_OBJECT_LIBRARY}>
  )

  # Include the libraries and directories for ALSA
  opensk_driver_include_directories(
    Alsa PUBLIC
    ${ALSA_INCLUDE_DIRS}
    ${UDEV_INCLUDE_DIRS}
  )
  opensk_driver_link_libraries(
    Alsa
    ${ALSA_LIBRARIES}
    ${UDEV_LIBRARIES}
  )

endif()

set_target_properties (${OPENSK_DRIVERS} PROPERTIES FOLDER "Drivers")
