#include "fsfs/file_system.hpp"

#include "disk-emulator/disk.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class FileSystemTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
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
        delete fs;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }
};
TEST_F(FileSystemTest, format) {
    FileSystem::format(*disk);
    fsize real_disk_size = disk->get_disk_size() - 1;
    fsize n_inode_blocks = real_disk_size * 0.1;
    fsize n_data_blocks = real_disk_size - n_inode_blocks;
    ASSERT_EQ(n_inode_blocks + n_data_blocks + 1, disk->get_disk_size());

    disk->mount();
    meta_block r_data[block_size / meta_fragm_size] = {};
    auto status = disk->read(fs_offset_super_block, r_data->raw_data, block_size);

    ASSERT_EQ(status, block_size);
    ASSERT_EQ(r_data[0].MB.magic_number[0], meta_magic_num_lut[0]);
    ASSERT_EQ(r_data[0].MB.magic_number[1], meta_magic_num_lut[1]);
    ASSERT_EQ(r_data[0].MB.magic_number[2], meta_magic_num_lut[2]);
    ASSERT_EQ(r_data[0].MB.magic_number[3], meta_magic_num_lut[3]);
    ASSERT_EQ(r_data[0].MB.n_blocks, disk->get_disk_size());
    ASSERT_EQ(r_data[0].MB.n_data_blocks, n_data_blocks);
    ASSERT_EQ(r_data[0].MB.n_inode_blocks, n_inode_blocks);
    ASSERT_EQ(r_data[0].MB.fs_ver_major, fs_system_major);
    ASSERT_EQ(r_data[0].MB.fs_ver_minor, fs_system_minor);

    for (auto i = 0; i < n_inode_blocks; i++) {
        status =
            disk->read(fs_offset_inode_block + i, r_data->raw_data, block_size);
        ASSERT_EQ(status, block_size);

        for (auto j = 0; j < block_size / meta_fragm_size; j++) {
            for (auto chr_idx = 0; chr_idx < meta_file_name_len; chr_idx++) {
                ASSERT_EQ(r_data[j].inode.file_name[chr_idx],
                          inode_def_file_name[chr_idx]);
            }

            for (auto ptr_idx = 0; ptr_idx < meta_inode_ptr_len; ptr_idx++) {
                ASSERT_EQ(r_data[j].inode.block_ptr[ptr_idx], fs_nullptr);
            }

            ASSERT_EQ(r_data[j].inode.type, block_status::Free);
            ASSERT_EQ(r_data[j].inode.indirect_inode_ptr, fs_nullptr);
        }
    }
    disk->unmount();

    fs->mount();
    fs->unmount();
}

TEST_F(FileSystemTest, set_inode_throw) {
    inode_block dummy_inode = {};
    FileSystem::format(*disk);

    ASSERT_THROW(fs->set_inode(0, dummy_inode), std::invalid_argument);
    fs->mount();

    ASSERT_THROW(fs->set_inode(-1, dummy_inode), std::invalid_argument);
    ASSERT_THROW(fs->set_inode(0xFFFFF, dummy_inode), std::invalid_argument);

    fs->unmount();
}

TEST_F(FileSystemTest, get_inode_throw) {
    inode_block dummy_inode = {};
    FileSystem::format(*disk);

    ASSERT_THROW(fs->get_inode(0, dummy_inode), std::invalid_argument);
    fs->mount();

    ASSERT_THROW(fs->get_inode(-1, dummy_inode), std::invalid_argument);
    ASSERT_THROW(fs->get_inode(0xFFFFF, dummy_inode), std::invalid_argument);

    fs->unmount();
}

