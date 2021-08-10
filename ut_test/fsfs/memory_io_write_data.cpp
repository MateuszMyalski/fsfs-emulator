#include "disk-emulator/disk.hpp"
#include "fsfs/data_block.hpp"
#include "fsfs/file_system.hpp"
#include "fsfs/indirect_inode.hpp"
#include "fsfs/inode.hpp"
#include "fsfs/memory_io.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
using data_buf = std::vector<data>;
class MemoryIOWriteDataTest : public ::testing::Test {
   protected:
    constexpr static const char* file_name = "SampleFile";
    constexpr static fsize block_size = 1024;
    fsize n_ptr_in_data_block = block_size / sizeof(address);

    MemoryIO* io;
    Disk* disk;
    super_block MB;
    Inode* inode;
    IndirectInode* iinode;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        inode = new Inode(*disk, MB);
        iinode = new IndirectInode(*disk, MB);

        io = new MemoryIO(*disk);

        io->init(MB);
        io->scan_blocks();
    }

    void TearDown() override {
        delete inode;
        delete iinode;

        delete io;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }

    void fill_dummy(data_buf& data) {
        std::memcpy(data.data(), (void*)memcpy, data.size());
    }

    bool check_block(address block_n, const data_buf& ref_data,
                     data_buf::iterator& ref_data_it) {
        DataBlock data_block(*disk, MB);
        data_buf rdata(MB.block_size);
        data_block.read(block_n, rdata.data(), 0, MB.block_size);

        auto rdata_it = rdata.begin();
        while ((rdata_it != rdata.end()) && (ref_data_it != ref_data.end())) {
            if (*rdata_it != *ref_data_it) {
                return false;
            }
            rdata_it++;
            ref_data_it++;
        }
        return true;
    }
};

TEST_F(MemoryIOWriteDataTest, sanity_check) {
    address invalid_inode = MB.n_inode_blocks - 1;
    data_buf wdata(1);

    EXPECT_EQ(io->write_data(invalid_inode, wdata.data(), 0, 1), fs_nullptr);

    address addr = io->alloc_inode(file_name);

    EXPECT_EQ(io->write_data(addr, wdata.data(), 0, 0), 0);
}

TEST_F(MemoryIOWriteDataTest, two_blocks) {
    fsize data_len = MB.block_size * 2;
    data_buf wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    ASSERT_NE(inode->get(addr).block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[1], fs_nullptr);

    auto wdata_it = wdata.begin();
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[1], wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, uneven_direct_blocks) {
    fsize data_len = MB.block_size * 2 + 1;
    data_buf wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    ASSERT_NE(inode->get(addr).block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[1], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[2], fs_nullptr);

    auto wdata_it = wdata.begin();
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[1], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[2], wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, single_indirect_blocks) {
    fsize data_len = MB.block_size * meta_inode_ptr_len + 1;
    data_buf wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        ASSERT_NE(inode->get(addr).block_ptr[i], fs_nullptr);
    }
    ASSERT_NE(inode->get(addr).indirect_inode_ptr, fs_nullptr);
    auto indirect_n0 = iinode->get_ptr(inode->get(addr).indirect_inode_ptr, 0);
    ASSERT_NE(indirect_n0, fs_nullptr);

    auto wdata_it = wdata.begin();
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_TRUE(
            check_block(inode->get(addr).block_ptr[i], wdata, wdata_it));
    }
    EXPECT_TRUE(check_block(indirect_n0, wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, nested_indirect_blocks) {}

TEST_F(MemoryIOWriteDataTest, offset_write) {}

// TEST_F(MemoryIOWriteDataTest, ----) {
//     inode->reinit();
//     EXPECT_EQ(inode->get(addr).file_len, data_len);

//     data_buf rdata(MB.block_size);
//     data_block.read(inode->get(addr).block_ptr[0], rdata.data(), 0,
//                     MB.block_size);
//     for (auto i = 0; i < MB.block_size; i++) {
//         EXPECT_EQ(wdata[i], rdata[i]);
//     }
//     data_block.read(inode->get(addr).block_ptr[1], rdata.data(), 0,
//                     MB.block_size);
//     for (auto i = 0; i < MB.block_size; i++) {
//         EXPECT_EQ(wdata[i + MB.block_size], rdata[i]);
//     }
//     data_block.read(inode->get(addr).block_ptr[2], rdata.data(), 0, 1);
//     EXPECT_EQ(wdata[data_len - 1], rdata[0]);

//     n_written = io->write_data(addr, wdata.data(), 0, MB.block_size);
//     EXPECT_EQ(n_written, MB.block_size);

//     inode->reinit();
//     EXPECT_EQ(inode->get(addr).file_len, data_len + MB.block_size);

//     data_block.reinit();
//     data_block.read(inode->get(addr).block_ptr[2], rdata.data(), 1,
//                     MB.block_size - 1);
//     for (auto i = 0; i < MB.block_size - 1; i++) {
//         EXPECT_EQ(wdata[i], rdata[i]);
//     }
//     data_block.read(inode->get(addr).block_ptr[3], rdata.data(), 0, 1);
//     EXPECT_EQ(wdata[MB.block_size - 1], rdata[0]);

//     n_written = io->write_data(addr, wdata.data(), 0, 2 * MB.block_size);
//     n_written += io->write_data(addr, wdata.data(), 0, 2 * MB.block_size);
//     EXPECT_EQ(n_written, 4 * MB.block_size);

//     data_block.reinit();
//     data_block.read(inode->get(addr).block_ptr[2], rdata.data(), 0,
//                     MB.block_size - 1);
// }
}
