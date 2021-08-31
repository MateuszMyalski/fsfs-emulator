#include "fsfs/block.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class BlockTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    Block* data_block;
    address data_n = 0;
    address block_n = fs_offset_inode_block + MB.n_inode_blocks + data_n;
    DataBufferType wdata;
    DataBufferType rdata;

   public:
    void SetUp() override {
        data_block = new Block(disk, MB);
        wdata.resize(block_size);
        rdata.resize(block_size);
        fill_dummy(wdata);
    }
    void TearDown() override { delete data_block; }
};

TEST_P(BlockTest, data_n_to_block_n_throw_invalid_block) {
    EXPECT_THROW(data_block->data_n_to_block_n(MB.n_data_blocks), std::invalid_argument);
    EXPECT_THROW(data_block->data_n_to_block_n(-1), std::invalid_argument);
}

TEST_P(BlockTest, inode_n_to_block_n_throw_invalid_block) {
    EXPECT_THROW(data_block->inode_n_to_block_n(MB.n_inode_blocks), std::invalid_argument);
    EXPECT_THROW(data_block->inode_n_to_block_n(-1), std::invalid_argument);
}

TEST_P(BlockTest, inode_n_to_block_n) {
    fsize n_meta_blocks_in_block = block_size / meta_fragm_size;

    EXPECT_EQ(data_block->inode_n_to_block_n(0), fs_offset_inode_block);
    EXPECT_EQ(data_block->inode_n_to_block_n(1), fs_offset_inode_block);
    EXPECT_EQ(data_block->inode_n_to_block_n(n_meta_blocks_in_block - 1), fs_offset_inode_block);
    EXPECT_EQ(data_block->inode_n_to_block_n(n_meta_blocks_in_block), fs_offset_inode_block + 1);

    EXPECT_EQ(data_block->inode_n_to_block_n(MB.n_inode_blocks - 1),
              (MB.n_inode_blocks - 1) / n_meta_blocks_in_block + fs_offset_inode_block);
}

TEST_P(BlockTest, data_n_to_block_n) {
    EXPECT_EQ(data_block->data_n_to_block_n(0), fs_offset_inode_block + MB.n_inode_blocks);
    EXPECT_EQ(data_block->data_n_to_block_n(MB.n_inode_blocks - 1),
              (MB.n_inode_blocks - 1) + MB.n_inode_blocks + fs_offset_inode_block);
}

TEST_P(BlockTest, write_throw_invalid_block_number) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->write(MB.n_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(data_block->write(-1, &dummy_data, 0, 0), std::invalid_argument);
}

TEST_P(BlockTest, write_throw_invalid_offset) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->write(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(data_block->write(0, &dummy_data, -block_size, 0), std::invalid_argument);
}

TEST_P(BlockTest, write_throw_invalid_length) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->write(0, &dummy_data, 0, -1), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_block_number) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->read(MB.n_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(data_block->read(-1, &dummy_data, 0, 0), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_block_offset) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->read(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -block_size, 0), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_length) {
    data dummy_data = 0x00;
    EXPECT_THROW(data_block->read(0, &dummy_data, 0, -1), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, 0, block_size + 1), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, -2, 3), std::invalid_argument);
    EXPECT_THROW(data_block->read(0, &dummy_data, block_size - 1, 2), std::invalid_argument);
}

TEST_P(BlockTest, write_basic_write) {
    auto n_written = data_block->write(block_n, wdata.data(), 0, wdata.size());
    ASSERT_EQ(n_written, wdata.size());

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_TRUE(cmp_data(wdata.data(), rdata.data(), n_written));
}

TEST_P(BlockTest, write_with_offset_and_overflow) {
    constexpr fsize overflow_length = 1;
    auto n_written = data_block->write(block_n, wdata.data(), block_size / 2, wdata.size() + overflow_length);
    ASSERT_EQ(n_written, block_size - block_size / 2);

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_TRUE(cmp_data(wdata.data(), &rdata[n_written], n_written));
}

TEST_P(BlockTest, write_with_end_offset) {
    data data_fragm[] = "abcdef";
    auto n_written = data_block->write(block_n, wdata.data(), 0, wdata.size());
    n_written = data_block->write(block_n, data_fragm, -3, 2);
    ASSERT_EQ(n_written, 2);

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_EQ(rdata[block_size - 3], data_fragm[0]);
    EXPECT_EQ(rdata[block_size - 2], data_fragm[1]);

    EXPECT_TRUE(cmp_data(wdata.data(), rdata.data(), wdata.size() - 3));
    EXPECT_EQ(rdata[block_size - 1], wdata.back());
}

TEST_P(BlockTest, read_basic_read) {
    auto n_write = disk.write(block_n, wdata.data(), wdata.size());
    ASSERT_EQ(n_write, wdata.size());

    auto n_read = data_block->read(block_n, rdata.data(), 0, block_size / 2);
    ASSERT_EQ(n_read, block_size / 2);

    EXPECT_TRUE(cmp_data(wdata.data(), rdata.data(), n_read));
}

TEST_P(BlockTest, read_with_offset) {
    auto n_written = data_block->write(block_n, wdata.data(), 0, wdata.size());
    ASSERT_EQ(n_written, wdata.size());

    auto n_read = data_block->read(block_n, rdata.data(), -3, 2);
    ASSERT_EQ(n_read, 2);

    EXPECT_EQ(rdata[0], wdata[wdata.size() - 3]);
    EXPECT_EQ(rdata[1], wdata[wdata.size() - 2]);
}

TEST_P(BlockTest, bytes_to_block) {
    EXPECT_EQ(data_block->bytes_to_blocks(0), 0);
    EXPECT_EQ(data_block->bytes_to_blocks(-1), 0);
    EXPECT_EQ(data_block->bytes_to_blocks(MB.block_size / 2), 1);
    EXPECT_EQ(data_block->bytes_to_blocks(MB.block_size), 1);
    EXPECT_EQ(data_block->bytes_to_blocks(MB.block_size + 1), 2);
    EXPECT_EQ(data_block->bytes_to_blocks(MB.block_size + MB.block_size / 2), 2);
    EXPECT_EQ(data_block->bytes_to_blocks(2 * MB.block_size), 2);
    EXPECT_EQ(data_block->bytes_to_blocks(2 * MB.block_size + 1), 3);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, BlockTest, testing::ValuesIn(valid_block_sizes));
}