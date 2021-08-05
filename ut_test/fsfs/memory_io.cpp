#include "fsfs/memory_io.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/data_block.hpp"
#include "fsfs/file_system.hpp"
#include "fsfs/indirect_inode.hpp"
#include "fsfs/inode.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class MemoryIOTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    address invalid_inode = fs_nullptr;
    fsize n_ptr_in_data_block = block_size / sizeof(address);

    MemoryIO* io;
    Disk* disk;
    super_block MB;
    Inode* inode;
    IndirectInode* iinode;

    std::vector<address> used_data_blocks;
    std::vector<address> used_inode_blocks;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        inode = new Inode(*disk, MB);
        iinode = new IndirectInode(*disk, MB);

        io = new MemoryIO(*disk);

        set_dummy_blocks();
        io->init(MB);
        io->scan_blocks();

        invalid_inode = MB.n_inode_blocks - 1;
    }

    void TearDown() override {
        delete inode;
        delete iinode;

        delete io;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }

    void set_dummy_blocks() {
        inode->alloc(0);
        inode->commit();
        used_inode_blocks.push_back(0);

        inode->alloc(1);
        inode->update(1).file_len = 4 * block_size + 1;
        address ptr_offset = 0;
        for (auto& block_ptr : inode->update(1).block_ptr) {
            block_ptr = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
        }
        inode->commit();
        used_inode_blocks.push_back(1);

        fsize direct_ptr_size = meta_inode_ptr_len * block_size;
        fsize n_ptrs_indirect = MB.block_size / sizeof(address);
        address long_inode_addr = MB.block_size / meta_fragm_size + 1;
        fsize long_inode_len = direct_ptr_size;

        long_inode_len += 2 * (n_ptrs_indirect - 2) * block_size;
        inode->alloc(long_inode_addr);

        inode->update(long_inode_addr).file_len = long_inode_len;
        for (auto& block_ptr : inode->update(long_inode_addr).block_ptr) {
            block_ptr = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
        }
        inode->update(long_inode_addr).indirect_inode_ptr = 1024;
        inode->commit();
        used_inode_blocks.push_back(long_inode_addr);

        iinode->alloc(1024);
        iinode->set_indirect_address(1024, 1028);
        iinode->commit();
        used_data_blocks.push_back(1024);
        iinode->alloc(1028);
        iinode->commit();
        used_data_blocks.push_back(1028);

        fsize n_ptrs = (long_inode_len - direct_ptr_size) / MB.block_size;
        n_ptrs += (long_inode_len - direct_ptr_size) % MB.block_size ? 1 : 0;
        fsize nth_ptr = 0;
        for (auto i = 0; i < n_ptrs; i++) {
            iinode->set_ptr(1024, i) = ptr_offset;
            used_data_blocks.push_back(ptr_offset);
            ptr_offset += 1;
            nth_ptr += 1;

            if (nth_ptr == (n_ptrs_indirect - 1)) {
                iinode->commit();
                nth_ptr = 0;
            }
        }
        iinode->commit();
    }
};

TEST_F(MemoryIOTest, bytes_to_block_test) {
    EXPECT_EQ(io->bytes_to_blocks(0), 0);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size / 2), 1);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size), 1);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size + 1), 2);
    EXPECT_EQ(io->bytes_to_blocks(MB.block_size + MB.block_size / 2), 2);
    EXPECT_EQ(io->bytes_to_blocks(2 * MB.block_size), 2);
    EXPECT_EQ(io->bytes_to_blocks(2 * MB.block_size + 1), 3);
}

