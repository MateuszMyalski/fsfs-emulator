#include "fsfs/block.hpp"
#include "fsfs/indirect_inode.hpp"
#include "fsfs/inode.hpp"
#include "fsfs/memory_io.hpp"
#include "test_base.hpp"

using namespace FSFS;
namespace {
class MemoryIOWriteDataTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    constexpr static const char* file_name = "SampleFile";
    fsize n_ptr_in_data_block = block_size / sizeof(address);

    MemoryIO* io;
    Inode* inode;
    Block* data_block;

   public:
    using BufferType = std::vector<data>;
    void SetUp() override {
        data_block = new Block(disk, MB);
        inode = new Inode();

        io = new MemoryIO(disk);

        io->init(MB);
        io->scan_blocks();
    }

    void TearDown() override {
        delete inode;
        delete data_block;

        delete io;
    }

    bool check_block(address data_n, const BufferType& ref_data, BufferType::const_iterator& ref_data_it) {
        BufferType rdata(block_size);
        data_block->read(data_block->data_n_to_block_n(data_n), rdata.data(), 0, block_size);

        auto rdata_it = rdata.cbegin();
        while ((rdata_it != rdata.cend()) && (ref_data_it != ref_data.cend())) {
            if (*rdata_it != *ref_data_it) {
                printf("Expected: %d instead of: %d\n", *ref_data_it, *rdata_it);
                return false;
            }
            rdata_it++;
            ref_data_it++;
        }
        return true;
    }
};

TEST_P(MemoryIOWriteDataTest, sanity_check) {
    address invalid_inode = MB.n_inode_blocks - 1;
    BufferType wdata(1);

    EXPECT_EQ(io->write_data(invalid_inode, wdata.data(), 0, 1), fs_nullptr);

    address inode_n = io->alloc_inode(file_name);

    EXPECT_EQ(io->write_data(inode_n, wdata.data(), 0, 0), 0);
}

TEST_P(MemoryIOWriteDataTest, two_blocks) {
    fsize data_len = block_size * 2;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, data_len);
    ASSERT_NE(inode->meta().block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->meta().block_ptr[1], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->meta().block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->meta().block_ptr[1], wdata, wdata_it));
}

TEST_P(MemoryIOWriteDataTest, uneven_direct_blocks) {
    fsize data_len = block_size * 2 + 1;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, data_len);
    ASSERT_NE(inode->meta().block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->meta().block_ptr[1], fs_nullptr);
    ASSERT_NE(inode->meta().block_ptr[2], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->meta().block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->meta().block_ptr[1], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->meta().block_ptr[2], wdata, wdata_it));
}

TEST_P(MemoryIOWriteDataTest, single_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 1;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, data_len);
    ASSERT_NE(inode->meta().indirect_inode_ptr, fs_nullptr);

    for (auto i = 0; i < meta_inode_ptr_len + 1; i++) {
        ASSERT_NE(inode->ptr(i), fs_nullptr);
    }

    auto wdata_it = wdata.cbegin();
    for (auto i = 0; i < meta_inode_ptr_len + 1; i++) {
        EXPECT_TRUE(check_block(inode->ptr(i), wdata, wdata_it));
    }
}

TEST_P(MemoryIOWriteDataTest, append_data_uneven) {
    fsize data_len = block_size * 2 + 1;
    fsize total_len = data_len + data_len;
    BufferType wdata(total_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    n_written += io->write_data(inode_n, wdata.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, total_len);
    ASSERT_NE(inode->ptr(0), fs_nullptr);
    ASSERT_NE(inode->ptr(1), fs_nullptr);
    ASSERT_NE(inode->ptr(2), fs_nullptr);
    ASSERT_NE(inode->ptr(3), fs_nullptr);
    ASSERT_NE(inode->ptr(4), fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->ptr(0), wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->ptr(1), wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->ptr(2), wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->ptr(3), wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->ptr(4), wdata, wdata_it));
}

TEST_P(MemoryIOWriteDataTest, append_data_even) {
    fsize data_len = block_size;
    fsize total_len = data_len + data_len;
    BufferType wdata(total_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    n_written += io->write_data(inode_n, wdata.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, total_len);
    ASSERT_NE(inode->meta().block_ptr[0], fs_nullptr);
    ASSERT_NE(inode->meta().block_ptr[1], fs_nullptr);

    auto wdata_it = wdata.cbegin();
    EXPECT_TRUE(check_block(inode->meta().block_ptr[0], wdata, wdata_it));
    EXPECT_TRUE(check_block(inode->meta().block_ptr[1], wdata, wdata_it));
}

TEST_P(MemoryIOWriteDataTest, nested_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size * (data_block->get_n_addreses_in_block() - 1);
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, data_len);
    for (auto i = 0; i < meta_inode_ptr_len; i++) {
        ASSERT_NE(inode->meta().block_ptr[i], fs_nullptr);
    }

    fsize reserved_blocks = data_block->bytes_to_blocks(data_len);
    auto wdata_it = wdata.cbegin();

    for (auto i = 0; i < reserved_blocks; i++) {
        EXPECT_TRUE(check_block(inode->ptr(i), wdata, wdata_it));
    }
}

TEST_P(MemoryIOWriteDataTest, offset_write) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size;
    BufferType wdata(data_len);
    fill_dummy(wdata);

    address inode_n = io->alloc_inode(file_name);
    fsize n_written = io->write_data(inode_n, wdata.data(), 0, data_len);

    /* First edit */
    constexpr fsize edit1_length = 20;
    constexpr fsize offset1 = 30;
    BufferType edit1_data(edit1_length);
    memset(edit1_data.data(), 0xAA, edit1_length);
    memcpy(&wdata[data_len - offset1], edit1_data.data(), edit1_length);
    n_written = io->write_data(inode_n, edit1_data.data(), offset1, edit1_length);
    EXPECT_EQ(n_written, edit1_length);

    /* Second edit */
    fsize edit2_length = block_size + 1;
    fsize offset2 = block_size * meta_inode_ptr_len;
    BufferType edit2_data(edit2_length);
    memset(edit2_data.data(), 0xBB, edit2_length);
    memcpy(&wdata[data_len - offset2], edit2_data.data(), edit2_length);
    n_written = io->write_data(inode_n, edit2_data.data(), offset2, edit2_length);
    EXPECT_EQ(n_written, edit2_length);

    /* Third edit and add */
    fsize edit3_length = block_size - 3;
    fsize offset3 = edit3_length;
    fsize add_length = block_size / 2;
    BufferType edit3_data(edit3_length + add_length);
    memset(edit3_data.data(), 0xCC, edit3_length);
    memset(&edit3_data[edit3_length], 0xFF, add_length);
    memcpy(&wdata[data_len - offset3], edit3_data.data(), edit3_length);
    wdata.insert(wdata.end(), add_length, {0xFF});
    n_written = io->write_data(inode_n, edit3_data.data(), offset3, edit3_length + add_length);
    EXPECT_EQ(n_written, edit3_length + add_length);

    inode->load(inode_n, *data_block);
    EXPECT_EQ(inode->meta().file_len, data_len + add_length);

    fsize reserved_blocks = data_block->bytes_to_blocks(data_len + add_length);
    auto wdata_it = wdata.cbegin();
    for (auto i = 0; i < reserved_blocks; i++) {
        EXPECT_TRUE(check_block(inode->ptr(i), wdata, wdata_it));
    }
}

INSTANTIATE_TEST_SUITE_P(BlockSize, MemoryIOWriteDataTest, testing::ValuesIn(valid_block_sizes));

}
