#include "fsfs/memory_io.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class MemoryIOTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    fsize n_ptr_in_data_block = block_size / sizeof(address);
    MemoryIO* io;
    Disk* disk;
    super_block MB;

    std::vector<address> used_data_blocks;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        io = new MemoryIO(*disk);
    }
    void TearDown() override {
        delete io;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }

    void set_inode(address data_offset, address indirect_address,
                   bool allocated) {
        inode_block inode_data = {};

        inode_data.block_ptr[0] = data_offset + 0;
        inode_data.block_ptr[1] = data_offset + 1;
        inode_data.block_ptr[2] = data_offset + 2;
        inode_data.block_ptr[3] = data_offset + 3;
        inode_data.block_ptr[4] = data_offset + 4;
        inode_data.indirect_inode_ptr = indirect_address;
        if (indirect_address != fs_nullptr) {
            used_data_blocks.push_back(indirect_address);
        }

        io->set_data_blocks_status(inode_data, allocated);
    }

    void alloc_indirect(address block_n, address data_offset,
                        address indirect_address) {
        fsize n_addr = n_ptr_in_data_block;
        std::vector<data> indirect_block(block_size, 0x0);
        address* indirect_addr =
            reinterpret_cast<address*>(indirect_block.data());

        for (auto idx = 0; idx < n_addr - 1; idx++) {
            indirect_addr[idx] = idx + data_offset;
            used_data_blocks.push_back(indirect_addr[idx]);
        }
        indirect_addr[n_addr - 1] = indirect_address;
        if (indirect_address != fs_nullptr) {
            used_data_blocks.push_back(indirect_address);
        }

        io->set_data_block(block_n, *indirect_block.data());
    }
};

TEST_F(MemoryIOTest, set_inode_throw) {
    meta_block dummy_inode = {};
    dummy_inode = fs_nullptr;

    EXPECT_THROW(io->set_inode(0, dummy_inode.inode), std::invalid_argument);
    io->init(MB);
    EXPECT_THROW(io->set_inode(-1, dummy_inode.inode), std::invalid_argument);
    EXPECT_THROW(io->set_inode(0xFFFFF, dummy_inode.inode),
                 std::invalid_argument);
}

TEST_F(MemoryIOTest, get_inode_throw) {
    meta_block dummy_inode = {};
    dummy_inode = fs_nullptr;

    EXPECT_THROW(io->get_inode(0, dummy_inode.inode), std::invalid_argument);
    io->init(MB);
    EXPECT_THROW(io->get_inode(-1, dummy_inode.inode), std::invalid_argument);
    EXPECT_THROW(io->get_inode(0xFFFFF, dummy_inode.inode),
                 std::invalid_argument);
}

