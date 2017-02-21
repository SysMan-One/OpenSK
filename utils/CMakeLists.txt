################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Helpful utilities for testing the functionality of OpenSK.
################################################################################

# Create a common utility library for utilities to share.
set(OPENSK_UTILITY_SOURCES
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/allocators.h
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/color_config.c
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/color_config.h
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/error.c
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/error.h
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/macros.h
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/ring_buffer.c
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/ring_buffer.h
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/string.c
  ${CMAKE_SOURCE_DIR}/OpenSK/utl/string.h
)

# Add the platform-specific sources here.
if(UNIX)
  set(OPENSK_UTILITY_SOURCES ${OPENSK_UTILITY_SOURCES}
    ${CMAKE_SOURCE_DIR}/OpenSK/utl/allocators_unix.c
  )
elseif(WIN32)
  set(OPENSK_UTILITY_SOURCES ${OPENSK_UTILITY_SOURCES}
    ${CMAKE_SOURCE_DIR}/OpenSK/utl/allocators_windows.c
  )
else()
  message(FATAL_ERROR "The platform you are building on is not yet supported!")
endif()

# Create the library target.
add_library(skUtility_s STATIC ${OPENSK_UTILITY_SOURCES} $<TARGET_OBJECTS:${OPENSK_OBJECT_LIBRARY}>)
set_target_properties(skUtility_s PROPERTIES FOLDER "Libraries")

# Include different libraries depending on the build type.
if(OPENSK_BUILD_UTILS_TYPE STREQUAL "shared")
  set(UTILITY_LIBS ${MATH_LIBS} ${OPENSK_SHARED_LIBRARY} skUtility_s)
elseif(OPENSK_BUILD_UTILS_TYPE STREQUAL "static")
  message(WARNING "Static build utility type is not fully supported yet!")
  set(UTILITY_LIBS ${MATH_LIBS} ${OPENSK_STATIC_LIBRARY} skUtility_s)
else()
  message(FATAL_ERROR "Unsupported build utility type: ${OPENSK_BUILD_UTILS_TYPE}")
endif()

################################################################################
# skls - Allows for discovery of OpenSK virtual filesystem.
################################################################################
add_executable(skls skls/main.c)
target_link_libraries(skls ${UTILITY_LIBS})
configure_file(skls/SKCOLORS SKCOLORS COPYONLY)
set(OPENSK_UTILS ${OPENSK_UTILS} skls)
set_property(TARGET skls APPEND PROPERTY COMPILE_DEFINITIONS SK_IMPORT)

################################################################################
# skdbg - A debugging tool for OpenSK drivers/extensions.
################################################################################
add_executable(skdbg skdbg/main.c)
target_link_libraries(skdbg ${UTILITY_LIBS})
set_property(TARGET skdbg APPEND PROPERTY COMPILE_DEFINITIONS SK_IMPORT)

################################################################################
# skfreq - A very simple tool which can play PCM frequencies.
################################################################################
add_executable(skfreq skfreq/main.c)
target_link_libraries(skfreq ${UTILITY_LIBS})
set(OPENSK_UTILS ${OPENSK_UTILS} skfreq)
set_property(TARGET skfreq APPEND PROPERTY COMPILE_DEFINITIONS SK_IMPORT)

################################################################################
# skecho - record and playback PCM data.
################################################################################
add_executable(skecho skecho/main.c)
target_link_libraries(skecho ${UTILITY_LIBS})
set(OPENSK_UTILS ${OPENSK_UTILS} skecho)
set_property(TARGET skecho APPEND PROPERTY COMPILE_DEFINITIONS SK_IMPORT)

set_target_properties (${OPENSK_UTILS} PROPERTIES FOLDER "Utils")

install(
  TARGETS ${OPENSK_UTILS}
  RUNTIME DESTINATION bin
)
install(
  FILES skls/SKCOLORS
  DESTINATION /etc
)
