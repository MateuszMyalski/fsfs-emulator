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

TEST_P(BlockBitmapTest, get_data_block_throw) {
    EXPECT_THROW(bitmap->get_block_status(n_blocks), std::runtime_error);

    bitmap->init(n_blocks);

    EXPECT_THROW(bitmap->get_block_status(n_blocks), std::invalid_argument);
    EXPECT_THROW(bitmap->get_block_status(-1), std::invalid_argument);

    bitmap->get_block_status(n_blocks - 1);
}

TEST_P(BlockBitmapTest, set_and_get_map_line_border) {
    bitmap->init(n_blocks);

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

TEST_P(BlockBitmapTest, set_and_get_block) {
    bitmap->init(n_blocks);

    bitmap->set_block(block_size - 1, 1);
    EXPECT_EQ(bitmap->get_block_status(block_size - 1), 1);

    bitmap->set_block(block_size + 1, 1);
    EXPECT_EQ(bitmap->get_block_status(block_size + 1), 1);

    EXPECT_EQ(bitmap->get_block_status(block_size + 2), 0);

    bitmap->set_block(block_size + 1, 0);
    EXPECT_EQ(bitmap->get_block_status(block_size + 1), 0);
}

TEST_P(BlockBitmapTest, next_free_throw) {
    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
    EXPECT_THROW(bitmap->next_free(0), std::runtime_error);

    bitmap->init(n_blocks);

    EXPECT_THROW(bitmap->next_free(-1), std::invalid_argument);
    EXPECT_THROW(bitmap->next_free(n_blocks), std::runtime_error);
    EXPECT_THROW(bitmap->next_free(n_blocks + 1), std::runtime_error);
}

TEST_P(BlockBitmapTest, next_free) {
    bitmap->init(n_blocks);

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

    for (auto i = 0; i < n_blocks; i++) {
        bitmap->set_block(i, 1);
    }

    EXPECT_EQ(bitmap->next_free(0), -1);

    bitmap->set_block(n_blocks - 1, 0);
    EXPECT_EQ(bitmap->next_free(0), n_blocks - 1);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, BlockBitmapTest, testing::ValuesIn(valid_block_sizes));

}