TEST_F(MemoryIOTest, set_and_get_inode) {
    io->init(MB);
    const char name[] = "Test inode";

    inode_block ref_inode_one = {};
    inode_block ref_inode_two = {};
    inode_block inode = {};

    ref_inode_one.type = block_status::Used;
    ref_inode_one.file_len = 1024 * 5;
    ref_inode_one.indirect_inode_ptr = fs_nullptr;
    ref_inode_one.block_ptr[0] = 1;
    ref_inode_one.block_ptr[1] = 3;
    ref_inode_one.block_ptr[2] = 4;
    ref_inode_one.block_ptr[3] = 5;
    ref_inode_one.block_ptr[4] = 8;
    std::memcpy(ref_inode_one.file_name, name, sizeof(name));

    ref_inode_two.type = block_status::Used;
    ref_inode_two.file_len = 1024 * 2;
    ref_inode_two.indirect_inode_ptr = fs_nullptr;
    ref_inode_two.block_ptr[0] = 20;
    ref_inode_two.block_ptr[1] = 31;
    ref_inode_two.block_ptr[2] = 451;
    ref_inode_two.block_ptr[3] = 51;
    ref_inode_two.block_ptr[4] = 82;
    std::memcpy(ref_inode_one.file_name, name, sizeof(name));

    io->set_inode(0, ref_inode_one);
    io->set_inode(3, ref_inode_two);
    io->set_inode(block_size / meta_fragm_size + 4, ref_inode_one);

    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(0));
    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(3));
    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(
        block_size / meta_fragm_size + 4));

    for (auto idx = 0; idx < meta_inode_ptr_len; idx++) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(
            ref_inode_one.block_ptr[idx]));
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(
            ref_inode_two.block_ptr[idx]));
    }

    io->get_inode(0, inode);

    EXPECT_EQ(ref_inode_one.type, inode.type);
    EXPECT_EQ(ref_inode_one.file_len, inode.file_len);
    EXPECT_EQ(ref_inode_one.block_ptr[0], inode.block_ptr[0]);
    EXPECT_EQ(ref_inode_one.block_ptr[1], inode.block_ptr[1]);
    EXPECT_EQ(ref_inode_one.block_ptr[2], inode.block_ptr[2]);
    EXPECT_EQ(ref_inode_one.block_ptr[3], inode.block_ptr[3]);
    EXPECT_EQ(ref_inode_one.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        EXPECT_EQ(ref_inode_one.file_name[i], inode.file_name[i]);
    }

    io->get_inode(3, inode);

    EXPECT_EQ(ref_inode_two.type, inode.type);
    EXPECT_EQ(ref_inode_two.file_len, inode.file_len);
    EXPECT_EQ(ref_inode_two.block_ptr[0], inode.block_ptr[0]);
    EXPECT_EQ(ref_inode_two.block_ptr[1], inode.block_ptr[1]);
    EXPECT_EQ(ref_inode_two.block_ptr[2], inode.block_ptr[2]);
    EXPECT_EQ(ref_inode_two.block_ptr[3], inode.block_ptr[3]);
    EXPECT_EQ(ref_inode_two.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        EXPECT_EQ(ref_inode_two.file_name[i], inode.file_name[i]);
    }

    io->get_inode(block_size / meta_fragm_size + 4, inode);

    EXPECT_EQ(ref_inode_one.type, inode.type);
    EXPECT_EQ(ref_inode_one.file_len, inode.file_len);
    EXPECT_EQ(ref_inode_one.block_ptr[0], inode.block_ptr[0]);
    EXPECT_EQ(ref_inode_one.block_ptr[1], inode.block_ptr[1]);
    EXPECT_EQ(ref_inode_one.block_ptr[2], inode.block_ptr[2]);
    EXPECT_EQ(ref_inode_one.block_ptr[3], inode.block_ptr[3]);
    EXPECT_EQ(ref_inode_one.block_ptr[4], inode.block_ptr[4]);

    for (auto i = 0; i < meta_file_name_len; i++) {
        EXPECT_EQ(ref_inode_one.file_name[i], inode.file_name[i]);
    }

    ref_inode_two.type = block_status::Free;
    io->set_inode(0, ref_inode_two);

    EXPECT_FALSE(io->get_inode_bitmap().get_block_status(0));

    for (auto idx = 0; idx < meta_inode_ptr_len; idx++) {
        EXPECT_FALSE(io->get_data_bitmap().get_block_status(
            ref_inode_one.block_ptr[idx]));
    }
}

TEST_F(MemoryIOTest, set_data_block_throw) {
    data dummy_data[1] = {};

    EXPECT_THROW(io->set_data_block(0, dummy_data[0]), std::invalid_argument);
    io->init(MB);
    EXPECT_THROW(io->set_data_block(-1, dummy_data[0]), std::invalid_argument);
    EXPECT_THROW(io->set_data_block(0xFFFFF, dummy_data[0]),
                 std::invalid_argument);
}

TEST_F(MemoryIOTest, get_data_block_throw) {
    data dummy_data[1] = {};

    EXPECT_THROW(io->get_data_block(0, dummy_data[0]), std::invalid_argument);
    io->init(MB);
    EXPECT_THROW(io->get_data_block(-1, dummy_data[0]), std::invalid_argument);
    EXPECT_THROW(io->get_data_block(0xFFFFF, dummy_data[0]),
                 std::invalid_argument);
}

TEST_F(MemoryIOTest, set_and_get_block) {
    io->init(MB);
    std::vector<data> ref_data(block_size);
    std::vector<data> r_data(block_size);
    std::memcpy(ref_data.data(), (void*)memcpy, block_size);

    io->set_data_block(345, ref_data.data()[0]);
    io->get_data_block(345, r_data.data()[0]);

    for (auto i = 0; i < block_size; i++) {
        EXPECT_EQ(ref_data[i], r_data[i]);
    }
}

