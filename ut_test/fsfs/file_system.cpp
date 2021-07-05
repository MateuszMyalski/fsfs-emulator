#include "fsfs/file_system.hpp"

#include "disk-emulator/disk.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class FileSystemTest : public ::testing::Test {
   protected:
    constexpr static v_size block_size = 1024;
    FileSystem* fs;
    Disk* disk;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        fs = new FileSystem(*disk);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");
    }
    void TearDown() override {
        std::remove("tmp_disk.img");
        delete fs;
        delete disk;
    }
};
TEST_F(FileSystemTest, format) {
    FileSystem::format(*disk);
    v_size real_disk_size = disk->get_disk_size() - 1;
    v_size n_inode_blocks = real_disk_size * 0.1;
    v_size n_data_blocks = real_disk_size - n_inode_blocks;
    ASSERT_EQ(n_inode_blocks + n_data_blocks + 1, disk->get_disk_size());

    disk->mount();
    meta_block r_data[block_size / meta_block_size] = {};
    auto status = disk->read(super_block_offset, r_data->raw_data, block_size);

    ASSERT_EQ(status, block_size);
    ASSERT_EQ(r_data[0].MB.magic_number[0], magic_number_lut[0]);
    ASSERT_EQ(r_data[0].MB.magic_number[1], magic_number_lut[1]);
    ASSERT_EQ(r_data[0].MB.magic_number[2], magic_number_lut[2]);
    ASSERT_EQ(r_data[0].MB.magic_number[3], magic_number_lut[3]);
    ASSERT_EQ(r_data[0].MB.n_blocks, disk->get_disk_size());
    ASSERT_EQ(r_data[0].MB.n_data_blocks, n_data_blocks);
    ASSERT_EQ(r_data[0].MB.n_inode_blocks, n_inode_blocks);
    ASSERT_EQ(r_data[0].MB.fs_ver_major, fs_system_major);
    ASSERT_EQ(r_data[0].MB.fs_ver_minor, fs_system_minor);

    for (auto i = 0; i < n_inode_blocks; i++) {
        status =
            disk->read(inode_blocks_offeset + i, r_data->raw_data, block_size);
        ASSERT_EQ(status, block_size);

        for (auto j = 0; j < block_size / meta_block_size; j++) {
            for (auto chr_idx = 0; chr_idx < file_name_len; chr_idx++) {
                ASSERT_EQ(r_data[j].inode.file_name[chr_idx],
                          dummy_file_name[chr_idx]);
            }

            for (auto ptr_idx = 0; ptr_idx < inode_n_block_ptr_len; ptr_idx++) {
                ASSERT_EQ(r_data[j].inode.block_ptr[ptr_idx], inode_empty_ptr);
            }

            ASSERT_EQ(r_data[j].inode.type, block_status::FREE);
            ASSERT_EQ(r_data[j].inode.indirect_inode_ptr, inode_empty_ptr);
        }
    }
    disk->unmount();

    fs->mount();
    fs->unmount();
}
}