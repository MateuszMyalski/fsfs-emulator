#ifndef FSFS_DATA_STRUCTS_HPP
#define FSFS_DATA_STRUCTS_HPP

#include <assert.h>

#include "common/types.hpp"
namespace FSFS {
constexpr int16_t fs_system_major = 1;
constexpr int16_t fs_system_minor = 2;
constexpr int32_t fs_data_row_size = sizeof(fsize);

constexpr fsize meta_fragm_size_bytes = 64;
constexpr fsize meta_max_file_name_size = 32;
constexpr fsize meta_n_direct_ptrs = 5;

constexpr address fs_offset_super_block = 0;
constexpr address fs_offset_inode_block = 1;

const data meta_magic_seq_lut[] = {0xDE, 0xAD, 0xC0, 0xDE};
const char inode_default_file_name[] = "././DuMmY !@#$%^&*() fIlE nAmE";
constexpr address fs_nullptr = 0xFFFFFFFF;
static_assert(sizeof(inode_default_file_name) < meta_max_file_name_size);

enum class block_status : data { Free = 0UL, Used };

struct super_block {
    data magic_number[fs_data_row_size];
    fsize block_size;
    fsize n_blocks;
    fsize n_inode_blocks;
    fsize n_data_blocks;
    int16_t fs_ver_major;
    int16_t fs_ver_minor;
    data _padding[35];
    uint32_t checksum;
};
static_assert(sizeof(super_block) == meta_fragm_size_bytes);

struct inode_block {
    block_status status;
    data _padding[3];
    char file_name[meta_max_file_name_size];
    fsize file_len;
    address direct_ptr[meta_n_direct_ptrs];
    address indirect_inode_ptr;
};
static_assert(sizeof(inode_block) == meta_fragm_size_bytes);

union meta_block {
    super_block MB;
    inode_block inode;
    data raw_data[meta_fragm_size_bytes];

    void operator=(data fill_data) { std::fill(raw_data, &raw_data[meta_fragm_size_bytes], fill_data); }
};
static_assert(sizeof(meta_block) == meta_fragm_size_bytes);
}
#endif