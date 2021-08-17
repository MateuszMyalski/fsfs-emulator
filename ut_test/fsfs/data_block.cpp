#include "fsfs/data_block.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class DataBlockTest : public ::testing::Test, public TestBaseFileSystem {
   protected:
    DataBlock* data_block;

   public:
    void SetUp() override { data_block = new DataBlock(disk, MB); }
    void TearDown() override { delete data_block; }
};

TEST_F(DataBlockTest, write_throw) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->write(MB.n_data_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(data_block->write(-1, &dummy_data, 0, 0), std::invalid_argument);

    EXPECT_THROW(data_block->write(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, -block_size, 0), std::invalid_argument);

    EXPECT_THROW(data_block->write(0, &dummy_data, 0, -1), std::invalid_argument);
}

TEST_F(DataBlockTest, read_throw) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->read(MB.n_data_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(data_block->read(-1, &dummy_data, 0, 0), std::invalid_argument);

    EXPECT_THROW(data_block->read(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -block_size, 0), std::invalid_argument);

    EXPECT_THROW(data_block->read(0, &dummy_data, 0, -1), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, 0, block_size + 1), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -2, 3), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, block_size - 1, 2), std::invalid_argument);
}

TEST_F(DataBlockTest, write) {
    std::vector<data> wdata(block_size / 2);
    std::vector<data> rdata(block_size);
    fill_dummy(wdata);

    auto n_written = data_block->write(0, wdata.data(), 0, wdata.size());
    EXPECT_EQ(n_written, wdata.size());

    disk.read(data_block_to_addr(0), rdata.data(), block_size);

    for (size_t i = 0; i < wdata.size(); i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }

    constexpr fsize overflow_test = 1;
    n_written = data_block->write(0, wdata.data(), block_size / 2, wdata.size() + overflow_test);
    EXPECT_EQ(n_written, wdata.size());

    disk.read(data_block_to_addr(0), rdata.data(), block_size);

    for (size_t i = 0; i < wdata.size(); i++) {
        EXPECT_EQ(wdata[i], rdata[i + wdata.size()]);
    }

    data data_fragm[] = "abc";
    n_written = data_block->write(0, data_fragm, -3, 2);
    EXPECT_EQ(n_written, 2);

    disk.read(data_block_to_addr(0), rdata.data(), block_size);

    EXPECT_EQ(rdata[block_size - 3], data_fragm[0]);
    EXPECT_EQ(rdata[block_size - 2], data_fragm[1]);
    for (size_t i = 0; i < wdata.size() - 3; i++) {
        EXPECT_EQ(wdata[i], rdata[i + wdata.size()]);
    }
    EXPECT_EQ(rdata[block_size - 1], wdata.back());
}

TEST_F(DataBlockTest, read) {
    std::vector<data> wdata(block_size);
    std::vector<data> rdata(block_size);
    fill_dummy(wdata);
    std::memset(rdata.data(), 0x0, rdata.size());
    disk.write(data_block_to_addr(0), wdata.data(), wdata.size());

    auto n_read = data_block->read(0, rdata.data(), 0, block_size / 2);
    EXPECT_EQ(n_read, block_size / 2);
    for (fsize i = 0; i < block_size / 2; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
    for (size_t i = block_size / 2; i < rdata.size(); i++) {
        EXPECT_EQ(0x00, rdata[i]);
    }

    n_read = data_block->read(0, rdata.data(), -3, 2);
    EXPECT_EQ(n_read, 2);
    EXPECT_EQ(rdata[0], wdata[block_size - 3]);
    EXPECT_EQ(rdata[1], wdata[block_size - 2]);
    for (fsize i = 2; i < block_size / 2; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
}
}