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
    constexpr static fsize metablocks_in_block = block_size / meta_block_size;

    Disk* dummy_disk;
    BlockMap* block_map;
    super_block MB;

    std::vector<address> used_data_blocks;
    std::vector<address> used_inode_blocks;

   public:
    void SetUp() override {
        dummy_disk = new Disk(block_size);
        block_map = new BlockMap();

        Disk::create("tmp_disk.img", 500, block_size);
        dummy_disk->open("tmp_disk.img");
        FileSystem::format(*dummy_disk);

        dummy_disk->mount();
        dummy_disk->read(super_block_offset, reinterpret_cast<data*>(&MB),
                         meta_block_size);
    }

    void TearDown() override {
        dummy_disk->unmount();
        EXPECT_FALSE(dummy_disk->is_mounted());

        delete block_map;
        delete dummy_disk;

        std::remove("tmp_disk.img");
    }

    address set_dummy_inode(address inode_n, address data_offset,
                            address indirect_inode) {
        std::vector<data> data(block_size, 0x0);
        inode_block* inodes = reinterpret_cast<inode_block*>(data.data());

        address last_data_block = data_offset;
        for (auto idx = 0; idx < metablocks_in_block; idx++) {
            if (idx == 3) {
                inodes[idx].type = block_status::FREE;
                continue;
            }

            inodes[idx].type = block_status::USED;
            const char file_name[] = "TEST INODE";
            std::memcpy(inodes[idx].file_name, file_name, sizeof(file_name));

            inodes[idx].block_ptr[0] = last_data_block;
            inodes[idx].block_ptr[1] = last_data_block + 3;
            inodes[idx].block_ptr[2] =
                idx == 5 ? last_data_block + 4 : inode_empty_ptr;
            inodes[idx].block_ptr[3] =
                idx == 5 ? last_data_block + 7 : inode_empty_ptr;
            inodes[idx].block_ptr[4] =
                idx == 5 ? last_data_block + 8 : inode_empty_ptr;
            last_data_block += 10;

            inodes[idx].indirect_inode_ptr = indirect_inode;

            used_inode_blocks.push_back(inode_n + idx);
            for (auto i = 0; i < inode_n_block_ptr_len; i++) {
                if (inodes[idx].block_ptr[i] == inode_empty_ptr) {
                    continue;
                }
                used_data_blocks.push_back(inodes[idx].block_ptr[i]);
            }
        }

        dummy_disk->write(inode_blocks_offset + inode_n, data.data(),
                          block_size);

        return last_data_block;
    }
    // void set_dummy_indirect_inode(address data_n, address data_offset) {}
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
    auto last_data_n = set_dummy_inode(0, 0, inode_empty_ptr);
    set_dummy_inode(1, last_data_n, inode_empty_ptr);

    block_map->scan_blocks(*dummy_disk, MB);

    for (auto& addr : used_inode_blocks) {
        ASSERT_EQ(block_map->get_block_status<map_type::INODE>(addr),
                  block_status::USED);
    }

    for (auto& addr : used_data_blocks) {
        ASSERT_EQ(block_map->get_block_status<map_type::DATA>(addr),
                  block_status::USED);
    }
}
}