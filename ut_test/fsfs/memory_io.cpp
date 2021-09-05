#include "fsfs/memory_io.hpp"

#include "fsfs/block.hpp"
#include "fsfs/block_bitmap.hpp"
#include "fsfs/inode.hpp"
#include "test_base.hpp"
using namespace FSFS;
namespace {
class MemoryIOTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    std::unique_ptr<MemoryIO> io;
    std::unique_ptr<BlockBitmap> data_bitmap;
    std::unique_ptr<Block> block;
    std::unique_ptr<Inode> inode;

    std::vector<address> used_inode_blocks;
    constexpr static const char* valid_file_name = "SampleFile";

    fsize ref_nested_inode_n_ptrs = meta_inode_ptr_len + (n_indirect_ptrs_in_block - 1) * 4;

   public:
    void SetUp() override {
        io = std::make_unique<MemoryIO>(disk);
        data_bitmap = std::make_unique<BlockBitmap>(MB.n_data_blocks);
        block = std::make_unique<Block>(disk, MB);
        inode = std::make_unique<Inode>();

        set_dummy_blocks();
        io->resize(MB);
        io->scan_blocks();
    }

    void set_dummy_blocks() {
        add_inode(*data_bitmap, 0, 0);
        add_inode(*data_bitmap, 1, 4 * block_size + 1);
        add_inode(*data_bitmap, 2, ref_nested_inode_n_ptrs * block_size);
    }

