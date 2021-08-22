#include "fsfs/memory_io.hpp"

#include "fsfs/data_block.hpp"
#include "fsfs/file_system.hpp"
#include "fsfs/indirect_inode.hpp"
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
        Inode inode(disk, MB);
        IndirectInode iinode(disk, MB);

        inode.alloc(0);
        inode.commit();
        used_inode_blocks.push_back(0);

        inode.alloc(1);
        inode.update(1).file_len = 4 * block_size + 1;
        address ptr_offset = 0;
        for (auto& block_ptr : inode.update(1).block_ptr) {
            block_ptr = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
        }
        inode.commit();
        used_inode_blocks.push_back(1);

        fsize direct_ptr_size = meta_inode_ptr_len * block_size;
        fsize n_ptrs_indirect = MB.block_size / sizeof(address);
        address long_inode_addr = MB.block_size / meta_fragm_size + 1;
        fsize long_inode_len = direct_ptr_size;

        long_inode_len += 2 * (n_ptrs_indirect - 2) * block_size;
        inode.alloc(long_inode_addr);

        inode.update(long_inode_addr).file_len = long_inode_len;
        for (auto& block_ptr : inode.update(long_inode_addr).block_ptr) {
            block_ptr = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
        }
        inode.update(long_inode_addr).indirect_inode_ptr = 1024;
        inode.commit();
        used_inode_blocks.push_back(long_inode_addr);

        iinode.alloc(1024);
        iinode.set_indirect_address(1024, 1028);
        iinode.commit();
        used_data_blocks.push_back(1024);
        iinode.alloc(1028);
        iinode.commit();
        used_data_blocks.push_back(1028);

        fsize n_ptrs = (long_inode_len - direct_ptr_size) / MB.block_size;
        n_ptrs += (long_inode_len - direct_ptr_size) % MB.block_size ? 1 : 0;
        fsize nth_ptr = 0;
        for (auto i = 0; i < n_ptrs; i++) {
            iinode.set_ptr(1024, i) = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
            nth_ptr += 1;

            if (nth_ptr == (n_ptrs_indirect - 1)) {
                iinode.commit();
                nth_ptr = 0;
            }
        }
        iinode.commit();
    }
};

TEST_P(MemoryIOTest, bytes_to_block) {
    EXPECT_EQ(io->bytes_to_blocks(0), 0);
    EXPECT_EQ(io->bytes_to_blocks(-1), 0);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size / 2), 1);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size), 1);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size + 1), 2);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size + MB.block_size / 2), 2);
    EXPECT_EQ(io->bytes_to_blocks(2 * MB.block_size), 2);
    EXPECT_EQ(io->bytes_to_blocks(2 * MB.block_size + 1), 3);
}

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

    auto addr = io->alloc_inode("Inode name");

    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(addr));
}

TEST_P(MemoryIOTest, dealloc_inode) {
    address invalid_inode = MB.n_inode_blocks - 1;
    EXPECT_EQ(io->dealloc_inode(invalid_inode), fs_nullptr);

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
