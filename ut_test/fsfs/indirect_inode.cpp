#include "fsfs/indirect_inode.hpp"

#include "test_base.hpp"

using namespace FSFS;
namespace {
class IndirectInodeTest : public ::testing::Test, public TestBaseFileSystem {
   protected:
    IndirectInode* iinode;

    fsize n_ptrs_in_block = MB.block_size / sizeof(address);

   public:
    void SetUp() override { iinode = new IndirectInode(disk, MB); }
    void TearDown() override { delete iinode; }
};

TEST_F(IndirectInodeTest, get_ptr_throw) {
    EXPECT_THROW(iinode->get_ptr(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->get_ptr(MB.n_data_blocks, 0), std::invalid_argument);
    EXPECT_THROW(iinode->get_ptr(0, -1), std::invalid_argument);
}

TEST_F(IndirectInodeTest, set_ptr_throw) {
    EXPECT_THROW(iinode->set_ptr(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->set_ptr(MB.n_data_blocks, 0), std::invalid_argument);
    EXPECT_THROW(iinode->set_ptr(0, -1), std::invalid_argument);
}

TEST_F(IndirectInodeTest, get_indirect_block_address_throw) {
    EXPECT_THROW(iinode->get_block_address(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->get_block_address(MB.n_data_blocks, 0), std::invalid_argument);
    EXPECT_THROW(iinode->get_block_address(0, -1), std::invalid_argument);
}

TEST_F(IndirectInodeTest, set_indirect_address_throw) {
    EXPECT_THROW(iinode->set_indirect_address(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(MB.n_data_blocks, 0), std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(0, -1), std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(0, MB.n_data_blocks), std::invalid_argument);
}

TEST_F(IndirectInodeTest, alloc_indirect_inode_throw) {
    EXPECT_THROW(iinode->alloc(-1), std::invalid_argument);
    EXPECT_THROW(iinode->alloc(MB.n_data_blocks), std::invalid_argument);
}

TEST_F(IndirectInodeTest, get_ptr) {
    address ptr_idx = 0;
    address base_addr = 0;
    std::vector<address> indirect_block(n_ptrs_in_block);
    data* indirect_data = reinterpret_cast<data*>(indirect_block.data());

    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;

    disk.write(get_data_block_offset(), indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, 0), 0);
    EXPECT_EQ(iinode->get_ptr(base_addr, 150), 150);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block - 2), n_ptrs_in_block - 2);
    EXPECT_THROW(iinode->get_ptr(base_addr, n_ptrs_in_block - 1), std::runtime_error);

    indirect_block[n_ptrs_in_block - 1] = 1024;
    disk.write(get_data_block_offset(), indirect_data, MB.block_size);

    ptr_idx = n_ptrs_in_block - 1;
    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;
    disk.write(get_data_block_offset() + 1024, indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block), n_ptrs_in_block);
    EXPECT_EQ(iinode->get_ptr(base_addr, 260), 260);
    EXPECT_THROW(iinode->get_ptr(base_addr, 2 * n_ptrs_in_block - 1), std::runtime_error);

    indirect_block[n_ptrs_in_block - 1] = 512;
    disk.write(get_data_block_offset() + 1024, indirect_data, MB.block_size);

    ptr_idx = 2 * n_ptrs_in_block - 2;
    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;
    disk.write(get_data_block_offset() + 512, indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, 0), 0);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block), n_ptrs_in_block);
    EXPECT_EQ(iinode->get_ptr(base_addr, 2 * n_ptrs_in_block), 2 * n_ptrs_in_block);
    EXPECT_THROW(iinode->get_ptr(base_addr, 3 * n_ptrs_in_block - 1), std::runtime_error);
}

TEST_F(IndirectInodeTest, set_ptr) {
    constexpr address base_addr = 0;
    constexpr address second_node = 10;
    constexpr address third_node = 15;
    address w_ptr_idx = 0;

    iinode->alloc(base_addr);
    iinode->commit();
    iinode->alloc(second_node);
    iinode->commit();
    iinode->alloc(third_node);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, i) = i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    /* No iinode->commit(); to check auto commit */

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    iinode->set_indirect_address(base_addr, second_node);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, w_ptr_idx + i) = w_ptr_idx + i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    iinode->commit();

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    iinode->set_indirect_address(second_node, third_node);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, w_ptr_idx + i) = w_ptr_idx + i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    iinode->commit();

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    std::vector<data> rdata(block_size);
    address* riinode = reinterpret_cast<address*>(rdata.data());
    disk.read(data_block_to_addr(base_addr), rdata.data(), block_size);

    address r_ptr_idx = 0;
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;

    disk.read(data_block_to_addr(second_node), rdata.data(), block_size);
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], r_ptr_idx + i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;

    disk.read(data_block_to_addr(third_node), rdata.data(), block_size);
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], r_ptr_idx + i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;
}

TEST_F(IndirectInodeTest, get_block_address) {
    constexpr address base_addr = 0;
    constexpr address second_node = 10;
    constexpr address third_node = 15;

    iinode->alloc(base_addr);
    iinode->commit();
    iinode->alloc(second_node);
    iinode->commit();
    iinode->alloc(third_node);
    iinode->commit();

    iinode->set_indirect_address(base_addr, second_node);
    iinode->commit();
    iinode->set_indirect_address(second_node, third_node);
    iinode->commit();

    EXPECT_EQ(iinode->get_block_address(base_addr, n_ptrs_in_block - 2), base_addr);
    EXPECT_EQ(iinode->get_block_address(base_addr, 2 * n_ptrs_in_block - 3), second_node);
    EXPECT_EQ(iinode->get_block_address(base_addr, 3 * n_ptrs_in_block - 4), third_node);
}
}