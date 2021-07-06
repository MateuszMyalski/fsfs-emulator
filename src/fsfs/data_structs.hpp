#ifndef FSFS_DATA_STRUCTS_HPP
#define FSFS_DATA_STRUCTS_HPP

#include <assert.h>

#include "common/types.hpp"
namespace FSFS {
constexpr int16_t fs_system_major = 1;
constexpr int16_t fs_system_minor = 2;
constexpr int32_t row_size = sizeof(fsize);

constexpr fsize meta_block_size = 64;
constexpr fsize file_name_len = 32;
constexpr fsize inode_n_block_ptr_len = 5;

constexpr address super_block_offset = 0;
constexpr address inode_blocks_offeset = 1;

const data magic_number_lut[] = {0xDE, 0xAD, 0xC0, 0xDE};
const data dummy_file_name[] = "././DuMmY  !@#$%^&*() fIlE nAmE";
constexpr address inode_empty_ptr = 0xFFFFFFFF;

enum class block_status : data { USED, FREE };
enum class map_type { INODE, DATA };

struct super_block {
    data magic_number[row_size];
    fsize block_size;
    fsize n_blocks;
    fsize n_inode_blocks;
    fsize n_data_blocks;
    int16_t fs_ver_major;
    int16_t fs_ver_minor;
    data _padding[35];
    uint32_t checksum;
};
static_assert(sizeof(super_block) == meta_block_size);

struct inode_block {
    block_status type;
    data _padding[3];
    data file_name[file_name_len];
    fsize file_len;
    address block_ptr[inode_n_block_ptr_len];
    address indirect_inode_ptr;
};
static_assert(sizeof(inode_block) == meta_block_size);

union meta_block {
    super_block MB;
    inode_block inode;
    data raw_data[meta_block_size];

    void operator=(data fill_data) {
        std::fill(raw_data, &raw_data[meta_block_size], fill_data);
    }
};
static_assert(sizeof(meta_block) == meta_block_size);
}
#endif