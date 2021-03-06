#include "fsfs/indirect_inode.hpp"

#include "fsfs/block.hpp"
#include "fsfs/block_bitmap.hpp"
#include "test_base.hpp"

using namespace FSFS;
namespace {
class IndirectInodeTest : public ::testing::TestWithParam<int32_t>, public TestBaseFileSystem {
   protected:
    std::unique_ptr<IndirectInode> indirect_inode;
    std::unique_ptr<Block> block;
    inode_block inode;
    std::vector<int32_t> ptrs;

    int32_t indirect0_data_block = 9;
    int32_t indirect1_data_block = 5;
    int32_t indirect2_data_block = 8;
    int32_t indirect3_data_block = fs_nullptr;

   public:
    void SetUp() override {
        indirect_inode = std::make_unique<IndirectInode>(inode);
        block = std::make_unique<Block>(disk, MB);

        auto n_ptrs = (n_indirect_ptrs_in_block - 1) * 3 - 2;
        ptrs.resize(n_ptrs);
        fill_dummy(ptrs);

        auto ptr_cnt = 0;
        inode.file_len = (n_ptrs + meta_n_direct_ptrs) * block_size;
        inode.indirect_inode_ptr = indirect0_data_block;

        store_indirect(block->data_n_to_block_n(indirect0_data_block), &ptrs[ptr_cnt], indirect1_data_block);
        ptr_cnt += n_indirect_ptrs_in_block - 1;
        store_indirect(block->data_n_to_block_n(indirect1_data_block), &ptrs[ptr_cnt], indirect2_data_block);
        ptr_cnt += n_indirect_ptrs_in_block - 1;
        store_indirect(block->data_n_to_block_n(indirect2_data_block), &ptrs[ptr_cnt], indirect3_data_block);
    }

   private:
    void store_indirect(int32_t block_n, int32_t* ptrs_list, int32_t next_indirect_addr) {
        block->write(block_n, cast_to_data(ptrs_list), 0, block_size - sizeof(int32_t));
        block->write(block_n, cast_to_data(&next_indirect_addr), block_size - sizeof(int32_t), sizeof(int32_t));
    }
};

TEST_P(IndirectInodeTest, load_and_check) {
    indirect_inode->load(*block);
    for (size_t i = 0; i < ptrs.size(); i++) {
        EXPECT_EQ(ptrs[i], indirect_inode->ptr(i));
    }
}

TEST_P(IndirectInodeTest, add_data_and_commit) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap(MB.n_data_blocks);
    indirect_inode->load(*block);

    std::vector<int32_t> new_ptrs((n_indirect_ptrs_in_block - 1) * 2);
    fill_dummy(new_ptrs);
    new_ptrs_list.insert_after(new_ptrs_list.before_begin(), new_ptrs.begin(), new_ptrs.end());

    auto n_written = indirect_inode->commit(*block, data_bitmap, new_ptrs_list);
    EXPECT_EQ(n_written, (n_indirect_ptrs_in_block - 1) * 2);
    inode.file_len += new_ptrs.size() * block_size;
    indirect_inode->load(*block);

    for (size_t i = 0; i < ptrs.size(); i++) {
        EXPECT_EQ(ptrs[i], indirect_inode->ptr(i));
    }
    for (size_t i = 0; i < new_ptrs.size(); i++) {
        EXPECT_EQ(new_ptrs[i], indirect_inode->ptr(ptrs.size() + i));
    }
}

TEST_P(IndirectInodeTest, add_data_and_last_indirect_ptr) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap(MB.n_data_blocks);
    indirect_inode->load(*block);

    std::vector<int32_t> new_ptrs((n_indirect_ptrs_in_block - 1) * 2);
    fill_dummy(new_ptrs);
    new_ptrs_list.insert_after(new_ptrs_list.before_begin(), new_ptrs.begin(), new_ptrs.end());

    auto n_written = indirect_inode->commit(*block, data_bitmap, new_ptrs_list);
    EXPECT_EQ(n_written, (n_indirect_ptrs_in_block - 1) * 2);
    inode.file_len += new_ptrs.size() * block_size;
    indirect_inode->load(*block);

    EXPECT_NE(indirect_inode->last_indirect_ptr(0), fs_nullptr);
    EXPECT_NE(indirect_inode->last_indirect_ptr(1), fs_nullptr);
    EXPECT_EQ(indirect_inode->last_indirect_ptr(2), indirect2_data_block);
    EXPECT_EQ(indirect_inode->last_indirect_ptr(3), indirect1_data_block);
    EXPECT_EQ(indirect_inode->last_indirect_ptr(4), indirect0_data_block);
}

TEST_P(IndirectInodeTest, add_data_and_last_indirect_ptr_overflow) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap(MB.n_data_blocks);
    indirect_inode->load(*block);
    EXPECT_EQ(indirect_inode->last_indirect_ptr(3), fs_nullptr);
}

TEST_P(IndirectInodeTest, commit_throw_no_indirect_base_address) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap(MB.n_data_blocks);

    indirect_inode->load(*block);
    inode.indirect_inode_ptr = fs_nullptr;
    EXPECT_THROW(indirect_inode->commit(*block, data_bitmap, new_ptrs_list), std::runtime_error);
}

TEST_P(IndirectInodeTest, commit_with_empty_list) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap(n_blocks);

    indirect_inode->load(*block);
    EXPECT_NO_THROW(indirect_inode->commit(*block, data_bitmap, new_ptrs_list));
}

TEST_P(IndirectInodeTest, add_data_and_commit_with_no_free_space) {
    PtrsLList new_ptrs_list;
    constexpr auto n_free_blocks = 1;
    BlockBitmap data_bitmap(MB.n_data_blocks);
    for (auto i = 0; i < MB.n_data_blocks - n_free_blocks; i++) {
        data_bitmap.set_status(i, 1);
    }

    std::vector<int32_t> new_ptrs((n_indirect_ptrs_in_block - 1) * 2);
    fill_dummy(new_ptrs);
    new_ptrs_list.insert_after(new_ptrs_list.before_begin(), new_ptrs.begin(), new_ptrs.end());

    indirect_inode->load(*block);
    auto n_written = indirect_inode->commit(*block, data_bitmap, new_ptrs_list);
    EXPECT_EQ(n_written, 2 + (n_indirect_ptrs_in_block - 1));
}

INSTANTIATE_TEST_SUITE_P(BlockSize, IndirectInodeTest, testing::ValuesIn(valid_block_sizes));
}