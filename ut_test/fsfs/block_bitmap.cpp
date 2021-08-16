#include "fsfs/block_bitmap.hpp"

#include <exception>
#include <vector>

#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class BlockBitmapTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    constexpr static fsize data_n_blocks = 2048;
    constexpr static auto bitmap_row_length = std::numeric_limits<bitmap_t>::digits;

    BlockBitmap* bitmap;

   public:
    void SetUp() override { bitmap = new BlockBitmap(); }

    void TearDown() override { delete bitmap; }
};

TEST_F(BlockBitmapTest, get_data_block_throw) {
    EXPECT_THROW(bitmap->get_block_status(data_n_blocks), std::runtime_error);

    bitmap->init(data_n_blocks);

    EXPECT_THROW(bitmap->get_block_status(data_n_blocks), std::invalid_argument);
    EXPECT_THROW(bitmap->get_block_status(-1), std::invalid_argument);

    bitmap->get_block_status(data_n_blocks - 1);
}

TEST_F(BlockBitmapTest, set_and_get_map_line_border) {
    bitmap->init(data_n_blocks);

    EXPECT_EQ(bitmap->get_block_status(0), 0);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length - 1), 0);

    bitmap->set_block(bitmap_row_length, 1);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length), 1);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length + 1), 0);

    EXPECT_EQ(bitmap->get_block_status(0), 0);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length - 1), 0);

    bitmap->set_block(bitmap_row_length, 1);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length), 1);

    EXPECT_EQ(bitmap->get_block_status(bitmap_row_length + 1), 0);
}

TEST_F(BlockBitmapTest, set_and_get_block) {
    bitmap->init(data_n_blocks);

    bitmap->set_block(block_size - 1, 1);
    EXPECT_EQ(bitmap->get_block_status(block_size - 1), 1);

    bitmap->set_block(block_size + 1, 1);
    EXPECT_EQ(bitmap->get_block_status(block_size + 1), 1);

    EXPECT_EQ(bitmap->get_block_status(block_size + 2), 0);

    bitmap->set_block(block_size + 1, 0);
    EXPECT_EQ(bitmap->get_block_status(block_size + 1), 0);
}

TEST_F(BlockBitmapTest, next_free_throw) {
    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
    EXPECT_THROW(bitmap->next_free(0), std::runtime_error);

    bitmap->init(data_n_blocks);

    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
    EXPECT_THROW(bitmap->next_free(data_n_blocks), std::runtime_error);
    EXPECT_THROW(bitmap->next_free(data_n_blocks + 1), std::runtime_error);
}

TEST_F(BlockBitmapTest, next_free) {
    bitmap->init(data_n_blocks);

    EXPECT_EQ(bitmap->next_free(0), 0);
    EXPECT_EQ(bitmap->next_free(2), 2);

    bitmap->set_block(0, 1);
    bitmap->set_block(1, 1);
    bitmap->set_block(2, 1);

    EXPECT_EQ(bitmap->next_free(0), 3);

    for (auto i = 0; i < bitmap_row_length; i++) {
        bitmap->set_block(i, 1);
    }

    bitmap->set_block(bitmap_row_length - 5, 0);
    EXPECT_EQ(bitmap->next_free(0), bitmap_row_length - 5);

    for (auto i = 0; i < data_n_blocks; i++) {
        bitmap->set_block(i, 1);
    }

    EXPECT_EQ(bitmap->next_free(0), -1);

    bitmap->set_block(data_n_blocks - 1, 0);
    EXPECT_EQ(bitmap->next_free(0), data_n_blocks - 1);
}
}