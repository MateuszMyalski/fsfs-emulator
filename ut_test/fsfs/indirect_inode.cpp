#include "fsfs/indirect_inode.hpp"

#include "fsfs/block.hpp"
#include "fsfs/block_bitmap.hpp"
#include "test_base.hpp"

using namespace FSFS;
namespace {
class IndirectInodeTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    IndirectInode* indirect_inode;
    Block* data_block;
    inode_block inode;
    std::vector<address> ptrs;

    address indirect0_data_block = 9;
    address indirect1_data_block = 5;
    address indirect2_data_block = 8;
    address indirect3_data_block = fs_nullptr;

   public:
    void SetUp() override {
        indirect_inode = new IndirectInode(inode);
        data_block = new Block(disk, MB);

        auto n_ptrs = (n_indirect_ptrs_in_block - 1) * 3 - 2;
        ptrs.resize(n_ptrs);
        fill_dummy(ptrs);

        auto ptr_cnt = 0;
        inode.file_len = (n_ptrs + meta_inode_ptr_len) * block_size;
        inode.indirect_inode_ptr = indirect0_data_block;

        store_indirect(data_block->data_n_to_block_n(indirect0_data_block), &ptrs[ptr_cnt], indirect1_data_block);
        ptr_cnt += n_indirect_ptrs_in_block - 1;
        store_indirect(data_block->data_n_to_block_n(indirect1_data_block), &ptrs[ptr_cnt], indirect2_data_block);
        ptr_cnt += n_indirect_ptrs_in_block - 1;
        store_indirect(data_block->data_n_to_block_n(indirect2_data_block), &ptrs[ptr_cnt], indirect3_data_block);
    }

    void TearDown() override {
        delete indirect_inode;
        delete data_block;
    }

    void store_indirect(address block_n, address* ptrs_list, address next_indirect_addr) {
        data_block->write(block_n, cast_to_data(ptrs_list), 0, block_size - sizeof(address));
        data_block->write(block_n, cast_to_data(&next_indirect_addr), block_size - sizeof(address), sizeof(address));
    }
};

TEST_P(IndirectInodeTest, load_and_check) {
    indirect_inode->load(*data_block);
    for (size_t i = 0; i < ptrs.size(); i++) {
        EXPECT_EQ(ptrs[i], indirect_inode->ptr(i));
    }
}
TEST_P(IndirectInodeTest, add_data_and_commit) {
    PtrsLList new_ptrs_list;
    BlockBitmap data_bitmap;
    data_bitmap.init(n_blocks);
    auto n_new_ptrs = (n_indirect_ptrs_in_block - 1) * 2;
    indirect_inode->load(*data_block);

    std::vector<address> new_ptrs(n_new_ptrs);
    fill_dummy(new_ptrs);

    new_ptrs_list.insert_after(new_ptrs_list.before_begin(), new_ptrs.begin(), new_ptrs.end());
    indirect_inode->commit(*data_block, data_bitmap, new_ptrs_list);

    inode.file_len += n_new_ptrs * block_size;
    indirect_inode->load(*data_block);
    for (size_t i = 0; i < ptrs.size(); i++) {
        EXPECT_EQ(ptrs[i], indirect_inode->ptr(i));
    }
    for (size_t i = 0; i < new_ptrs.size(); i++) {
        EXPECT_EQ(new_ptrs[i], indirect_inode->ptr(ptrs.size() + i));
    }
}

TEST_P(InodeTest, commit_throw_no_indirect_base_address) {}

TEST_P(IndirectInodeTest, add_data_and_commit_with_empty_list) {}

TEST_P(IndirectInodeTest, add_data_and_commit_with_no_free_space) {}

TEST_P(IndirectInodeTest, add_data_to_no_indirect_throw) {}

TEST_P(IndirectInodeTest, load_clear_and_check) {}

INSTANTIATE_TEST_SUITE_P(BlockSize, IndirectInodeTest, testing::ValuesIn(valid_block_sizes));
}