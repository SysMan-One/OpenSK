################################################################################
# Project: OpenSK
# Legal  : Copyright Trent Reed 2016, All rights reserved.
# Author : Trent Reed
# About  : CMake code for initializing cmake-share git submodules.
################################################################################

################################################################################
# Git Submodule Synchronization
################################################################################
find_package(Git REQUIRED)
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/share/.git")

  # Attempt to synchronize the repository's submodules
  message(STATUS "Synchronizing repository cmake-share submodule...")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" submodule update --init cmake/share
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE SUBMODULE_RESULTS
  )

  # Detect if the submodule was initialized properly
  if(SUBMODULE_RESULTS)
    message(FATAL_ERROR ${SUBMODULE_RESULTS})
  endif()

endif()