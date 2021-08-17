#include "fsfs/data_block.hpp"
#include "fsfs/indirect_inode.hpp"
#include "fsfs/inode.hpp"
#include "fsfs/memory_io.hpp"
#include "test_base.hpp"

using namespace FSFS;
namespace {
class MemoryIOWriteDataTest : public ::testing::Test, public TestBaseFileSystem {
   protected:
    constexpr static const char* file_name = "SampleFile";
    fsize n_ptr_in_data_block = block_size / sizeof(address);

    MemoryIO* io;
    Inode* inode;
    IndirectInode* iinode;

   public:
    using BufferType = std::vector<data>;
    void SetUp() override {
        inode = new Inode(disk, MB);
        iinode = new IndirectInode(disk, MB);

        io = new MemoryIO(disk);

        io->init(MB);
        io->scan_blocks();
    }

    void TearDown() override {
        delete inode;
        delete iinode;

        delete io;
    }

    bool check_block(address block_n, const BufferType& ref_data, BufferType::const_iterator& ref_data_it) {
        DataBlock data_block(disk, MB);
        BufferType rdata(block_size);
        data_block.read(block_n, rdata.data(), 0, block_size);

        auto rdata_it = rdata.cbegin();
        while ((rdata_it != rdata.end()) && (ref_data_it != ref_data.end())) {
            if (*rdata_it != *ref_data_it) {
                return false;
            }
            rdata_it++;
            ref_data_it++;
        }
        return true;
    }
};

TEST_F(MemoryIOWriteDataTest, sanity_check) {
    address invalid_inode = MB.n_inode_blocks - 1;
    BufferType wdata(1);

    EXPECT_EQ(io->write_data(invalid_inode, wdata.data(), 0, 1), fs_nullptr);

    address addr = io->alloc_inode(file_name);

    EXPECT_EQ(io->write_data(addr, wdata.data(), 0, 0), 0);
}

TEST_F(MemoryIOWriteDataTest, two_blocks) {
    fsize data_len = block_size * 2;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    ASSERT_NE(inode->get(addr).block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[1], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[1], wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, uneven_direct_blocks) {
    fsize data_len = block_size * 2 + 1;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    ASSERT_NE(inode->get(addr).block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[1], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[2], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[1], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[2], wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, single_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 1;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        ASSERT_NE(inode->get(addr).block_ptr[i], fs_nullptr);
    }
    ASSERT_NE(inode->get(addr).indirect_inode_ptr, fs_nullptr);
    auto indirect_n0 = iinode->get_ptr(inode->get(addr).indirect_inode_ptr, 0);
    ASSERT_NE(indirect_n0, fs_nullptr);

    auto wdata_it = wdata.cbegin();
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_TRUE(check_block(inode->get(addr).block_ptr[i], wdata, wdata_it));
    }
    EXPECT_TRUE(check_block(indirect_n0, wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, append_data) {
    fsize data_len = block_size * 2 + 1;
    fsize total_len = data_len + data_len;
    BufferType wdata(total_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    n_written += io->write_data(addr, wdata.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, total_len);
    ASSERT_NE(inode->get(addr).block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[1], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[2], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[3], fs_nullptr);
    ASSERT_NE(inode->get(addr).block_ptr[4], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[1], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[2], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[3], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->get(addr).block_ptr[4], wdata, wdata_it));
}

TEST_F(MemoryIOWriteDataTest, nested_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size * iinode->get_n_ptrs_in_block();
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address addr = io->alloc_inode(file_name);
    fsize n_written = io->write_data(addr, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->reinit();
    EXPECT_EQ(inode->get(addr).file_len, data_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        ASSERT_NE(inode->get(addr).block_ptr[i], fs_nullptr);
    }

    fsize reserved_blocks = io->bytes_to_blocks(data_len);
    auto wdata_it = wdata.cbegin();

    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        EXPECT_TRUE(check_block(inode->get(addr).block_ptr[i], wdata, wdata_it));
    }
    address base_indirect = inode->get(addr).indirect_inode_ptr;
    ASSERT_NE(base_indirect, fs_nullptr);
    for (auto i = 0; i < reserved_blocks - meta_inode_ptr_len; i++) {
        address block_addr = iinode->get_ptr(base_indirect, i);
        ASSERT_NE(block_addr, fs_nullptr);
        EXPECT_TRUE(check_block(block_addr, wdata, wdata_it));
    }
}

TEST_F(MemoryIOWriteDataTest, offset_write) {}
}
