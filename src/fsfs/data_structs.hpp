#ifndef FSFS_DATA_STRUCTS_HPP
#define FSFS_DATA_STRUCTS_HPP

#include <assert.h>

#include "common/types.hpp"
namespace FSFS {
constexpr int16_t fs_system_major = 1;
constexpr int16_t fs_system_minor = 2;
constexpr int32_t fs_data_row_size = sizeof(int32_t);

constexpr int32_t meta_fragm_size_bytes = 64;
constexpr int32_t meta_max_file_name_size = 32;
constexpr int32_t meta_n_direct_ptrs = 5;

constexpr int32_t fs_offset_super_block = 0;
constexpr int32_t fs_offset_inode_block = 1;

const uint8_t meta_magic_seq_lut[] = {0xDE, 0xAD, 0xC0, 0xDE};
const char inode_default_file_name[] = "././DuMmY !@#$%^&*() fIlE nAmE";
constexpr int32_t fs_nullptr = 0xFFFFFFFF;
static_assert(sizeof(inode_default_file_name) < meta_max_file_name_size);

enum class block_status : uint8_t { Free = 0UL, Used };

struct super_block {
    uint8_t magic_number[fs_data_row_size];
    int32_t block_size;
    int32_t n_blocks;
    int32_t n_inode_blocks;
    int32_t n_data_blocks;
    int16_t fs_ver_major;
    int16_t fs_ver_minor;
    uint8_t _padding[36];
    uint32_t checksum;
} __attribute__((aligned(fs_data_row_size)));
static_assert(sizeof(super_block) == meta_fragm_size_bytes);
struct inode_block {
    block_status status;
    uint8_t _padding[3];
    char file_name[meta_max_file_name_size];
    int32_t file_len;
    int32_t direct_ptr[meta_n_direct_ptrs];
    int32_t indirect_inode_ptr;
} __attribute__((aligned(fs_data_row_size)));
static_assert(sizeof(inode_block) == meta_fragm_size_bytes);
}
#endif