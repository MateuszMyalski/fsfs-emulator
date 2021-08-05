#include "fsfs/data_block.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class DataBlockTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    Disk* disk;
    super_block MB;
    DataBlock* data_block;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        data_block = new DataBlock(*disk, MB);
    }
    void TearDown() override {
        delete data_block;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }

    address data_block_to_addr(address data_block_n) {
        return fs_offset_inode_block + MB.n_inode_blocks + data_block_n;
    }
};

TEST_F(DataBlockTest, write_throw_test) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->write(MB.n_data_blocks, &dummy_data, 0, 0),
                 std::invalid_argument);
    EXPECT_THROW(data_block->write(-1, &dummy_data, 0, 0),
                 std::invalid_argument);

    EXPECT_THROW(data_block->write(0, &dummy_data, MB.block_size, 0),
                 std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, -MB.block_size, 0),
                 std::invalid_argument);

    EXPECT_THROW(data_block->write(0, &dummy_data, 0, -1),
                 std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, 0, MB.block_size + 1),
                 std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, -2, 3),
                 std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, MB.block_size - 1, 2),
                 std::invalid_argument);
}

TEST_F(DataBlockTest, read_throw_test) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->read(MB.n_data_blocks, &dummy_data, 0, 0),
                 std::invalid_argument);
    EXPECT_THROW(data_block->read(-1, &dummy_data, 0, 0),
                 std::invalid_argument);

    EXPECT_THROW(data_block->read(0, &dummy_data, MB.block_size, 0),
                 std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -MB.block_size, 0),
                 std::invalid_argument);

    EXPECT_THROW(data_block->read(0, &dummy_data, 0, -1),
                 std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, 0, MB.block_size + 1),
                 std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -2, 3),
                 std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, MB.block_size - 1, 2),
                 std::invalid_argument);
}

TEST_F(DataBlockTest, write_test) {
    std::vector<data> wdata(MB.block_size / 2);
    std::vector<data> rdata(MB.block_size);
    std::memcpy(wdata.data(), (void*)memcpy, wdata.size());

    auto n_written = data_block->write(0, wdata.data(), 0, wdata.size());
    EXPECT_EQ(n_written, wdata.size());

    disk->read(data_block_to_addr(0), rdata.data(), MB.block_size);

    for (size_t i = 0; i < wdata.size(); i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }

    n_written =
        data_block->write(0, wdata.data(), MB.block_size / 2, wdata.size());
    EXPECT_EQ(n_written, wdata.size());

    disk->read(data_block_to_addr(0), rdata.data(), MB.block_size);

    for (size_t i = 0; i < wdata.size(); i++) {
        EXPECT_EQ(wdata[i], rdata[i + wdata.size()]);
    }

    data data_fragm[] = "abc";
    n_written = data_block->write(0, data_fragm, -3, 2);
    EXPECT_EQ(n_written, 2);

    disk->read(data_block_to_addr(0), rdata.data(), MB.block_size);

    EXPECT_EQ(rdata[MB.block_size - 3], data_fragm[0]);
    EXPECT_EQ(rdata[MB.block_size - 2], data_fragm[1]);
    for (size_t i = 0; i < wdata.size() - 3; i++) {
        EXPECT_EQ(wdata[i], rdata[i + wdata.size()]);
    }
    EXPECT_EQ(rdata[MB.block_size - 1], wdata.back());
}

TEST_F(DataBlockTest, read_test) {
    std::vector<data> wdata(MB.block_size);
    std::vector<data> rdata(MB.block_size);
    std::memcpy(wdata.data(), (void*)memcpy, wdata.size());
    std::memset(rdata.data(), 0x0, rdata.size());
    disk->write(data_block_to_addr(0), wdata.data(), wdata.size());

    auto n_read = data_block->read(0, rdata.data(), 0, MB.block_size / 2);
    EXPECT_EQ(n_read, MB.block_size / 2);
    for (fsize i = 0; i < MB.block_size / 2; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
    for (size_t i = MB.block_size / 2; i < rdata.size(); i++) {
        EXPECT_EQ(0x00, rdata[i]);
    }

    n_read = data_block->read(0, rdata.data(), -3, 2);
    EXPECT_EQ(n_read, 2);
    EXPECT_EQ(rdata[0], wdata[MB.block_size - 3]);
    EXPECT_EQ(rdata[1], wdata[MB.block_size - 2]);
    for (fsize i = 2; i < MB.block_size / 2; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
}
}