add_subdirectory(disk-emulator)
add_subdirectory(file-system)
add_subdirectory(ut-main)

include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(DISK-EMULATOR_UT_SOURCES ${DISK-EMULATOR_UT_SOURCES} ${UT_MAIN_SOURCE})
set(FILE-SYSTEM_UT_SOURCES ${FILE-SYSTEM_UT_SOURCES} ${UT_MAIN_SOURCE})

add_executable(disk-emulator_ut ${DISK-EMULATOR_UT_SOURCES})
target_include_directories(disk-emulator_ut PRIVATE ${INCLUDE_DIRS})
target_link_libraries(disk-emulator_ut libcommands gtest)
target_compile_options(disk-emulator_ut PRIVATE ${COMPILE_FLAGS})
add_test(NAME disk-emulator_ut COMMAND disk-emulator_ut)

add_executable(file-system_ut ${FILE-SYSTEM_UT_SOURCES})
target_include_directories(file-system_ut PRIVATE ${INCLUDE_DIRS})
target_link_libraries(file-system_ut libserver gtest)
target_compile_options(file-system_ut PRIVATE ${COMPILE_FLAGS})
add_test(NAME file-system_ut COMMAND file-system_ut)
