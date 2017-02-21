################################################################################
# Project: OpenSK
# Legal  : Copyright Trent Reed 2014, All rights reserved.
# About  : Creates functions which are commonly used in OpenSK CMakeLists.
# Usage  : include(functions.cmake) to define/use common functions.
################################################################################

################################################################################
# add_opensk_driver(<NAME name> <MANIFEST manifest> <INTERFACE interface> <SOURCE files [...]>)
################################################################################
# A function for configuring the build system to handle a new OpenSK driver.
# All fields of the function are required, and though there is no requirement
# on naming, we suggest heavily that you follow an upper-camelcase format.
# For example, the ALSA driver is turned into Alsa. A driver like Core Audio may
# be represented as CoreAudio, etc.
################################################################################
function(add_opensk_driver)

  # Parse the arguments out of the structure
  parse_arguments(DRIVER
    SINGLE_ARGS "NAME;MANIFEST;INTERFACE"
    MULTI_ARGS "SOURCE"
    ARGUMENTS ${ARGV}
  )

  # Validate user input
  if(NOT DEFINED DRIVER_NAME)
    message(FATAL_ERROR "The driver name was not specified!")
  endif()
  if(NOT EXISTS ${DRIVER_MANIFEST})
    message(FATAL_ERROR "The manifest file specified '${DRIVER_MANIFEST}' does not exist!")
  endif()
  if(IS_DIRECTORY ${DRIVER_MANIFEST})
    message(FATAL_ERROR "The manifest variable must point to a file, not a directory!")
  endif()
  if(NOT EXISTS ${DRIVER_INTERFACE})
    message(FATAL_ERROR "The interface file specified '${DRIVER_INTERFACE}' does not exist!")
  endif()
  if(IS_DIRECTORY ${DRIVER_INTERFACE})
    message(FATAL_ERROR "The interface variable must point to a file, not a directory!")
  endif()

  # Create helper variables
  set(DRIVERS)
  string(TOUPPER DRIVER_NAME UPPERCASE_NAME)
  set(BINARY_NAME skDriver${DRIVER_NAME})
  set(SHARED_DRIVER ${BINARY_NAME})
  set(STATIC_DRIVER ${BINARY_NAME}${OPENSK_STATIC_BINARY_POSTFIX})
  message(STATUS "Configuring driver: ${BINARY_NAME}")

  # Handle the shared driver case
  if(OPENSK_BUILD_SHARED_LIBRARIES)

    # Create the export library
    add_library(${SHARED_DRIVER} SHARED ${DRIVER_INTERFACE} ${DRIVER_SOURCE})
    target_link_libraries(${SHARED_DRIVER} ${OPENSK_SHARED_LIBRARY})
    set_target_properties(${SHARED_DRIVER} PROPERTIES COMPILE_DEFINITIONS "SK_EXPORT;OPENSK_SHARED")
    add_custom_command(
      TARGET ${SHARED_DRIVER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${DRIVER_MANIFEST} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.json
    )
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.json
      DESTINATION ${OPENSK_DRIVER_DIRECTORY}
    )
    set(DRIVERS ${DRIVERS} ${SHARED_DRIVER})
  endif()

  # Handle the static driver case
  if(OPENSK_BUILD_STATIC_LIBRARIES)
    add_library(${STATIC_DRIVER} STATIC ${DRIVER_INTERFACE} ${DRIVER_SOURCE})
    target_link_libraries(${STATIC_DRIVER} ${OPENSK_STATIC_LIBRARY})
    set(DRIVERS ${DRIVERS} ${STATIC_DRIVER})
  endif()

  # Instruct the CMake root-directory that OPENSK_ALSA has been configured
  install(
    FILES ${DRIVER_INTERFACE}
    DESTINATION include/OpenSK/icd
  )
  install(
    TARGETS ${DRIVERS}
    LIBRARY DESTINATION ${OPENSK_DRIVER_DIRECTORY}
    ARCHIVE DESTINATION lib
  )

  set(OPENSK_DRIVERS ${OPENSK_DRIVERS} ${DRIVERS} PARENT_SCOPE)
endfunction()

################################################################################
# opensk_driver_include_directories(<NAME name> <INTERFACE|PUBLIC|PRIVATE dirs [...]>)
################################################################################
# This function includes directories to search when compiling a driver.
# It calls the target_include_directories() function for both the static and/or
# shared binaries.
################################################################################
function(opensk_driver_include_directories NAME)

  # Create helper variables
  set(BINARY_NAME skDriver${NAME})
  set(SHARED_DRIVER ${BINARY_NAME})
  set(STATIC_DRIVER ${BINARY_NAME}${OPENSK_STATIC_BINARY_POSTFIX})
  list(REMOVE_AT ARGV 0)

  # Parse the arguments out of the structure
  parse_arguments(DRIVER
    MULTI_ARGS "INTERFACE;PUBLIC;PRIVATE"
    ARGUMENTS ${ARGV}
  )

  # Include the required directories
  if(OPENSK_BUILD_SHARED_LIBRARIES)
    target_include_directories(${SHARED_DRIVER} INTERFACE ${DRIVER_INTERFACE})
    target_include_directories(${SHARED_DRIVER} PUBLIC ${DRIVER_PUBLIC})
    target_include_directories(${SHARED_DRIVER} PRIVATE ${DRIVER_PRIVATE})
  endif()
  if(OPENSK_BUILD_STATIC_LIBRARIES)
    target_include_directories(${STATIC_DRIVER} INTERFACE ${DRIVER_INTERFACE})
    target_include_directories(${STATIC_DRIVER} PUBLIC ${DRIVER_PUBLIC})
    target_include_directories(${STATIC_DRIVER} PRIVATE ${DRIVER_PRIVATE})
  endif()

