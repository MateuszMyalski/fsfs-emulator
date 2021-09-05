#include "fsfs/block_bitmap.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class BlockBitmapTest : public ::testing::TestWithParam<fsize>, public TestBaseBasic {
   protected:
    constexpr static auto bitmap_row_length = std::numeric_limits<bitmap_t>::digits;

    BlockBitmap* bitmap;

   public:
    void SetUp() override { bitmap = new BlockBitmap(); }

    void TearDown() override { delete bitmap; }
};

TEST_P(BlockBitmapTest, get_block_status_throw_bitmap_not_initialized) {
    EXPECT_THROW(bitmap->get_status(n_blocks), std::runtime_error);
}

TEST_P(BlockBitmapTest, get_block_status_throw_overflow_bitmap_size) {
    bitmap->resize(n_blocks);
    EXPECT_THROW(bitmap->get_status(n_blocks), std::invalid_argument);
}

TEST_P(BlockBitmapTest, get_block_status_throw_negative_value) {
    bitmap->resize(n_blocks);
    EXPECT_THROW(bitmap->get_status(-1), std::invalid_argument);
}

TEST_P(BlockBitmapTest, get_block_status_valid_value) {
    bitmap->resize(n_blocks);
    EXPECT_THROW(bitmap->get_status(-1), std::invalid_argument);
}

TEST_P(BlockBitmapTest, get_data_block_throw) {
    bitmap->resize(n_blocks);
    EXPECT_NO_THROW(bitmap->get_status(n_blocks - 1));
}

TEST_P(BlockBitmapTest, set_and_get_multiple_set) {
    bitmap->resize(n_blocks);

    ASSERT_FALSE(bitmap->get_status(0));
    ASSERT_FALSE(bitmap->get_status(bitmap_row_length));
    EXPECT_FALSE(bitmap->get_status(bitmap_row_length + 1));
    ASSERT_FALSE(bitmap->get_status(bitmap_row_length - 1));

    bitmap->set_status(bitmap_row_length, 1);

    EXPECT_FALSE(bitmap->get_status(0));
    EXPECT_TRUE(bitmap->get_status(bitmap_row_length));
    EXPECT_FALSE(bitmap->get_status(bitmap_row_length + 1));
    EXPECT_FALSE(bitmap->get_status(bitmap_row_length - 1));

    bitmap->set_status(bitmap_row_length, 1);

    EXPECT_FALSE(bitmap->get_status(0));
    EXPECT_TRUE(bitmap->get_status(bitmap_row_length));
    EXPECT_FALSE(bitmap->get_status(bitmap_row_length + 1));
    EXPECT_FALSE(bitmap->get_status(bitmap_row_length - 1));
}

TEST_P(BlockBitmapTest, set_and_get_block) {
    bitmap->resize(n_blocks);

    bitmap->set_status(block_size - 1, 1);
    EXPECT_TRUE(bitmap->get_status(block_size - 1));

    bitmap->set_status(block_size + 1, 1);
    EXPECT_TRUE(bitmap->get_status(block_size - 1));
    EXPECT_TRUE(bitmap->get_status(block_size + 1));
    EXPECT_FALSE(bitmap->get_status(block_size + 2));

    bitmap->set_status(block_size + 1, 0);
    EXPECT_TRUE(bitmap->get_status(block_size - 1));
    EXPECT_FALSE(bitmap->get_status(block_size + 1));
    EXPECT_FALSE(bitmap->get_status(block_size + 2));
}

TEST_P(BlockBitmapTest, next_free_throw_uninitialized) {
    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
    EXPECT_THROW(bitmap->next_free(0), std::runtime_error);
}

TEST_P(BlockBitmapTest, next_free_throw_invalid_argument) {
    bitmap->resize(n_blocks);

    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
}

TEST_P(BlockBitmapTest, next_free_throw_overflow_bitmap_size) {
    bitmap->resize(n_blocks);
    EXPECT_THROW(bitmap->next_free(n_blocks), std::runtime_error);
    EXPECT_THROW(bitmap->next_free(n_blocks + 1), std::runtime_error);
}

TEST_P(BlockBitmapTest, next_free_basic_set) {
    bitmap->resize(n_blocks);

    EXPECT_EQ(bitmap->next_free(0), 0);
    EXPECT_EQ(bitmap->next_free(2), 2);

    bitmap->set_status(0, 1);
    bitmap->set_status(1, 1);
    bitmap->set_status(2, 1);

    EXPECT_EQ(bitmap->next_free(0), 3);
}

TEST_P(BlockBitmapTest, next_free_in_next_row) {
    bitmap->resize(n_blocks);

    for (auto i = 0; i < bitmap_row_length; i++) {
        bitmap->set_status(i, 1);
    }

    bitmap->set_status(bitmap_row_length - 5, 0);
    EXPECT_EQ(bitmap->next_free(0), bitmap_row_length - 5);
}

TEST_P(BlockBitmapTest, next_free_no_free) {
    bitmap->resize(n_blocks);

    for (auto i = 0; i < n_blocks; i++) {
        bitmap->set_status(i, 1);
    }

    EXPECT_EQ(bitmap->next_free(0), -1);

    bitmap->set_status(n_blocks - 1, 0);
    EXPECT_EQ(bitmap->next_free(0), n_blocks - 1);
}

TEST_P(BlockBitmapTest, next_free_not_even_bitmap_no_free) {
    bitmap->resize(n_blocks - 1);

    for (auto i = 0; i < n_blocks - 1; i++) {
        bitmap->set_status(i, 1);
    }

    EXPECT_EQ(bitmap->next_free(0), -1);

    bitmap->set_status(n_blocks - 2, 0);
    EXPECT_EQ(bitmap->next_free(0), n_blocks - 2);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, BlockBitmapTest, testing::ValuesIn(valid_block_sizes));

}