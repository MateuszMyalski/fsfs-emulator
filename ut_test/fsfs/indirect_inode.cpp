#include "fsfs/indirect_inode.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class IndirectInodeTest : public ::testing::Test {
   protected:
    IndirectInode* iinode;

    constexpr static fsize block_size = 1024;
    fsize n_ptrs_in_block = MB.block_size / sizeof(address);
    address data_blocks_offset = fs_offset_inode_block + MB.n_inode_blocks;
    Disk* disk;
    super_block MB;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        iinode = new IndirectInode(*disk, MB);
    }
    void TearDown() override {
        delete iinode;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }
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

TEST_F(IndirectInodeTest, get_indirect_block_address_throw_test) {
    EXPECT_THROW(iinode->get_block_address(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->get_block_address(MB.n_data_blocks, 0),
                 std::invalid_argument);
    EXPECT_THROW(iinode->get_block_address(0, -1), std::invalid_argument);
}

TEST_F(IndirectInodeTest, set_indirect_address_throw_test) {
    EXPECT_THROW(iinode->set_indirect_address(-1, 0), std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(MB.n_data_blocks, 0),
                 std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(0, -1), std::invalid_argument);
    EXPECT_THROW(iinode->set_indirect_address(0, MB.n_data_blocks),
                 std::invalid_argument);
}

TEST_F(IndirectInodeTest, alloc_indirect_inode_throw_test) {
    EXPECT_THROW(iinode->alloc(-1), std::invalid_argument);
    EXPECT_THROW(iinode->alloc(MB.n_data_blocks), std::invalid_argument);
}

TEST_F(IndirectInodeTest, get_ptr_test) {
    address ptr_idx = 0;
    address base_addr = 0;
    std::vector<address> indirect_block(n_ptrs_in_block);
    data* indirect_data = reinterpret_cast<data*>(indirect_block.data());

    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;

    disk->write(data_blocks_offset, indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, 0), 0);
    EXPECT_EQ(iinode->get_ptr(base_addr, 150), 150);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block - 2),
              n_ptrs_in_block - 2);
    EXPECT_THROW(iinode->get_ptr(base_addr, n_ptrs_in_block - 1),
                 std::runtime_error);

    indirect_block[n_ptrs_in_block - 1] = 1024;
    disk->write(data_blocks_offset, indirect_data, MB.block_size);

    ptr_idx = n_ptrs_in_block - 1;
    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;
    disk->write(data_blocks_offset + 1024, indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block), n_ptrs_in_block);
    EXPECT_EQ(iinode->get_ptr(base_addr, 260), 260);
    EXPECT_THROW(iinode->get_ptr(base_addr, 2 * n_ptrs_in_block - 1),
                 std::runtime_error);

    indirect_block[n_ptrs_in_block - 1] = 512;
    disk->write(data_blocks_offset + 1024, indirect_data, MB.block_size);

    ptr_idx = 2 * n_ptrs_in_block - 2;
    for (auto& ptr : indirect_block) {
        ptr = ptr_idx++;
    }
    indirect_block[n_ptrs_in_block - 1] = fs_nullptr;
    disk->write(data_blocks_offset + 512, indirect_data, MB.block_size);
    EXPECT_EQ(iinode->get_ptr(base_addr, 0), 0);
    EXPECT_EQ(iinode->get_ptr(base_addr, n_ptrs_in_block), n_ptrs_in_block);
    EXPECT_EQ(iinode->get_ptr(base_addr, 2 * n_ptrs_in_block),
              2 * n_ptrs_in_block);
    EXPECT_THROW(iinode->get_ptr(base_addr, 3 * n_ptrs_in_block - 1),
                 std::runtime_error);
}

TEST_F(IndirectInodeTest, set_ptr_test) {
    constexpr address base_addr = 0;
    constexpr address second_indirect = 10;
    constexpr address third_indirect = 15;
    address w_ptr_idx = 0;

    iinode->alloc(base_addr);
    iinode->commit();
    iinode->alloc(second_indirect);
    iinode->commit();
    iinode->alloc(third_indirect);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, i) = i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    iinode->commit();

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    iinode->set_indirect_address(base_addr, second_indirect);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, w_ptr_idx + i) = w_ptr_idx + i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    iinode->commit();

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    iinode->set_indirect_address(second_indirect, third_indirect);
    iinode->commit();

    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        iinode->set_ptr(base_addr, w_ptr_idx + i) = w_ptr_idx + i;
    }
    w_ptr_idx += n_ptrs_in_block - 1;
    iinode->commit();

    EXPECT_THROW(iinode->set_ptr(base_addr, w_ptr_idx + 1), std::runtime_error);

    std::vector<data> rdata(block_size);
    address* riinode = reinterpret_cast<address*>(rdata.data());
    disk->read(fs_offset_inode_block + MB.n_inode_blocks + base_addr,
               rdata.data(), block_size);

    address r_ptr_idx = 0;
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;

    disk->read(fs_offset_inode_block + MB.n_inode_blocks + second_indirect,
               rdata.data(), block_size);
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], r_ptr_idx + i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;

    disk->read(fs_offset_inode_block + MB.n_inode_blocks + third_indirect,
               rdata.data(), block_size);
    for (auto i = 0; i < n_ptrs_in_block - 1; i++) {
        EXPECT_EQ(riinode[i], r_ptr_idx + i);
    }
    r_ptr_idx += n_ptrs_in_block - 1;
}

TEST_F(IndirectInodeTest, get_block_address) {
    constexpr address base_addr = 0;
    constexpr address second_indirect = 10;
    constexpr address third_indirect = 15;

    iinode->alloc(base_addr);
    iinode->commit();
    iinode->alloc(second_indirect);
    iinode->commit();
    iinode->alloc(third_indirect);
    iinode->commit();

    iinode->set_indirect_address(base_addr, second_indirect);
    iinode->commit();
    iinode->set_indirect_address(second_indirect, third_indirect);
    iinode->commit();

    EXPECT_EQ(iinode->get_block_address(base_addr, n_ptrs_in_block - 2),
              base_addr);
    EXPECT_EQ(iinode->get_block_address(base_addr, 2 * n_ptrs_in_block - 3),
              second_indirect);
    EXPECT_EQ(iinode->get_block_address(base_addr, 3 * n_ptrs_in_block - 4),
              third_indirect);
}
}