endfunction()

################################################################################
# opensk_driver_link_libraries(<NAME name> <libs [...]>)
################################################################################
# Links in the provided libraries by calling the target_link_libraries()
# function on each of the shared and/or static libraries.
################################################################################
function(opensk_driver_link_libraries NAME)

  # Create helper variables
  set(BINARY_NAME skDriver${NAME})
  set(SHARED_DRIVER ${BINARY_NAME})
  set(STATIC_DRIVER ${BINARY_NAME}${OPENSK_STATIC_BINARY_POSTFIX})
  list(REMOVE_AT ARGV 0)

  # Include the required directories
  if(OPENSK_BUILD_SHARED_LIBRARIES)
    target_link_libraries(${SHARED_DRIVER} ${ARGV})
  endif()
  if(OPENSK_BUILD_STATIC_LIBRARIES)
    target_link_libraries(${STATIC_DRIVER} ${ARGV})
  endif()

endfunction()

################################################################################
# add_opensk_layer(<IMPLICIT|EXPLICIT name> <MANIFEST manifest> <INTERFACE interface> <SOURCE file [...]>)
################################################################################
# A function for configuring the build system to compile a layer for shared and/
# or static versions of OpenSK. The main difference between this and driver in
# terms of input is that you cannot just specify a name, you must specity whether
# or not the layer is IMPLICIT or EXPLICIT, which may change the installation
# directory.
################################################################################
function(add_opensk_layer)

  # Parse the arguments out of the structure
  parse_arguments(LAYER
    SINGLE_ARGS "IMPLICIT;EXPLICIT;MANIFEST;INTERFACE"
    MULTI_ARGS "SOURCE"
    ARGUMENTS ${ARGV}
  )

  # Validate user input
  if(NOT EXISTS ${LAYER_MANIFEST})
    message(FATAL_ERROR "The manifest file specified '${LAYER_MANIFEST}' does not exist!")
  endif()
  if(IS_DIRECTORY ${LAYER_MANIFEST})
    message(FATAL_ERROR "The manifest variable must point to a file, not a directory!")
  endif()
  if(NOT LAYER_IMPLICIT AND NOT LAYER_EXPLICIT)
    message(FATAL_ERROR "The layer must be marked as either LAYER_IMPLICIT or LAYER_EXPLICIT!")
  endif()
  if(LAYER_IMPLICIT AND LAYER_EXPLICIT)
    message(FATAL_ERROR "The layer cannot be marked as both LAYER_IMPLICIT and LAYER_EXPLICIT!")
  endif()

  # Determine layer metadata based on parsed settings.
  if(LAYER_IMPLICIT)
    set(LAYER_NAME ${LAYER_IMPLICIT})
    set(LAYER_DIRECTORY ${OPENSK_IMPLICIT_LAYER_DIRECTORY})
  elseif(LAYER_EXPLICIT)
    set(LAYER_NAME ${LAYER_EXPLICIT})
    set(LAYER_DIRECTORY ${OPENSK_EXPLICIT_LAYER_DIRECTORY})
  else()
    message(FATAL_ERROR "The layer type is not currently supported!")
  endif()

  # Create helper variables
  set(LAYERS)
  string(TOUPPER LAYER_NAME UPPERCASE_NAME)
  set(BINARY_NAME skLayer${LAYER_NAME})
  set(SHARED_LAYER ${BINARY_NAME})
  set(STATIC_LAYER ${BINARY_NAME}${OPENSK_STATIC_BINARY_POSTFIX})
  message(STATUS "Configuring layer : ${BINARY_NAME}")

  # Handle the shared layer case
  if(OPENSK_BUILD_SHARED_LIBRARIES)
    add_library(${SHARED_LAYER} MODULE ${LAYER_INTERFACE} ${LAYER_SOURCE})
    target_link_libraries(${SHARED_LAYER} ${OPENSK_SHARED_LIBRARY})
    set_target_properties(${SHARED_LAYER} PROPERTIES COMPILE_DEFINITIONS "SK_EXPORT;OPENSK_SHARED")
    add_custom_command(
      TARGET ${SHARED_LAYER} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${LAYER_MANIFEST} ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.json
    )
    install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/${BINARY_NAME}.json
      DESTINATION ${LAYER_DIRECTORY}
    )
    set(LAYERS ${LAYERS} ${SHARED_LAYER})
  endif()

  # Handle the static layer case
  if(OPENSK_BUILD_STATIC_LIBRARIES)
    add_library(${STATIC_LAYER} STATIC ${LAYER_INTERFACE} ${LAYER_SOURCE})
    target_link_libraries(${STATIC_LAYER} ${OPENSK_STATIC_LIBRARY})
    set(LAYERS ${LAYERS} ${STATIC_LAYER})
  endif()

  # Instruct the CMake root-directory that OPENSK_ALSA has been configured
  install(
    FILES ${LAYER_INTERFACE}
    DESTINATION include/OpenSK/ext
  )
  install(
    TARGETS ${LAYERS}
    LIBRARY DESTINATION ${LAYER_DIRECTORY}
    ARCHIVE DESTINATION lib
  )

  set(OPENSK_LAYERS ${OPENSK_LAYERS} ${LAYERS} PARENT_SCOPE)
endfunction()