TEST_F(FileSystemTest, set_and_get_inode) {
    FileSystem::format(*disk);
    fs->mount();

    const char name[] = "Test inode";

    inode_block ref_inode_one = {};
    inode_block ref_inode_two = {};
    inode_block inode = {};

    ref_inode_one.type = block_status::Used;
    ref_inode_one.file_len = 1024 * 5;
    ref_inode_one.indirect_inode_ptr = fs_nullptr;
    ref_inode_one.block_ptr[0] = 1;
    ref_inode_one.block_ptr[1] = 3;
    ref_inode_one.block_ptr[2] = 4;
    ref_inode_one.block_ptr[3] = 5;
    ref_inode_one.block_ptr[4] = 8;
    std::memcpy(ref_inode_one.file_name, name, sizeof(name));

    ref_inode_two.type = block_status::Used;
    ref_inode_two.file_len = 1024 * 2;
    ref_inode_two.indirect_inode_ptr = fs_nullptr;
    ref_inode_two.block_ptr[0] = 20;
    ref_inode_two.block_ptr[1] = 31;
    ref_inode_two.block_ptr[2] = 451;
    ref_inode_two.block_ptr[3] = 51;
    ref_inode_two.block_ptr[4] = 82;
    std::memcpy(ref_inode_one.file_name, name, sizeof(name));

    fs->set_inode(0, ref_inode_one);
    fs->set_inode(3, ref_inode_two);
    fs->set_inode(block_size / meta_fragm_size + 4, ref_inode_one);

    fs->get_inode(0, inode);

    ASSERT_EQ(ref_inode_one.type, inode.type);
    ASSERT_EQ(ref_inode_one.file_len, inode.file_len);
    ASSERT_EQ(ref_inode_one.block_ptr[0], inode.block_ptr[0]);
    ASSERT_EQ(ref_inode_one.block_ptr[1], inode.block_ptr[1]);
    ASSERT_EQ(ref_inode_one.block_ptr[2], inode.block_ptr[2]);
    ASSERT_EQ(ref_inode_one.block_ptr[3], inode.block_ptr[3]);
    ASSERT_EQ(ref_inode_one.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        ASSERT_EQ(ref_inode_one.file_name[i], inode.file_name[i]);
    }

    fs->get_inode(3, inode);

    ASSERT_EQ(ref_inode_two.type, inode.type);
    ASSERT_EQ(ref_inode_two.file_len, inode.file_len);
    ASSERT_EQ(ref_inode_two.block_ptr[0], inode.block_ptr[0]);
    ASSERT_EQ(ref_inode_two.block_ptr[1], inode.block_ptr[1]);
    ASSERT_EQ(ref_inode_two.block_ptr[2], inode.block_ptr[2]);
    ASSERT_EQ(ref_inode_two.block_ptr[3], inode.block_ptr[3]);
    ASSERT_EQ(ref_inode_two.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        ASSERT_EQ(ref_inode_two.file_name[i], inode.file_name[i]);
    }

    fs->get_inode(block_size / meta_fragm_size + 4, inode);

    ASSERT_EQ(ref_inode_one.type, inode.type);
    ASSERT_EQ(ref_inode_one.file_len, inode.file_len);
    ASSERT_EQ(ref_inode_one.block_ptr[0], inode.block_ptr[0]);
    ASSERT_EQ(ref_inode_one.block_ptr[1], inode.block_ptr[1]);
    ASSERT_EQ(ref_inode_one.block_ptr[2], inode.block_ptr[2]);
    ASSERT_EQ(ref_inode_one.block_ptr[3], inode.block_ptr[3]);
    ASSERT_EQ(ref_inode_one.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        ASSERT_EQ(ref_inode_one.file_name[i], inode.file_name[i]);
    }

    fs->unmount();
}

TEST_F(FileSystemTest, set_data_block_throw) {
    data dummy_data[1] = {};
    FileSystem::format(*disk);

    ASSERT_THROW(fs->set_data_block(0, dummy_data[0]), std::invalid_argument);
    fs->mount();

    ASSERT_THROW(fs->set_data_block(-1, dummy_data[0]), std::invalid_argument);
    ASSERT_THROW(fs->set_data_block(0xFFFFF, dummy_data[0]),
                 std::invalid_argument);

    fs->unmount();
}

TEST_F(FileSystemTest, get_data_block_throw) {
    data dummy_data[1] = {};
    FileSystem::format(*disk);

    ASSERT_THROW(fs->get_data_block(0, dummy_data[0]), std::invalid_argument);
    fs->mount();

    ASSERT_THROW(fs->get_data_block(-1, dummy_data[0]), std::invalid_argument);
    ASSERT_THROW(fs->get_data_block(0xFFFFF, dummy_data[0]),
                 std::invalid_argument);

    fs->unmount();
}

TEST_F(FileSystemTest, set_and_get_block) {
    std::vector<data> ref_data(block_size);
    std::vector<data> r_data(block_size);
    std::memcpy(ref_data.data(), (void*)memcpy, block_size);
    FileSystem::format(*disk);
    fs->mount();

    fs->set_data_block(345, ref_data.data()[0]);
    fs->get_data_block(345, r_data.data()[0]);

    for (auto i = 0; i < block_size; i++) {
        EXPECT_EQ(ref_data[i], r_data[i]);
    }

    fs->unmount();
}
}