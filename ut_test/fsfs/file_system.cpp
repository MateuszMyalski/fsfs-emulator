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
    MemoryIO* dbg_io;

    super_block MB;
    std::vector<address> used_data_blocks;

   public:
    void SetUp() override {
        Disk::create("tmp_disk.img", 1024 * 10, block_size);

        disk = new Disk(block_size);
        disk->open("tmp_disk.img");

        fs = new FileSystem(*disk);
        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        dbg_io = new MemoryIO(*disk);
        dbg_io->init(MB);
    }
    void TearDown() override {
        delete fs;
        delete dbg_io;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }

    bool get_inode_status(address inode_n) {
        dbg_io->scan_blocks();
        return dbg_io->get_inode_bitmap().get_block_status(inode_n);
    }
};
TEST_F(FileSystemTest, format) {
    fsize real_disk_size = disk->get_disk_size() - 1;
    fsize n_inode_blocks = real_disk_size * 0.1;
    fsize n_data_blocks = real_disk_size - n_inode_blocks;
    ASSERT_EQ(n_inode_blocks + n_data_blocks + 1, disk->get_disk_size());

    disk->mount();
    meta_block r_data[block_size / meta_fragm_size] = {};
    auto status =
        disk->read(fs_offset_super_block, r_data->raw_data, block_size);

    EXPECT_EQ(status, block_size);
    EXPECT_EQ(r_data[0].MB.magic_number[0], meta_magic_num_lut[0]);
    EXPECT_EQ(r_data[0].MB.magic_number[1], meta_magic_num_lut[1]);
    EXPECT_EQ(r_data[0].MB.magic_number[2], meta_magic_num_lut[2]);
    EXPECT_EQ(r_data[0].MB.magic_number[3], meta_magic_num_lut[3]);
    EXPECT_EQ(r_data[0].MB.n_blocks, disk->get_disk_size());
    EXPECT_EQ(r_data[0].MB.n_data_blocks, n_data_blocks);
    EXPECT_EQ(r_data[0].MB.n_inode_blocks, n_inode_blocks);
    EXPECT_EQ(r_data[0].MB.fs_ver_major, fs_system_major);
    EXPECT_EQ(r_data[0].MB.fs_ver_minor, fs_system_minor);

    for (auto i = 0; i < n_inode_blocks; i++) {
        status =
            disk->read(fs_offset_inode_block + i, r_data->raw_data, block_size);
        EXPECT_EQ(status, block_size);

        for (auto j = 0; j < block_size / meta_fragm_size; j++) {
            EXPECT_STREQ(r_data[j].inode.file_name, inode_def_file_name);
            for (auto ptr_idx = 0; ptr_idx < meta_inode_ptr_len; ptr_idx++) {
                EXPECT_EQ(r_data[j].inode.block_ptr[ptr_idx], fs_nullptr);
            }

            EXPECT_EQ(r_data[j].inode.type, block_status::Free);
            EXPECT_EQ(r_data[j].inode.indirect_inode_ptr, fs_nullptr);
        }
    }
    disk->unmount();
}

// TEST_F(FileSystemTest, create_test) {
//     fs->mount();
//     inode_block inode = {};
//     const char* file1_name = "dummy1_file_name.txt";
//     const char* file2_name = "dummy2_file_name.txt";

//     auto file1_addr = fs->create(file1_name);
//     EXPECT_EQ(file1_addr, 0);
//     EXPECT_TRUE(get_inode_status(file1_addr));
//     dbg_io->get_inode(file1_addr, inode);
//     for (auto& ptr : inode.block_ptr) {
//         EXPECT_EQ(ptr, fs_nullptr);
//     }
//     EXPECT_EQ(inode.type, block_status::Used);
//     EXPECT_EQ(inode.indirect_inode_ptr, fs_nullptr);
//     EXPECT_EQ(inode.file_len, 0);
//     EXPECT_STREQ(inode.file_name, file1_name);

//     auto file2_addr = fs->create(file2_name);
//     EXPECT_EQ(file2_addr, 1);
//     EXPECT_TRUE(get_inode_status(file2_addr));
//     dbg_io->get_inode(file2_addr, inode);
//     for (auto& ptr : inode.block_ptr) {
//         EXPECT_EQ(ptr, fs_nullptr);
//     }
//     EXPECT_EQ(inode.type, block_status::Used);
//     EXPECT_EQ(inode.indirect_inode_ptr, fs_nullptr);
//     EXPECT_EQ(inode.file_len, 0);
//     EXPECT_STREQ(inode.file_name, file2_name);

//     fs->unmount();
// }

// TEST_F(FileSystemTest, remove_throw_test) {
//     EXPECT_THROW(fs->remove(0), std::runtime_error);
//     fs->mount();
//     EXPECT_THROW(fs->remove(0), std::runtime_error);
//     fs->unmount();
// }

// TEST_F(FileSystemTest, remove_test) {
//     fs->mount();

//     auto file1_addr = fs->create("dummy1_file.txt");
//     auto file2_addr = fs->create("dummy2_file.txt");
//     EXPECT_TRUE(get_inode_status(file1_addr));
//     EXPECT_TRUE(get_inode_status(file2_addr));