TEST_F(MemoryIOTest, set_blocks_blocks_noindirect_test) {
    io->init(MB);
    set_inode(0, fs_nullptr, 1);

    for (auto& addr : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(addr));
    }

    for (auto& addr : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(addr));
    }

    set_inode(0, fs_nullptr, 0);

    for (auto& addr : used_data_blocks) {
        EXPECT_FALSE(io->get_data_bitmap().get_block_status(addr));
    }
}

TEST_F(MemoryIOTest, set_blocks_blocks_single_indirect_test) {
    io->init(MB);
    constexpr address indirect_block_addr = 1;
    alloc_indirect(indirect_block_addr, 100, fs_nullptr);
    set_inode(0, indirect_block_addr, 1);

    for (auto& addr : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(addr));
    }

    set_inode(0, indirect_block_addr, 0);

    for (auto& addr : used_data_blocks) {
        EXPECT_FALSE(io->get_data_bitmap().get_block_status(addr));
    }
}

TEST_F(MemoryIOTest, set_blocks_blocks_nested_indirect_test) {
    io->init(MB);
    constexpr address indirect_block_addr_one = 1;
    constexpr address indirect_block_addr_two = 2;
    alloc_indirect(indirect_block_addr_one, 100, indirect_block_addr_two);
    alloc_indirect(indirect_block_addr_two, 200, fs_nullptr);
    set_inode(0, indirect_block_addr_one, 1);

    for (auto& addr : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(addr));
    }

    set_inode(0, indirect_block_addr_one, 0);

    for (auto& addr : used_data_blocks) {
        EXPECT_FALSE(io->get_data_bitmap().get_block_status(addr));
    }
}

TEST_F(MemoryIOTest, set_blocks_blocks_discontinuity_test) {
    io->init(MB);
    inode_block inode_data = {};

    inode_data.block_ptr[0] = 0;
    inode_data.block_ptr[1] = 1;
    inode_data.block_ptr[2] = 2;
    inode_data.block_ptr[3] = fs_nullptr;
    inode_data.block_ptr[4] = 4;
    inode_data.indirect_inode_ptr = fs_nullptr;

    io->set_data_blocks_status(inode_data, 1);

    EXPECT_TRUE(io->get_data_bitmap().get_block_status(0));
    EXPECT_TRUE(io->get_data_bitmap().get_block_status(1));
    EXPECT_TRUE(io->get_data_bitmap().get_block_status(2));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(3));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(4));

    io->set_data_blocks_status(inode_data, 0);

    EXPECT_FALSE(io->get_data_bitmap().get_block_status(0));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(1));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(2));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(3));
    EXPECT_FALSE(io->get_data_bitmap().get_block_status(4));
}

TEST_F(MemoryIOTest, set_nth_ptr_throw_test) {
    EXPECT_THROW(io->set_nth_ptr(0, 0, 0), std::runtime_error);
    io->init(MB);

    EXPECT_THROW(io->set_nth_ptr(0, -1, 0), std::invalid_argument);
    EXPECT_THROW(io->set_nth_ptr(0, 0, -1), std::invalid_argument);
    EXPECT_THROW(io->set_nth_ptr(-1, 0, -1), std::invalid_argument);
}

TEST_F(MemoryIOTest, set_nth_ptr_test) {
    io->init(MB);

    inode_block inode = {};
    inode.type = block_status::Used;
    inode.file_len = 0;
    inode.block_ptr[0] = fs_nullptr;
    inode.indirect_inode_ptr = fs_nullptr;
    inode.file_len = 5 * block_size;
    io->set_inode(0, inode);

    io->set_nth_ptr(0, 0, 0);
    io->set_nth_ptr(0, 1, 1);
    io->set_nth_ptr(0, 2, 2);
    io->set_nth_ptr(0, 3, 3);
    io->set_nth_ptr(0, 4, 4);

    EXPECT_EQ(io->get_nth_ptr(0, 0), 0);
    EXPECT_EQ(io->get_nth_ptr(0, 1), 1);
    EXPECT_EQ(io->get_nth_ptr(0, 2), 2);
    EXPECT_EQ(io->get_nth_ptr(0, 3), 3);
    EXPECT_EQ(io->get_nth_ptr(0, 4), 4);
}

