cmake_minimum_required(VERSION 3.14)
project(fsFS)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(startup)
add_subdirectory(ut_test)
add_subdirectory(cmake-conf)

add_custom_target(run DEPENDS run COMMAND ${CMAKE_SOURCE_DIR}/build/startup/fsFS)
add_custom_target(run_all_ut DEPENDS run_all_ut COMMAND ${CMAKE_SOURCE_DIR}/build/ut_test/disk-emulator_ut && ${CMAKE_SOURCE_DIR}/build/ut_test/fsfs_ut)