//     fs->remove(file1_addr);
//     EXPECT_FALSE(get_inode_status(file1_addr));
//     EXPECT_TRUE(get_inode_status(file2_addr));

//     fs->remove(file2_addr);
//     EXPECT_FALSE(get_inode_status(file2_addr));

//     fs->unmount();
// }

// TEST_F(FileSystemTest, write_test_throw) {
//     std::array<data, block_size> wdata;
//     EXPECT_THROW(fs->write(0, *wdata.data(), wdata.size(), 0),
//                  std::runtime_error);
//     fs->mount();

//     EXPECT_THROW(fs->write(0, *wdata.data(), wdata.size(), 0),
//                  std::runtime_error);

//     address file_addr = fs->create("dummy_file.bin");
//     EXPECT_THROW(fs->write(file_addr, *wdata.data(), wdata.size(), -1),
//                  std::invalid_argument);
//     EXPECT_THROW(fs->write(file_addr, *wdata.data(), -1, 0),
//                  std::invalid_argument);
//     EXPECT_THROW(fs->write(-1, *wdata.data(), -1, 0), std::invalid_argument);
//     EXPECT_THROW(
//         fs->write(file_addr, *wdata.data(), wdata.size(), wdata.size() + 1),
//         std::runtime_error);
//     fs->unmount();
// }

// TEST_F(FileSystemTest, write_one_block_test) {
//     fs->mount();
//     inode_block inode = {};
//     std::array<data, block_size> wdata;
//     std::memcpy(wdata.data(), (void*)memcpy, wdata.size());

//     address file_addr = fs->create("dummy_file.bin");
//     auto n_written = fs->write(file_addr, *wdata.data(), wdata.size(), 0);
//     EXPECT_EQ(n_written, wdata.size());

//     EXPECT_TRUE(get_inode_status(file_addr));
//     dbg_io->get_inode(file_addr, inode);

//     EXPECT_EQ(inode.file_len, wdata.size());
//     std::array<data, wdata.size()> rdata;
//     dbg_io->get_data_block(inode.block_ptr[0], *rdata.data());
//     for (size_t i = 0; i < wdata.size(); i++) {
//         EXPECT_EQ(rdata[i], wdata[i]);
//     }

//     fs->unmount();
// }

// TEST_F(FileSystemTest, write_two_block_test) {
//     fs->mount();
//     inode_block inode = {};
//     std::array<data, block_size * 2> wdata;
//     std::memcpy(wdata.data(), (void*)memcpy, wdata.size());

//     address file_addr = fs->create("dummy_file.bin");
//     auto n_written = fs->write(file_addr, *wdata.data(), wdata.size(), 0);
//     ASSERT_EQ(n_written, wdata.size());

//     EXPECT_TRUE(get_inode_status(file_addr));
//     dbg_io->get_inode(file_addr, inode);

//     EXPECT_EQ(inode.file_len, wdata.size());
//     std::array<data, wdata.size()> rdata;
//     dbg_io->get_data_block(inode.block_ptr[0], *rdata.data());
//     for (size_t i = 0; i < wdata.size(); i++) {
//         EXPECT_EQ(rdata[i], wdata[i]);
//     }

//     fs->unmount();
// }

// TEST_F(FileSystemTest, read_throw_test) {
//     std::array<data, block_size> rdata;
//     EXPECT_THROW(fs->read(0, *rdata.data(), rdata.size(), 0),
//                  std::runtime_error);
//     fs->mount();

//     EXPECT_THROW(fs->read(0, *rdata.data(), rdata.size(), 0),
//                  std::runtime_error);

//     address file_addr = fs->create("dummy_file.bin");
//     EXPECT_THROW(fs->read(file_addr, *rdata.data(), rdata.size(), -1),
//                  std::invalid_argument);
//     EXPECT_THROW(fs->read(file_addr, *rdata.data(), -1, 0),
//                  std::invalid_argument);
//     EXPECT_THROW(fs->read(-1, *rdata.data(), -1, 0), std::invalid_argument);
//     EXPECT_THROW(
//         fs->read(file_addr, *rdata.data(), rdata.size(), rdata.size() + 1),
//         std::runtime_error);
//     fs->unmount();
// }

// TEST_F(FileSystemTest, read_one_block_test) {
//     inode_block inode = {};
//     inode.file_len = block_size;
//     inode.type = block_status::Used;
//     inode.block_ptr[0] = dbg_io->get_data_bitmap().next_free(0);
//     inode.indirect_inode_ptr = fs_nullptr;  // TODO: Fix this!
//     dbg_io->set_inode(0, inode);

//     std::array<data, block_size> ref_data;
//     std::memcpy(ref_data.data(), (void*)memcpy, ref_data.size());
//     dbg_io->set_data_block(inode.block_ptr[0], *ref_data.data());

//     fs->mount();
//     std::array<data, ref_data.size()> rdata;
//     fs->read(0, *rdata.data(), rdata.size(), rdata.size());

//     for (size_t i = 0; i < rdata.size(); i++) {
//         EXPECT_EQ(rdata[i], ref_data[i]);
//     }

//     fs->unmount();
// }
}