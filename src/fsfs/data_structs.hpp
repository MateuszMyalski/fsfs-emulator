#ifndef FSFS_DATA_STRUCTS_HPP
#define FSFS_DATA_STRUCTS_HPP

#include <assert.h>

#include "common/types.hpp"
namespace FSFS {
constexpr int32_t row_size = sizeof(v_size);
constexpr v_size meta_block_size = 64;

// constexpr data super_block_magic_number = 0xDEADC0FU;
constexpr v_size super_block_offset = 0;

enum class block_status : data { USED, FREE };
enum class map_type { INODE, DATA };

struct super_block {
    data magic_number[row_size];
    v_size block_size;
    v_size n_blocks;
    v_size n_inode_blocks;
    data _padding[45];
};
static_assert(sizeof(super_block) == meta_block_size);

struct inode_block {
    block_status type;
    data _padding[3];
    data file_name[32];
    v_size file_len;
    v_size block_ptr[5];
    v_size indirect_inode_ptr;
};
static_assert(sizeof(inode_block) == meta_block_size);

union meta_block {
    super_block MB;
    inode_block inode;
    data raw_data[meta_block_size];
};
static_assert(sizeof(meta_block) == meta_block_size);
}
#endif