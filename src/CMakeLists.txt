add_subdirectory(disk-emulator)
add_subdirectory(fsfs)
add_subdirectory(cli-emulator)
add_subdirectory(common)

add_library(lib_disk-emulator ${DISK-EMULATOR_LIB_SOURCE_FILES})
target_include_directories(lib_disk-emulator PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_disk-emulator PRIVATE ${COMPILE_FLAGS})

add_library(lib_fsfs ${FSFS_LIB_SOURCE_FILES})
target_include_directories(lib_fsfs PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_fsfs PRIVATE ${COMPILE_FLAGS})

add_library(lib_cli-emulator ${CLI-EMULATOR_LIB_SOURCE_FILES})
target_include_directories(lib_cli-emulator PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_cli-emulator PRIVATE ${COMPILE_FLAGS})

