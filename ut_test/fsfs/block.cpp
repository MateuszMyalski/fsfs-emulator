#include "fsfs/block.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class BlockTest : public ::testing::TestWithParam<int32_t>, public TestBaseFileSystem {
   protected:
    std::unique_ptr<Block> block;
    int32_t data_n = 0;
    int32_t block_n = fs_offset_inode_block + MB.n_inode_blocks + data_n;
    DataBufferType ref_data;
    DataBufferType rdata;

   public:
    void SetUp() override {
        block = std::make_unique<Block>(disk, MB);
        ref_data.resize(block_size);
        rdata.resize(block_size);
        fill_dummy(ref_data);
    }
};

TEST_P(BlockTest, data_n_to_block_n_throw_invalid_block) {
    EXPECT_THROW(block->data_n_to_block_n(MB.n_blocks), std::invalid_argument);
    EXPECT_THROW(block->data_n_to_block_n(-1), std::invalid_argument);
}

TEST_P(BlockTest, inode_n_to_block_n_throw_invalid_block) {
    EXPECT_THROW(block->inode_n_to_block_n(MB.n_inode_blocks), std::invalid_argument);
    EXPECT_THROW(block->inode_n_to_block_n(-1), std::invalid_argument);
}

TEST_P(BlockTest, inode_n_to_block_n) {
    int32_t n_meta_blocks_in_block = block_size / meta_fragm_size_bytes;

    EXPECT_EQ(block->inode_n_to_block_n(0), fs_offset_inode_block);
    EXPECT_EQ(block->inode_n_to_block_n(1), fs_offset_inode_block);
    EXPECT_EQ(block->inode_n_to_block_n(n_meta_blocks_in_block - 1), fs_offset_inode_block);
    EXPECT_EQ(block->inode_n_to_block_n(n_meta_blocks_in_block), fs_offset_inode_block + 1);

    EXPECT_EQ(block->inode_n_to_block_n(MB.n_inode_blocks - 1),
              (MB.n_inode_blocks - 1) / n_meta_blocks_in_block + fs_offset_inode_block);
}

TEST_P(BlockTest, data_n_to_block_n) {
    EXPECT_EQ(block->data_n_to_block_n(0), fs_offset_inode_block + MB.n_inode_blocks);
    EXPECT_EQ(block->data_n_to_block_n(MB.n_inode_blocks - 1),
              (MB.n_inode_blocks - 1) + MB.n_inode_blocks + fs_offset_inode_block);
}

TEST_P(BlockTest, write_throw_invalid_block_number) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->write(MB.n_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(block->write(-1, &dummy_data, 0, 0), std::invalid_argument);
}

TEST_P(BlockTest, write_throw_invalid_offset) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->write(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(block->write(0, &dummy_data, -block_size, 0), std::invalid_argument);
}

TEST_P(BlockTest, write_throw_invalid_length) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->write(0, &dummy_data, 0, -1), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_block_number) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->read(MB.n_blocks, &dummy_data, 0, 0), std::invalid_argument);
    EXPECT_THROW(block->read(-1, &dummy_data, 0, 0), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_block_offset) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->read(0, &dummy_data, block_size, 0), std::invalid_argument);
    EXPECT_THROW(block->read(0, &dummy_data, -block_size, 0), std::invalid_argument);
}

TEST_P(BlockTest, read_throw_invalid_length) {
    uint8_t dummy_data = 0x00;
    EXPECT_THROW(block->read(0, &dummy_data, 0, -1), std::invalid_argument);
    EXPECT_THROW(block->read(0, &dummy_data, 0, block_size + 1), std::invalid_argument);
    EXPECT_THROW(block->read(0, &dummy_data, -2, 3), std::invalid_argument);
    EXPECT_THROW(block->read(0, &dummy_data, block_size - 1, 2), std::invalid_argument);
}

TEST_P(BlockTest, write_basic_write) {
    auto n_written = block->write(block_n, ref_data.data(), 0, ref_data.size());
    ASSERT_EQ(n_written, ref_data.size());

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_TRUE(cmp_data(ref_data.data(), rdata.data(), n_written));
}

TEST_P(BlockTest, write_with_offset_and_overflow) {
    constexpr int32_t overflow_length = 1;
    auto n_written = block->write(block_n, ref_data.data(), block_size / 2, ref_data.size() + overflow_length);
    ASSERT_EQ(n_written, block_size - block_size / 2);

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_TRUE(cmp_data(ref_data.data(), &rdata[n_written], n_written));
}

TEST_P(BlockTest, write_with_end_offset) {
    constexpr const uint8_t data_fragm[] = "abcdef";
    auto n_written = block->write(block_n, ref_data.data(), 0, ref_data.size());
    n_written = block->write(block_n, data_fragm, -3, 2);
    ASSERT_EQ(n_written, 2);

    auto n_read = disk.read(block_n, rdata.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    EXPECT_EQ(rdata[block_size - 3], data_fragm[0]);
    EXPECT_EQ(rdata[block_size - 2], data_fragm[1]);

    EXPECT_TRUE(cmp_data(ref_data.data(), rdata.data(), ref_data.size() - 3));
    EXPECT_EQ(rdata[block_size - 1], ref_data.back());
}

TEST_P(BlockTest, read_basic_read) {
    auto n_write = disk.write(block_n, ref_data.data(), ref_data.size());
    ASSERT_EQ(n_write, ref_data.size());

    auto n_read = block->read(block_n, rdata.data(), 0, block_size / 2);
    ASSERT_EQ(n_read, block_size / 2);

    EXPECT_TRUE(cmp_data(ref_data.data(), rdata.data(), n_read));
}

TEST_P(BlockTest, read_with_offset) {
    auto n_written = block->write(block_n, ref_data.data(), 0, ref_data.size());
    ASSERT_EQ(n_written, ref_data.size());

    auto n_read = block->read(block_n, rdata.data(), -3, 2);
    ASSERT_EQ(n_read, 2);

    EXPECT_EQ(rdata[0], ref_data[ref_data.size() - 3]);
    EXPECT_EQ(rdata[1], ref_data[ref_data.size() - 2]);
}

TEST_P(BlockTest, bytes_to_block) {
    EXPECT_EQ(block->bytes_to_blocks(0), 0);
    EXPECT_EQ(block->bytes_to_blocks(-1), 0);
    EXPECT_EQ(block->bytes_to_blocks(MB.block_size / 2), 1);
    EXPECT_EQ(block->bytes_to_blocks(MB.block_size), 1);
    EXPECT_EQ(block->bytes_to_blocks(MB.block_size + 1), 2);
    EXPECT_EQ(block->bytes_to_blocks(MB.block_size + MB.block_size / 2), 2);
    EXPECT_EQ(block->bytes_to_blocks(2 * MB.block_size), 2);
    EXPECT_EQ(block->bytes_to_blocks(2 * MB.block_size + 1), 3);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, BlockTest, testing::ValuesIn(valid_block_sizes));
}