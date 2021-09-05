#include "fsfs/inode.hpp"

#include "fsfs/block_bitmap.hpp"
#include "test_base.hpp"
using namespace FSFS;
namespace {
class InodeTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    BlockBitmap data_bitmap;
    Block *data_block;
    Inode *inode;

    address test_inode_n = n_meta_blocks_in_block * 2 + 1;
    inode_block ref_inode1 = {};
    inode_block ref_inode2 = {};
    address ref_inode1_n = 0;
    address ref_inode2_n = block_size / meta_fragm_size;

   public:
    void SetUp() override {
        data_bitmap.init(MB.n_data_blocks);
        data_block = new Block(disk, MB);

        const char name[] = "Test inode";

        ref_inode1.type = block_status::Used;
        ref_inode1.file_len = block_size * meta_inode_ptr_len;
        ref_inode1.indirect_inode_ptr = fs_nullptr;
        fill_dummy(ref_inode1.block_ptr);
        std::memcpy(ref_inode1.file_name, name, sizeof(name));

        ref_inode2.type = block_status::Used;
        ref_inode2.file_len = block_size * 2;
        ref_inode2.indirect_inode_ptr = fs_nullptr;
        fill_dummy(ref_inode2.block_ptr);
        std::memcpy(ref_inode1.file_name, name, sizeof(name));

        disk.write(ref_inode1_n / n_meta_blocks_in_block + fs_offset_inode_block, cast_to_data(&ref_inode1),
                   meta_fragm_size);
        disk.write(ref_inode2_n / n_meta_blocks_in_block + fs_offset_inode_block, cast_to_data(&ref_inode2),
                   meta_fragm_size);

        inode = new Inode();

        ASSERT_EQ(ref_inode1.file_len, block_size * meta_inode_ptr_len);
    }

    void update_meta_data(inode_block &inode_to_insert) {
        inode->meta().type = inode_to_insert.type;
        inode->meta().file_len = inode_to_insert.file_len;
        inode->meta().indirect_inode_ptr = inode_to_insert.indirect_inode_ptr;
        std::memcpy(inode_to_insert.file_name, inode->meta().file_name, meta_file_name_len);
    }

    void TearDown() override {
        delete data_block;
        delete inode;
    }
};

TEST(InodeTest, meta_throw_not_initialized) {
    Inode inode;
    inode.add_data(2);
    EXPECT_THROW(inode.meta().file_name, std::runtime_error);
}

TEST(InodeTest, get_ptr_operator_throw_not_initialized) {
    Inode inode;
    EXPECT_THROW(inode.ptr(0), std::runtime_error);
}

TEST_P(InodeTest, load_and_check_meta) {
    inode->load(ref_inode1_n, *data_block);

    EXPECT_EQ(ref_inode1.type, inode->meta().type);
    EXPECT_EQ(ref_inode1.file_len, inode->meta().file_len);
    EXPECT_EQ(ref_inode1.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_STREQ(ref_inode1.file_name, inode->meta().file_name);
    std::memcpy(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr));
    std::memcpy(ref_inode1.file_name, inode->meta().file_name, meta_file_name_len);

    inode->load(ref_inode2_n, *data_block);
    EXPECT_EQ(ref_inode2.type, inode->meta().type);
    EXPECT_EQ(ref_inode2.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_EQ(ref_inode2.file_len, inode->meta().file_len);
    EXPECT_STREQ(ref_inode2.file_name, inode->meta().file_name);
    EXPECT_TRUE(cmp_data(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr)));
}

TEST_P(InodeTest, load_and_clear_throw_not_initialized) {
    inode->load(ref_inode1_n, *data_block);
    inode->clear();

    EXPECT_THROW(inode->meta().type, std::runtime_error);
}

TEST_P(InodeTest, alloc_new_commit_clear_load_direct_inode) {
    inode->alloc_new(test_inode_n);
    update_meta_data(ref_inode1);
    std::memcpy(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr));

    auto n_written = inode->commit(*data_block, data_bitmap);
    EXPECT_EQ(n_written, 0);

    inode->clear();
    inode->load(test_inode_n, *data_block);

    EXPECT_EQ(ref_inode1.type, inode->meta().type);
    EXPECT_EQ(ref_inode1.file_len, inode->meta().file_len);
    EXPECT_EQ(ref_inode1.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_STREQ(ref_inode1.file_name, inode->meta().file_name);
    EXPECT_TRUE(cmp_data(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr)));
}

TEST_P(InodeTest, add_data_clear_load_direct_inode) {
    inode->alloc_new(test_inode_n);
    update_meta_data(ref_inode1);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->add_data(ref_inode1.block_ptr[i]);
    }

    auto n_written = inode->commit(*data_block, data_bitmap);
    EXPECT_EQ(n_written, meta_inode_ptr_len);

    inode->clear();
    inode->load(test_inode_n, *data_block);

    EXPECT_EQ(ref_inode1.type, inode->meta().type);
    EXPECT_EQ(ref_inode1.file_len, inode->meta().file_len);
    EXPECT_EQ(ref_inode1.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_STREQ(ref_inode1.file_name, inode->meta().file_name);
    EXPECT_TRUE(cmp_data(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr)));
}

TEST_P(InodeTest, add_data_clear_load_indirect_inode) {
    constexpr address indirect_ptr = 123;

    inode->alloc_new(test_inode_n);
    update_meta_data(ref_inode1);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->add_data(ref_inode1.block_ptr[i]);
    }
    inode->add_data(indirect_ptr);

    auto n_written = inode->commit(*data_block, data_bitmap);
    EXPECT_EQ(n_written, meta_inode_ptr_len + 1);

    inode->clear();
    inode->load(test_inode_n, *data_block);
    EXPECT_EQ(inode->ptr(meta_inode_ptr_len), indirect_ptr);
}

TEST_P(InodeTest, add_data_clear_load_indirect_inode_with_no_free_space) {
    for (auto i = 0; i < MB.n_data_blocks; i++) {
        data_bitmap.set_status(i, 1);
    }

    inode->alloc_new(test_inode_n);
    update_meta_data(ref_inode1);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->add_data(ref_inode1.block_ptr[i]);
    }
    inode->add_data(123);

    auto n_written = inode->commit(*data_block, data_bitmap);
    EXPECT_EQ(n_written, meta_inode_ptr_len);
}

INSTANTIATE_TEST_SUITE_P(BlockSize, InodeTest, testing::ValuesIn(valid_block_sizes));
}