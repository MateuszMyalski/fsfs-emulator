#include "fsfs/block_map.hpp"

#include <exception>

#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class BlockMapTest : public ::testing::Test {
   protected:
    constexpr static v_size block_size = 1024;
    constexpr static v_size data_n_blocks = 2048;
    constexpr static v_size inode_n_blocks = 512;
    Disk* dummy_disk;
    BlockMap* block_map;

   public:
    void SetUp() override {
        dummy_disk = new Disk(block_size);
        block_map = new BlockMap(*dummy_disk);
        dummy_disk->mount();
    }
    void TearDown() override {
        delete block_map;
        delete dummy_disk;
    }
};
TEST_F(BlockMapTest, initialize_throw) {
    EXPECT_THROW(block_map->initialize(0, inode_n_blocks),
                 std::invalid_argument);

    EXPECT_THROW(block_map->initialize(data_n_blocks, 0),
                 std::invalid_argument);

    block_map->initialize(data_n_blocks, inode_n_blocks);

    dummy_disk->unmount();
    EXPECT_THROW(block_map->initialize(data_n_blocks, inode_n_blocks),
                 std::runtime_error);
}

TEST_F(BlockMapTest, set_inode_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(block_map->set_block<BlockMap::map_type::INODE>(
                     inode_n_blocks, block_status::FREE),
                 std::invalid_argument);
    EXPECT_THROW(
        block_map->set_block<BlockMap::map_type::INODE>(-1, block_status::FREE),
        std::invalid_argument);

    block_map->set_block<BlockMap::map_type::INODE>(inode_n_blocks - 1,
                                                    block_status::FREE);
}

TEST_F(BlockMapTest, set_data_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(block_map->set_block<BlockMap::map_type::DATA>(
                     data_n_blocks, block_status::FREE),
                 std::invalid_argument);
    EXPECT_THROW(
        block_map->set_block<BlockMap::map_type::DATA>(-1, block_status::FREE),
        std::invalid_argument);

    block_map->set_block<BlockMap::map_type::DATA>(data_n_blocks - 1,
                                                   block_status::FREE);
}

TEST_F(BlockMapTest, get_inode_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(
        block_map->get_block_status<BlockMap::map_type::INODE>(inode_n_blocks),
        std::invalid_argument);
    EXPECT_THROW(block_map->get_block_status<BlockMap::map_type::INODE>(-1),
                 std::invalid_argument);

    block_map->get_block_status<BlockMap::map_type::INODE>(inode_n_blocks - 1);
}

TEST_F(BlockMapTest, get_data_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(
        block_map->get_block_status<BlockMap::map_type::DATA>(data_n_blocks),
        std::invalid_argument);
    EXPECT_THROW(block_map->get_block_status<BlockMap::map_type::DATA>(-1),
                 std::invalid_argument);

    block_map->get_block_status<BlockMap::map_type::DATA>(data_n_blocks - 1);
}

TEST_F(BlockMapTest, set_and_get_map_line_border) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::DATA>(0),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::DATA>(63),
              block_status::FREE);

    block_map->set_block<BlockMap::map_type::DATA>(64, block_status::USED);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::DATA>(64),
              block_status::USED);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::DATA>(65),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::INODE>(0),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::INODE>(63),
              block_status::FREE);

    block_map->set_block<BlockMap::map_type::INODE>(64, block_status::USED);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::INODE>(64),
              block_status::USED);

    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::INODE>(65),
              block_status::FREE);
}

TEST_F(BlockMapTest, set_and_get_data_block) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    block_map->set_block<BlockMap::map_type::DATA>(block_size - 1,
                                                   block_status::USED);
    EXPECT_EQ(
        block_map->get_block_status<BlockMap::map_type::DATA>(block_size - 1),
        block_status::USED);

    block_map->set_block<BlockMap::map_type::DATA>(block_size + 1,
                                                   block_status::USED);
    EXPECT_EQ(
        block_map->get_block_status<BlockMap::map_type::DATA>(block_size + 1),
        block_status::USED);

    EXPECT_EQ(
        block_map->get_block_status<BlockMap::map_type::DATA>(block_size + 2),
        block_status::FREE);

    block_map->set_block<BlockMap::map_type::DATA>(block_size + 1,
                                                   block_status::FREE);
    EXPECT_EQ(
        block_map->get_block_status<BlockMap::map_type::DATA>(block_size + 1),
        block_status::FREE);
}

TEST_F(BlockMapTest, set_and_get_inode_block) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    block_map->set_block<BlockMap::map_type::INODE>(inode_n_blocks - 1,
                                                    block_status::USED);
    EXPECT_EQ(block_map->get_block_status<BlockMap::map_type::INODE>(
                  inode_n_blocks - 1),
              block_status::USED);
}
}