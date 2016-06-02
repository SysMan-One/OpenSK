################################################################################
# Project: OpenSK
# Legal  : All content 2016 Trent Reed, all rights reserved.
# Author : Trent Reed
# About  : Helpful utilities for testing the functionality of OpenSK.
################################################################################

find_library(M_LIB m)

add_executable(skls skls/main.c)
target_link_libraries(skls sk skx)

install(
  TARGETS skls
  RUNTIME DESTINATION bin
)
install(
  FILES skls/SKCOLORS
  DESTINATION /etc
)