TEST_F(MemoryIOTest, set_nth_ptr_indirect_test) {
    io->init(MB);

    inode_block inode = {};
    inode.type = block_status::Used;
    inode.file_len = 0;
    inode.block_ptr[0] = fs_nullptr;
    inode.indirect_inode_ptr = fs_nullptr;
    inode.file_len = 300 * block_size;
    io->set_inode(0, inode);
    io->set_inode(1, inode);
    io->set_inode(2, inode);

    io->set_nth_ptr(0, meta_inode_ptr_len, meta_inode_ptr_len);
    io->set_nth_ptr(1, n_ptr_in_data_block + meta_inode_ptr_len,
                    meta_inode_ptr_len + n_ptr_in_data_block);
    // io->set_nth_ptr(0, n_ptr_in_data_block + 1, n_ptr_in_data_block + 1);
    // io->set_nth_ptr(0, n_ptr_in_data_block - 1, n_ptr_in_data_block - 1);

    EXPECT_EQ(io->get_nth_ptr(0, meta_inode_ptr_len), meta_inode_ptr_len);
    EXPECT_EQ(io->get_nth_ptr(1, meta_inode_ptr_len + n_ptr_in_data_block),
              meta_inode_ptr_len + n_ptr_in_data_block);
    // EXPECT_EQ(io->get_nth_ptr(0, n_ptr_in_data_block + 1),
    //           n_ptr_in_data_block + 1);
    // EXPECT_EQ(io->get_nth_ptr(0, n_ptr_in_data_block - 1),
    //           n_ptr_in_data_block - 1);
}

TEST_F(MemoryIOTest, get_nth_ptr_throw_test) {
    EXPECT_THROW(io->get_nth_ptr(0, 0), std::runtime_error);
    io->init(MB);

    EXPECT_THROW(io->get_nth_ptr(0, -1), std::invalid_argument);
    EXPECT_THROW(io->get_nth_ptr(-1, 0), std::invalid_argument);
}

TEST_F(MemoryIOTest, get_nth_ptr_test) {
    io->init(MB);
    inode_block inode = {};
    inode.type = block_status::Used;
    inode.file_len = 2 * block_size + 3;
    inode.block_ptr[0] = 0;
    inode.block_ptr[1] = 1;
    inode.block_ptr[2] = 2;
    inode.block_ptr[3] = 3;
    inode.block_ptr[4] = fs_nullptr;
    inode.indirect_inode_ptr = fs_nullptr;
    io->set_inode(0, inode);

    EXPECT_EQ(io->get_nth_ptr(0, 0), inode.block_ptr[0]);
    EXPECT_EQ(io->get_nth_ptr(0, 1), inode.block_ptr[1]);
    EXPECT_EQ(io->get_nth_ptr(0, 2), inode.block_ptr[2]);
    EXPECT_EQ(io->get_nth_ptr(0, 3), fs_nullptr);
    EXPECT_EQ(io->get_nth_ptr(0, 4), inode.block_ptr[4]);
    EXPECT_EQ(io->get_nth_ptr(0, 5), fs_nullptr);
}

TEST_F(MemoryIOTest, get_nth_ptr_indirect_test) {
    io->init(MB);
    inode_block inode = {};
    inode.type = block_status::Used;
    inode.file_len = 1024 * block_size;
    inode.indirect_inode_ptr = 1024;
    alloc_indirect(1024, 5, 1025);
    alloc_indirect(1025, n_ptr_in_data_block + 4, fs_nullptr);
    io->set_inode(0, inode);

    EXPECT_EQ(io->get_nth_ptr(0, 6), 6);
    EXPECT_EQ(io->get_nth_ptr(0, 64), 64);
    EXPECT_EQ(io->get_nth_ptr(0, 387), 387);
    EXPECT_EQ(io->get_nth_ptr(0, n_ptr_in_data_block), n_ptr_in_data_block);
    EXPECT_EQ(io->get_nth_ptr(0, n_ptr_in_data_block - 1),
              n_ptr_in_data_block - 1);
    EXPECT_EQ(io->get_nth_ptr(0, n_ptr_in_data_block + 1),
              n_ptr_in_data_block + 1);
    EXPECT_EQ(io->get_nth_ptr(0, 1024), fs_nullptr);
}
}