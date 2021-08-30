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
    constexpr address inode_n = 12;

    inode->alloc_new(inode_n);
    inode->meta().type = ref_inode1.type;
    inode->meta().file_len = ref_inode1.file_len;
    inode->meta().indirect_inode_ptr = ref_inode1.indirect_inode_ptr;
    std::memcpy(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr));
    std::memcpy(ref_inode1.file_name, inode->meta().file_name, meta_file_name_len);

    inode->commit(*data_block, data_bitmap);
    inode->clear();
    inode->load(inode_n, *data_block);

    EXPECT_EQ(ref_inode1.type, inode->meta().type);
    EXPECT_EQ(ref_inode1.file_len, inode->meta().file_len);
    EXPECT_EQ(ref_inode1.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_STREQ(ref_inode1.file_name, inode->meta().file_name);
    EXPECT_TRUE(cmp_data(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr)));
}

TEST_P(InodeTest, add_data_clear_load_direct_inode) {
    address inode_n = n_meta_blocks_in_block * 2 + 1;
    ASSERT_EQ(ref_inode1.file_len, block_size * meta_inode_ptr_len);

    inode->alloc_new(inode_n);
    inode->meta().type = ref_inode1.type;
    inode->meta().file_len = ref_inode1.file_len;
    inode->meta().indirect_inode_ptr = ref_inode1.indirect_inode_ptr;
    std::memcpy(ref_inode1.file_name, inode->meta().file_name, meta_file_name_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->add_data(ref_inode1.block_ptr[i]);
    }

    inode->commit(*data_block, data_bitmap);
    inode->clear();
    inode->load(inode_n, *data_block);

    EXPECT_EQ(ref_inode1.type, inode->meta().type);
    EXPECT_EQ(ref_inode1.file_len, inode->meta().file_len);
    EXPECT_EQ(ref_inode1.indirect_inode_ptr, inode->meta().indirect_inode_ptr);
    EXPECT_STREQ(ref_inode1.file_name, inode->meta().file_name);
    EXPECT_TRUE(cmp_data(ref_inode1.block_ptr, inode->meta().block_ptr, sizeof(ref_inode1.block_ptr)));
}

TEST_P(InodeTest, add_data_clear_load_indirect_inode) {
    constexpr address indirect_ptr = 123;
    address inode_n = n_meta_blocks_in_block * 2 + 1;
    ASSERT_EQ(ref_inode1.file_len, block_size * meta_inode_ptr_len);

    inode->alloc_new(inode_n);
    inode->meta().type = ref_inode1.type;
    inode->meta().file_len = ref_inode1.file_len;
    inode->meta().indirect_inode_ptr = ref_inode1.indirect_inode_ptr;
    std::memcpy(ref_inode1.file_name, inode->meta().file_name, meta_file_name_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->add_data(ref_inode1.block_ptr[i]);
    }
    inode->add_data(indirect_ptr);

    inode->commit(*data_block, data_bitmap);
    inode->clear();
    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->ptr(meta_inode_ptr_len), indirect_ptr);
}

TEST_P(InodeTest, add_data_clear_load_indirect_inode_with_no_free_space) {}

INSTANTIATE_TEST_SUITE_P(BlockSize, InodeTest, testing::ValuesIn(valid_block_sizes));
}