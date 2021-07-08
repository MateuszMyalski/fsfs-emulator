#include "fsfs/block_map.hpp"

#include <exception>
#include <vector>

#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class BlockMapTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    constexpr static fsize data_n_blocks = 2048;
    constexpr static fsize inode_n_blocks = 512;
    Disk* dummy_disk;
    BlockMap* block_map;

   public:
    void SetUp() override {
        dummy_disk = new Disk(block_size);
        block_map = new BlockMap();
        dummy_disk->mount();
    }
    void TearDown() override {
        dummy_disk->unmount();
        EXPECT_FALSE(dummy_disk->is_mounted());
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
}

TEST_F(BlockMapTest, set_inode_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(block_map->set_block<map_type::INODE>(inode_n_blocks,
                                                       block_status::FREE),
                 std::invalid_argument);
    EXPECT_THROW(block_map->set_block<map_type::INODE>(-1, block_status::FREE),
                 std::invalid_argument);

    block_map->set_block<map_type::INODE>(inode_n_blocks - 1,
                                          block_status::FREE);
}

TEST_F(BlockMapTest, set_data_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(
        block_map->set_block<map_type::DATA>(data_n_blocks, block_status::FREE),
        std::invalid_argument);
    EXPECT_THROW(block_map->set_block<map_type::DATA>(-1, block_status::FREE),
                 std::invalid_argument);

    block_map->set_block<map_type::DATA>(data_n_blocks - 1, block_status::FREE);
}

TEST_F(BlockMapTest, get_inode_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(block_map->get_block_status<map_type::INODE>(inode_n_blocks),
                 std::invalid_argument);
    EXPECT_THROW(block_map->get_block_status<map_type::INODE>(-1),
                 std::invalid_argument);

    block_map->get_block_status<map_type::INODE>(inode_n_blocks - 1);
}

TEST_F(BlockMapTest, get_data_block_throw) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_THROW(block_map->get_block_status<map_type::DATA>(data_n_blocks),
                 std::invalid_argument);
    EXPECT_THROW(block_map->get_block_status<map_type::DATA>(-1),
                 std::invalid_argument);

    block_map->get_block_status<map_type::DATA>(data_n_blocks - 1);
}

TEST_F(BlockMapTest, set_and_get_map_line_border) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(0),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(63),
              block_status::FREE);

    block_map->set_block<map_type::DATA>(64, block_status::USED);

    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(64),
              block_status::USED);

    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(65),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<map_type::INODE>(0),
              block_status::FREE);

    EXPECT_EQ(block_map->get_block_status<map_type::INODE>(63),
              block_status::FREE);

    block_map->set_block<map_type::INODE>(64, block_status::USED);

    EXPECT_EQ(block_map->get_block_status<map_type::INODE>(64),
              block_status::USED);

    EXPECT_EQ(block_map->get_block_status<map_type::INODE>(65),
              block_status::FREE);
}

TEST_F(BlockMapTest, set_and_get_data_block) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    block_map->set_block<map_type::DATA>(block_size - 1, block_status::USED);
    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(block_size - 1),
              block_status::USED);

    block_map->set_block<map_type::DATA>(block_size + 1, block_status::USED);
    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(block_size + 1),
              block_status::USED);

    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(block_size + 2),
              block_status::FREE);

    block_map->set_block<map_type::DATA>(block_size + 1, block_status::FREE);
    EXPECT_EQ(block_map->get_block_status<map_type::DATA>(block_size + 1),
              block_status::FREE);
}

TEST_F(BlockMapTest, set_and_get_inode_block) {
    block_map->initialize(data_n_blocks, inode_n_blocks);

    block_map->set_block<map_type::INODE>(inode_n_blocks - 1,
                                          block_status::USED);
    EXPECT_EQ(block_map->get_block_status<map_type::INODE>(inode_n_blocks - 1),
              block_status::USED);
}

TEST_F(BlockMapTest, scan_data_blocks) {
    constexpr int32_t subblocks_in_block = block_size / meta_block_size;
    const char file_name[] = "TEST INODE";

    Disk::create("disk.img", 1024, block_size);
    dummy_disk->open("disk.img");
    FileSystem::format(*dummy_disk);

    std::vector<data> mb_data = {};
    mb_data.resize(block_size);
    dummy_disk->read(super_block_offset, mb_data.data(), block_size);

    std::vector<data> data = {};
    data.resize(block_size);

    inode_block* inodes = reinterpret_cast<inode_block*>(data.data());

    address last_data_block = 0;
    std::vector<address> used_data_blocks;
    std::vector<address> used_inode_blocks;

    for (auto inode = 0; inode < subblocks_in_block; inode++) {
        if ((subblocks_in_block % 3) == 0) {
            continue;
        }
        used_inode_blocks.push_back(inode);
        inodes[inode].type = block_status::USED;
        std::memcpy(inodes[inode].file_name, file_name, sizeof(file_name));

        // for every inode pattern - 010110011
        inodes[inode].block_ptr[0] = last_data_block + 1;
        inodes[inode].block_ptr[1] = last_data_block + 3;
        inodes[inode].block_ptr[2] = last_data_block + 4;
        inodes[inode].block_ptr[3] = last_data_block + 7;
        inodes[inode].block_ptr[4] = last_data_block + 8;
        last_data_block += 10;

        for (auto j = 0; j < inode_n_block_ptr_len; j++) {
            used_data_blocks.push_back(inodes[inode].block_ptr[j]);
        }

        inodes[inode].indirect_inode_ptr = inode_empty_ptr;
    }

    dummy_disk->write(inode_blocks_offset, data.data(), block_size);

    block_map->scan_blocks(*dummy_disk,
                           *reinterpret_cast<super_block*>(mb_data.data()));

    for (auto& inode_n : used_inode_blocks) {
        ASSERT_EQ(block_map->get_block_status<map_type::INODE>(inode_n),
                  block_status::USED);
    }

    for (auto& inode_n : used_data_blocks) {
        ASSERT_EQ(block_map->get_block_status<map_type::DATA>(inode_n),
                  block_status::USED);
    }
}
}