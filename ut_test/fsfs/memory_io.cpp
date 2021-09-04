#include "fsfs/memory_io.hpp"

#include "fsfs/block.hpp"
#include "fsfs/block_bitmap.hpp"
#include "fsfs/inode.hpp"
#include "test_base.hpp"
using namespace FSFS;
namespace {
class MemoryIOTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    MemoryIO* io;

    std::vector<address> used_data_blocks;
    std::vector<address> used_inode_blocks;

   public:
    void SetUp() override {
        io = new MemoryIO(disk);

        set_dummy_blocks();
        io->init(MB);
        io->scan_blocks();
    }

    void TearDown() override { delete io; }

    void set_dummy_blocks() {
        BlockBitmap bitmap(n_blocks);
        Block data_block(disk, MB);
        Inode inode;

        inode.alloc_new(0);
        inode.commit(data_block, bitmap);
        used_inode_blocks.push_back(0);

        inode.alloc_new(1);
        auto inode1_len = 4 * block_size + 1;
        inode.meta().file_len = inode1_len;
        for (auto i = 0; i < data_block.bytes_to_blocks(inode1_len); i++) {
            auto block_n = bitmap.next_free(0);
            bitmap.set_block(block_n, 1);
            used_data_blocks.push_back(block_n);
            inode.add_data(block_n);
        }
        inode.commit(data_block, bitmap);
        used_inode_blocks.push_back(1);

        inode.alloc_new(2);
        auto inode2_len = (meta_inode_ptr_len * block_size) + ((n_indirect_ptrs_in_block - 1) * 4 * block_size);
        inode.meta().file_len = inode2_len;
        for (auto i = 0; i < data_block.bytes_to_blocks(inode2_len); i++) {
            auto block_n = bitmap.next_free(0);
            bitmap.set_block(block_n, 1);
            used_data_blocks.push_back(block_n);
            inode.add_data(block_n);
        }
        inode.commit(data_block, bitmap);
        used_inode_blocks.push_back(2);
    }
};

TEST_P(MemoryIOTest, scan_blocks) {
    for (const auto inode_n : used_inode_blocks) {
        EXPECT_TRUE(io->get_inode_bitmap().get_block_status(inode_n));
    }
    for (auto inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (std::find(used_inode_blocks.begin(), used_inode_blocks.end(), inode_n) != used_inode_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(io->get_inode_bitmap().get_block_status(inode_n));
    }

    for (const auto data_n : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(data_n));
    }
    for (auto data_n = 0; data_n < MB.n_inode_blocks; data_n++) {
        if (std::find(used_data_blocks.begin(), used_data_blocks.end(), data_n) != used_data_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(io->get_inode_bitmap().get_block_status(data_n));
    }
}

TEST_P(MemoryIOTest, alloc_inode) {
    EXPECT_EQ(io->alloc_inode("TOO LONG FILE NAME 012345678910"), fs_nullptr);

    auto inode_n = io->alloc_inode("Inode name");

    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(inode_n));
}

TEST_P(MemoryIOTest, dealloc_inode) {
    address invalid_inode_n = MB.n_inode_blocks - 1;
    EXPECT_EQ(io->dealloc_inode(invalid_inode_n), fs_nullptr);

    ASSERT_TRUE(io->get_inode_bitmap().get_block_status(1));
    for (fsize ptr_n = 0; ptr_n < meta_inode_ptr_len; ptr_n++) {
        ASSERT_TRUE(io->get_data_bitmap().get_block_status(used_data_blocks.at(ptr_n)));
    }

    EXPECT_EQ(io->dealloc_inode(1), 1);

    EXPECT_FALSE(io->get_inode_bitmap().get_block_status(1));
    for (fsize ptr_n = 0; ptr_n < meta_inode_ptr_len; ptr_n++) {
        EXPECT_FALSE(io->get_data_bitmap().get_block_status(used_data_blocks.at(ptr_n)));
    }
}

TEST_P(MemoryIOTest, rename_inode) {
    constexpr const char* file_name_ref = "Inode name_ref";
    address addr = io->alloc_inode(file_name_ref);

    char file_name[meta_file_name_len] = {};
    io->get_inode_file_name(addr, file_name);
    EXPECT_STREQ(file_name, file_name_ref);

    constexpr const char* file_name_ref2 = "New inode name";
    io->rename_inode(addr, file_name_ref2);
    file_name[0] = '\0';
    io->get_inode_file_name(addr, file_name);
    EXPECT_STREQ(file_name, file_name_ref2);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, MemoryIOTest, testing::ValuesIn(valid_block_sizes));
}
