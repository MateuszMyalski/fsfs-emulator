#include "fsfs/inode.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class InodeTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    Inode* inode;

    inode_block ref_inode1 = {};
    inode_block ref_inode2 = {};

   public:
    void SetUp() override {
        const char name[] = "Test inode";

        ref_inode1.type = block_status::Used;
        ref_inode1.file_len = 1024 * 5;
        ref_inode1.indirect_inode_ptr = fs_nullptr;
        ref_inode1.block_ptr[0] = 1;
        ref_inode1.block_ptr[1] = 3;
        ref_inode1.block_ptr[2] = 4;
        ref_inode1.block_ptr[3] = 5;
        ref_inode1.block_ptr[4] = 8;
        std::memcpy(ref_inode1.file_name, name, sizeof(name));

        ref_inode2.type = block_status::Used;
        ref_inode2.file_len = 1024 * 2;
        ref_inode2.indirect_inode_ptr = fs_nullptr;
        ref_inode2.block_ptr[0] = 20;
        ref_inode2.block_ptr[1] = 31;
        ref_inode2.block_ptr[2] = 451;
        ref_inode2.block_ptr[3] = 51;
        ref_inode2.block_ptr[4] = 82;
        std::memcpy(ref_inode1.file_name, name, sizeof(name));

        inode = new Inode(disk, MB);
    }
    void TearDown() override { delete inode; }
};

TEST_P(InodeTest, get_inode_throw) {
    EXPECT_THROW(inode->get(-1), std::invalid_argument);
    EXPECT_THROW(inode->get(MB.n_inode_blocks), std::invalid_argument);
    EXPECT_THROW(inode->get(MB.n_inode_blocks + 1), std::invalid_argument);
}

TEST_P(InodeTest, update_inode_throw) {
    EXPECT_THROW(inode->update(-1), std::invalid_argument);
    EXPECT_THROW(inode->update(MB.n_inode_blocks), std::invalid_argument);
    EXPECT_THROW(inode->update(MB.n_inode_blocks + 1), std::invalid_argument);
}

TEST_P(InodeTest, alloc_inode_throw) {
    EXPECT_THROW(inode->alloc(-1), std::invalid_argument);
    EXPECT_THROW(inode->alloc(MB.n_inode_blocks), std::invalid_argument);
    EXPECT_THROW(inode->alloc(MB.n_inode_blocks + 1), std::invalid_argument);
}

TEST_P(InodeTest, get_inode) {
    address test_inode1 = 0;
    address test_inode2 = block_size / meta_fragm_size;

    disk.write(inode_block_to_addr(0), reinterpret_cast<data*>(&ref_inode1), meta_fragm_size);
    disk.write(inode_block_to_addr(1), reinterpret_cast<data*>(&ref_inode2), meta_fragm_size);

    EXPECT_EQ(ref_inode1.type, inode->get(test_inode1).type);
    EXPECT_EQ(ref_inode1.file_len, inode->get(test_inode1).file_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_EQ(ref_inode1.block_ptr[i], inode->get(test_inode1).block_ptr[i]);
    }

    EXPECT_EQ(ref_inode2.type, inode->get(test_inode2).type);
    EXPECT_EQ(ref_inode2.file_len, inode->get(test_inode2).file_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_EQ(ref_inode2.block_ptr[i], inode->get(test_inode2).block_ptr[i]);
    }
}

TEST_P(InodeTest, update_inode) {
    address test_inode1 = 0;
    address test_inode2 = block_size / meta_fragm_size;

    inode->alloc(test_inode1);
    inode->update(test_inode1).file_len = ref_inode1.file_len;
    inode->update(test_inode1).indirect_inode_ptr = ref_inode1.indirect_inode_ptr;
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->update(test_inode1).block_ptr[i] = ref_inode1.block_ptr[i];
    }
    std::strcpy(inode->update(test_inode1).file_name, ref_inode1.file_name);
    inode->commit();

    inode->alloc(test_inode2);
    inode->update(test_inode2).file_len = ref_inode2.file_len;
    inode->update(test_inode2).indirect_inode_ptr = ref_inode2.indirect_inode_ptr;
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        inode->update(test_inode2).block_ptr[i] = ref_inode2.block_ptr[i];
    }
    std::strcpy(inode->update(test_inode2).file_name, ref_inode2.file_name);
    inode->commit();

    std::vector<data> rdata(meta_fragm_size);
    inode_block* rinode = reinterpret_cast<inode_block*>(rdata.data());

    disk.read(inode_block_to_addr(0), rdata.data(), meta_fragm_size);
    EXPECT_EQ(rinode->type, ref_inode1.type);
    EXPECT_EQ(rinode->file_len, ref_inode1.file_len);
    EXPECT_EQ(rinode->indirect_inode_ptr, ref_inode1.indirect_inode_ptr);
    EXPECT_STREQ(rinode->file_name, ref_inode1.file_name);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_EQ(rinode->block_ptr[i], ref_inode1.block_ptr[i]);
    }

    disk.read(inode_block_to_addr(1), rdata.data(), meta_fragm_size);
    EXPECT_EQ(rinode->type, ref_inode2.type);
    EXPECT_EQ(rinode->file_len, ref_inode2.file_len);
    EXPECT_EQ(rinode->indirect_inode_ptr, ref_inode2.indirect_inode_ptr);
    EXPECT_STREQ(rinode->file_name, ref_inode2.file_name);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_EQ(rinode->block_ptr[i], ref_inode2.block_ptr[i]);
    }
}

TEST_P(InodeTest, alloc_inode) {
    address test_inode1 = 0;
    address test_inode2 = block_size / meta_fragm_size;

    inode->alloc(test_inode1);
    inode->commit();

    inode->alloc(test_inode2);
    inode->commit();

    EXPECT_EQ(inode->alloc(test_inode1), fs_nullptr);

    EXPECT_EQ(inode->get(test_inode1).type, block_status::Used);
    EXPECT_EQ(inode->get(test_inode1).file_len, 0);
    EXPECT_EQ(inode->get(test_inode1).file_name[0], '\0');
    EXPECT_EQ(inode->get(test_inode1).indirect_inode_ptr, fs_nullptr);
    for (auto& ptr : inode->get(test_inode1).block_ptr) {
        EXPECT_EQ(ptr, fs_nullptr);
    }

    EXPECT_EQ(inode->get(test_inode2).type, block_status::Used);
    EXPECT_EQ(inode->get(test_inode2).file_len, 0);
    EXPECT_EQ(inode->get(test_inode2).file_name[0], '\0');
    EXPECT_EQ(inode->get(test_inode2).indirect_inode_ptr, fs_nullptr);
    for (auto& ptr : inode->get(test_inode2).block_ptr) {
        EXPECT_EQ(ptr, fs_nullptr);
    }
}

INSTANTIATE_TEST_SUITE_P(BlockSize, InodeTest, testing::ValuesIn(valid_block_sizes));
}