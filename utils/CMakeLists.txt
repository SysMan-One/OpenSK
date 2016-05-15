################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Helpful utilities for testing the functionality of OpenSK.
################################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
find_library(M_LIB m)

add_library(common common/common.c)
target_link_libraries(common sk)

add_executable(skinfo skinfo/main.c)
target_link_libraries(skinfo sk common)

add_executable(skfreq skfreq/main.c)
target_link_libraries(skfreq sk common ${M_LIB})

add_executable(skplay skplay/main.c)
target_link_libraries(skplay sk common)
