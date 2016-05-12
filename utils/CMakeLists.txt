################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Helpful utilities for testing the functionality of OpenSK.
################################################################################

find_library(M_LIB m)

add_executable(skinfo skinfo/main.c)
target_link_libraries(skinfo sk)

add_executable(skfreq skfreq/main.c)
target_link_libraries(skfreq sk ${M_LIB})