    bool check_block(address data_n, const DataBufferType& ref_data, DataBufferType::const_iterator& ref_data_it) {
        DataBufferType rdata(block_size);
        block->read(block->data_n_to_block_n(data_n), rdata.data(), 0, block_size);

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

    void check_stored_blocks(address inode_n, DataBufferType ref_data) {
        inode->load(inode_n, *block);
        auto n_ptrs = block->bytes_to_blocks(inode->meta().file_len);

        for (auto i = 0; i < n_ptrs; i++) {
            ASSERT_NE(inode->ptr(i), fs_nullptr);
        }

        auto ref_data_it = ref_data.cbegin();
        for (auto i = 0; i < n_ptrs; i++) {
            EXPECT_TRUE(check_block(inode->ptr(i), ref_data, ref_data_it));
        }
    }

    void dummy_edit(address inode_n, fsize offset, fsize length, DataBufferType& ref_data) {
        DataBufferType edit_data(length);
        fill_dummy(edit_data);
        memcpy(&ref_data[ref_data.size() - offset], edit_data.data(), length);
        auto n_written = io->write_data(inode_n, edit_data.data(), offset, length);
        ASSERT_EQ(n_written, length);
    }

   private:
    void add_inode(BlockBitmap& bitmap, address inode_n, fsize dummy_length) {
        Block data_block(disk, MB);
        Inode inode;

        inode.alloc_new(inode_n);
        inode.meta().file_len = dummy_length;
        for (auto i = 0; i < data_block.bytes_to_blocks(dummy_length); i++) {
            add_data_ptr(bitmap, inode);
        }
        inode.commit(data_block, bitmap);

        used_inode_blocks.push_back(inode_n);
    }

    void add_data_ptr(BlockBitmap& bitmap, Inode& inode) {
        auto block_n = bitmap.next_free(0);
        bitmap.set_status(block_n, 1);
        inode.add_data(block_n);
    }
};

TEST_P(MemoryIOTest, write_invalid_inode) {
    address invalid_inode = MB.n_inode_blocks - 1;
    DataBufferType wdata(1);

    EXPECT_EQ(io->write_data(invalid_inode, wdata.data(), 0, 1), fs_nullptr);
}

TEST_P(MemoryIOTest, write_zero_size_data) {
    DataBufferType wdata(1);
    address inode_n = io->alloc_inode(valid_file_name);

    EXPECT_EQ(io->write_data(inode_n, wdata.data(), 0, 0), 0);
}

TEST_P(MemoryIOTest, write_even_blocks) {
    fsize data_len = block_size * 2;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_uneven_direct_blocks) {
    fsize data_len = block_size * 2 + 1;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_single_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 1;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_append_data_uneven) {
    fsize data_len = block_size * 2 + 1;
    fsize total_len = data_len + data_len;
    DataBufferType ref_data(total_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    n_written += io->write_data(inode_n, ref_data.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_nested_indirect_blocks) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size * (block->get_n_addreses_in_block() - 1);
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_append_data_even) {
    fsize data_len = block_size;
    fsize total_len = data_len + data_len;
    DataBufferType ref_data(total_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    fsize n_written = io->write_data(inode_n, ref_data.data(), 0, data_len);
    n_written += io->write_data(inode_n, ref_data.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_with_offset_greater_than_edit_length) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    io->write_data(inode_n, ref_data.data(), 0, data_len);

    constexpr auto edit_data_len = 30;
    constexpr auto offset = 20;
    dummy_edit(inode_n, edit_data_len, offset, ref_data);
    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_with_offset_same_as_edit_length) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    io->write_data(inode_n, ref_data.data(), 0, data_len);

    constexpr auto edit_data_len = 30;
    constexpr auto offset = edit_data_len;
    dummy_edit(inode_n, edit_data_len, offset, ref_data);
    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, write_with_offset_and_overflown_length) {
    fsize data_len = block_size * meta_inode_ptr_len + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    address inode_n = io->alloc_inode(valid_file_name);
    io->write_data(inode_n, ref_data.data(), 0, data_len);

    constexpr auto additional_data_len = 30;
    constexpr auto edit_data_len = 15;
    constexpr auto offset = edit_data_len;

    DataBufferType edit_data(edit_data_len);
    fill_dummy(edit_data);
    memcpy(&ref_data[ref_data.size() - offset], edit_data.data(), edit_data_len);
    ref_data.insert(ref_data.end(), additional_data_len, {0xFF});
    edit_data.insert(edit_data.end(), additional_data_len, {0xFF});
    auto n_written = io->write_data(inode_n, edit_data.data(), offset, edit_data_len + additional_data_len);
    ASSERT_EQ(n_written, edit_data_len + additional_data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(MemoryIOTest, scan_blocks) {
    for (const auto inode_n : used_inode_blocks) {
        EXPECT_TRUE(io->get_inode_bitmap().get_status(inode_n));
    }
    for (auto inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (std::find(used_inode_blocks.begin(), used_inode_blocks.end(), inode_n) != used_inode_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(io->get_inode_bitmap().get_status(inode_n));
    }

    for (auto i = 0; i < MB.n_data_blocks; i++) {
        EXPECT_EQ(io->get_data_bitmap().get_status(i), data_bitmap->get_status(i));
    }
}

TEST_P(MemoryIOTest, alloc_inode_too_long_name) {
    constexpr const char* too_long_name = "TOO LONG FILE NAME 012345678910 ABCDE";
    ASSERT_EQ(strnlen(too_long_name, meta_file_name_len), meta_file_name_len);
    EXPECT_EQ(io->alloc_inode(too_long_name), fs_nullptr);
}

TEST_P(MemoryIOTest, alloc_inode_valid_data) {
    constexpr const char* inode_name = "Inode name";
    ASSERT_LT(strnlen(inode_name, meta_file_name_len), meta_file_name_len);
    auto inode_n = io->alloc_inode(inode_name);
    EXPECT_TRUE(io->get_inode_bitmap().get_status(inode_n));
}

TEST_P(MemoryIOTest, dealloc_inode_invalid_inode) {
    address invalid_inode_n = MB.n_inode_blocks - 1;
    EXPECT_EQ(io->dealloc_inode(invalid_inode_n), fs_nullptr);
}

TEST_P(MemoryIOTest, dealloc_inode) {
    constexpr address inode_to_dealloc = 2;
    EXPECT_EQ(io->dealloc_inode(inode_to_dealloc), inode_to_dealloc);
    EXPECT_FALSE(io->get_inode_bitmap().get_status(inode_to_dealloc));

    auto n_dealocated_blocks = 0;
    for (auto i = 0; i < MB.n_data_blocks; i++) {
        if (io->get_data_bitmap().get_status(i) != data_bitmap->get_status(i)) {
            n_dealocated_blocks++;
        }
    }
    constexpr fsize n_indirect_data_blocks = 5;
    EXPECT_EQ(n_dealocated_blocks, ref_nested_inode_n_ptrs + n_indirect_data_blocks);
}

TEST_P(MemoryIOTest, rename_inode) {
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

INSTANTIATE_TEST_SUITE_P(BlockSize, MemoryIOTest, testing::ValuesIn(valid_block_sizes));
}
