set(MAIN_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
set(MAIN_SOURCE_LIBS lib_cli-emulator lib_file_system)
set(CXX_COMPILER_FLAGS ${COMPILE_FLAGS})

add_executable(${CMAKE_PROJECT_NAME} ${MAIN_SOURCE_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${MAIN_SOURCE_LIBS})
target_compile_options (${CMAKE_PROJECT_NAME} PRIVATE ${CXX_COMPILER_FLAGS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${INCLUDE_DIRS})