add_subdirectory(disk-emulator)
add_subdirectory(file-system)
add_subdirectory(cli-emulator)
add_subdirectory(utils)

add_library(lib_disk-emulator ${COMMANDS_LIB_SOURCE_FILES})
target_include_directories(lib_disk-emulator PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_disk-emulator PRIVATE ${COMPILE_FLAGS})

add_library(lib_file-system ${LINUX_LIB_SOURCE_FILES})
target_include_directories(lib_file-system PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_file-system PRIVATE ${COMPILE_FLAGS})

add_library(lib_cli-emulator ${LINUX_LIB_SOURCE_FILES})
target_include_directories(lib_cli-emulator PRIVATE ${INCLUDE_DIRS})
target_compile_options(lib_cli-emulator PRIVATE ${COMPILE_FLAGS})