TEST_F(MemoryIOTest, scan_blocks_test) {
    for (const auto inode_n : used_inode_blocks) {
        EXPECT_TRUE(io->get_inode_bitmap().get_block_status(inode_n));
    }
    for (auto inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (std::find(used_inode_blocks.begin(), used_inode_blocks.end(),
                      inode_n) != used_inode_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(io->get_inode_bitmap().get_block_status(inode_n));
    }

    for (const auto data_n : used_data_blocks) {
        EXPECT_TRUE(io->get_data_bitmap().get_block_status(data_n));
    }
    for (auto data_n = 0; data_n < MB.n_inode_blocks; data_n++) {
        if (std::find(used_data_blocks.begin(), used_data_blocks.end(),
                      data_n) != used_data_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(io->get_inode_bitmap().get_block_status(data_n));
    }
}

TEST_F(MemoryIOTest, alloc_inode_test) {
    EXPECT_EQ(io->alloc_inode("TOO LONG FILE NAME 012345678910"), fs_nullptr);

    auto addr = io->alloc_inode("Inode name");

    EXPECT_TRUE(io->get_inode_bitmap().get_block_status(addr));
}

TEST_F(MemoryIOTest, dealloc_inode_test) {
    EXPECT_EQ(io->dealloc_inode(invalid_inode), fs_nullptr);

    ASSERT_TRUE(io->get_inode_bitmap().get_block_status(1));
    for (fsize ptr_n = 0; ptr_n < meta_inode_ptr_len; ptr_n++) {
        ASSERT_TRUE(
            io->get_data_bitmap().get_block_status(used_data_blocks.at(ptr_n)));
    }

    EXPECT_EQ(io->dealloc_inode(1), 1);

    EXPECT_FALSE(io->get_inode_bitmap().get_block_status(1));
    for (fsize ptr_n = 0; ptr_n < meta_inode_ptr_len; ptr_n++) {
        EXPECT_FALSE(
            io->get_data_bitmap().get_block_status(used_data_blocks.at(ptr_n)));
    }
}

TEST_F(MemoryIOTest, rename_inode_test) {
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

TEST_F(MemoryIOTest, add_data_test) {
    fsize data_len = MB.block_size * 2 + 1;
    std::vector<data> wdata(data_len);
    std::memcpy(wdata.data(), (void*)memcpy, wdata.size());

    EXPECT_EQ(io->add_data(invalid_inode, wdata.data(), data_len), fs_nullptr);

    constexpr const char* file_name = "SampleFile";
    address addr = io->alloc_inode(file_name);

    EXPECT_EQ(io->add_data(addr, wdata.data(), 0), 0);

    fsize n_written = io->add_data(addr, wdata.data(), data_len);
    EXPECT_EQ(n_written, data_len);
    EXPECT_EQ(inode->get(addr).file_len, data_len);

    DataBlock data_block(*disk, MB);
    std::vector<data> rdata(MB.block_size);
    data_block.read(inode->get(addr).block_ptr[0], rdata.data(), 0,
                    MB.block_size);
    for (auto i = 0; i < MB.block_size; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
    data_block.read(inode->get(addr).block_ptr[1], rdata.data(), 0,
                    MB.block_size);
    for (auto i = 0; i < MB.block_size; i++) {
        EXPECT_EQ(wdata[i + MB.block_size], rdata[i]);
    }
    data_block.read(inode->get(addr).block_ptr[2], rdata.data(), 0, 1);
    EXPECT_EQ(wdata[data_len - 1], rdata[0]);

    n_written = io->add_data(addr, wdata.data(), MB.block_size);
    EXPECT_EQ(n_written, MB.block_size);
    // HACK:
    inode->get(MB.n_inode_blocks - 1).file_len;
    // END OF HACK
    EXPECT_EQ(inode->get(addr).file_len, data_len + MB.block_size);

    // HACK:
    data_block.read(inode->get(addr).block_ptr[1], rdata.data(), 0,
                    MB.block_size);
    // END OF HACK
    data_block.read(inode->get(addr).block_ptr[2], rdata.data(), 1,
                    MB.block_size - 1);
    for (auto i = 0; i < MB.block_size - 1; i++) {
        EXPECT_EQ(wdata[i], rdata[i]);
    }
    data_block.read(inode->get(addr).block_ptr[3], rdata.data(), 0, 1);
    EXPECT_EQ(wdata[MB.block_size - 1], rdata[0]);

    // TODO not finished
}
}