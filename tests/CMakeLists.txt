
# All of the sources for unit tests
set(SKTEST_SRC
  unit/SkColorDatabaseUTL.c
  unit/SkHostMachineEXT.c
  unit/SkRingBufferEXT.c
  unit/SkStringHeapEXT.c
  unit/SkStringUTL.c
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/res")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/res/null.sks")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SKCOLORS.empty" "${CMAKE_CURRENT_BINARY_DIR}/res/SKCOLORS.empty" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SKCOLORS.comment" "${CMAKE_CURRENT_BINARY_DIR}/res/SKCOLORS.comment" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SKCOLORS.single" "${CMAKE_CURRENT_BINARY_DIR}/res/SKCOLORS.single" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SKCOLORS.multi" "${CMAKE_CURRENT_BINARY_DIR}/res/SKCOLORS.multi" COPYONLY)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/SKCOLORS.long" "${CMAKE_CURRENT_BINARY_DIR}/res/SKCOLORS.long" COPYONLY)

# Create the include definition
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/unit_suites.inc.in" "${CMAKE_CURRENT_BINARY_DIR}/unit_suites.inc" @ONLY)
foreach(UnitTest ${SKTEST_SRC})
  get_filename_component(TestName ${UnitTest} NAME_WE)
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/unit_suites.inc" "SK_TEST(\"${TestName}\", ${TestName})\n")
endforeach()

# Create the testing application
include_directories(${CMOCKA_INCLUDE_DIRS})
add_executable(sktest
  ${SKTEST_SRC}
  unit/cmocka.h
  unit_runner.c
)
target_link_libraries(sktest sk skx sku ${CMOCKA_LIBRARIES})

# Add unit tests to CTest
foreach(UnitTest ${SKTEST_SRC})
  get_filename_component(TestName ${UnitTest} NAME_WE)
  add_test(NAME ${TestName} COMMAND sktest ${TestName})
endforeach()
