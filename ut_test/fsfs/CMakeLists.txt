set(FSFS_UT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/file_system.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/block_bitmap.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/memory_io.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/memory_io_write_data.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/inode.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/indirect_inode.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/data_block.cpp
                    ${CMAKE_CURRENT_SOURCE_DIR}/dummy.cpp
                    PARENT_SCOPE)
