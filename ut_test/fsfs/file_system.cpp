#include "fsfs/file_system.hpp"

#include "fsfs/block.hpp"
#include "fsfs/block_bitmap.hpp"
#include "fsfs/inode.hpp"
#include "test_base.hpp"
using namespace FSFS;
namespace {
class FileSystemTest : public ::testing::TestWithParam<int32_t>, public TestBaseFileSystem {
   protected:
    std::unique_ptr<FileSystem> fs;
    std::unique_ptr<BlockBitmap> test_data_bitmap;

    std::vector<int32_t> used_inode_blocks;
    constexpr static const char* valid_file_name = "SampleFile";

    int32_t ref_nested_inode_n_ptrs = meta_n_direct_ptrs + (n_indirect_ptrs_in_block - 1) * 4;

   public:
    void SetUp() override {
        fs = std::make_unique<FileSystem>(disk);
        test_data_bitmap = std::make_unique<BlockBitmap>(MB.n_data_blocks);

        set_dummy_blocks();
        fs->mount();
    }

    void TearDown() override { fs->unmount(); }

    void set_dummy_blocks() {
        add_inode(*test_data_bitmap, 0, 0);
        add_inode(*test_data_bitmap, 1, 4 * block_size + 1);
        add_inode(*test_data_bitmap, 2, ref_nested_inode_n_ptrs * block_size);
    }

    bool check_block(int32_t data_n, const DataBufferType& ref_data, DataBufferType::const_iterator& ref_data_it) {
        DataBufferType rdata(block_size);
        Block block(disk, MB);
        block.read(block.data_n_to_block_n(data_n), rdata.data(), 0, block_size);

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

    void check_stored_blocks(int32_t inode_n, DataBufferType ref_data) {
        Inode inode;
        Block block(disk, MB);
        inode.load(inode_n, block);
        auto n_ptrs = block.bytes_to_blocks(inode.meta().file_len);

        for (auto i = 0; i < n_ptrs; i++) {
            ASSERT_NE(inode.ptr(i), fs_nullptr);
        }

        auto ref_data_it = ref_data.cbegin();
        for (auto i = 0; i < n_ptrs; i++) {
            EXPECT_TRUE(check_block(inode.ptr(i), ref_data, ref_data_it));
        }
    }

    void dummy_edit(int32_t inode_n, int32_t offset, int32_t length, DataBufferType& ref_data) {
        DataBufferType edit_data(length);
        fill_dummy(edit_data);
        memcpy(&ref_data[ref_data.size() - offset], edit_data.data(), length);
        auto n_written = fs->write(inode_n, edit_data.data(), offset, length);
        ASSERT_EQ(n_written, length);
    }

   private:
    void add_inode(BlockBitmap& bitmap, int32_t inode_n, int32_t dummy_length) {
        Block block(disk, MB);
        Inode inode;

        inode.alloc_new(inode_n);
        inode.meta().file_len = dummy_length;
        for (auto i = 0; i < block.bytes_to_blocks(dummy_length); i++) {
            add_data_ptr(bitmap, inode);
        }
        inode.commit(block, bitmap);

        used_inode_blocks.push_back(inode_n);
    }

    void add_data_ptr(BlockBitmap& bitmap, Inode& test_inode) {
        auto block_n = bitmap.next_free(0);
        bitmap.set_status(block_n, 1);
        test_inode.add_data(block_n);
    }
};

TEST_P(FileSystemTest, format) {
    // Calculate demanded values
    //
    int32_t real_disk_size = disk.get_disk_size() - 1;
    int32_t n_inode_blocks = real_disk_size * 0.1;
    int32_t n_data_blocks = real_disk_size - n_inode_blocks;
    ASSERT_EQ(n_inode_blocks + n_data_blocks + 1, disk.get_disk_size());

    FileSystem::format(disk);

    // Super block check
    //
    disk.mount();
    DataBufferType r_data(block_size);
    auto n_read = disk.read(fs_offset_super_block, r_data.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    super_block* rMB = reinterpret_cast<super_block*>(r_data.data());
    EXPECT_EQ(rMB[0].magic_number[0], meta_magic_seq_lut[0]);
    EXPECT_EQ(rMB[0].magic_number[1], meta_magic_seq_lut[1]);
    EXPECT_EQ(rMB[0].magic_number[2], meta_magic_seq_lut[2]);
    EXPECT_EQ(rMB[0].magic_number[3], meta_magic_seq_lut[3]);
    EXPECT_EQ(rMB[0].n_blocks, disk.get_disk_size());
    EXPECT_EQ(rMB[0].n_data_blocks, n_data_blocks);
    EXPECT_EQ(rMB[0].n_inode_blocks, n_inode_blocks);
    EXPECT_EQ(rMB[0].fs_ver_major, fs_system_major);
    EXPECT_EQ(rMB[0].fs_ver_minor, fs_system_minor);
    disk.unmount();

    // Inode check
    //
    Block block(disk, MB);
    Inode inode;
    for (auto i = 0; i < n_inode_blocks; i++) {
        inode.load(i, block);
        EXPECT_EQ(inode.meta().status, block_status::Free);
        EXPECT_EQ(inode.meta().file_len, 0);
    }
}

TEST_P(FileSystemTest, write_invalid_inode) {
    int32_t invalid_inode = MB.n_inode_blocks - 1;
    DataBufferType wdata(1);

    EXPECT_EQ(fs->write(invalid_inode, wdata.data(), 0, 1), fs_nullptr);
}

TEST_P(FileSystemTest, write_zero_size_data) {
    DataBufferType wdata(1);
    int32_t inode_n = fs->create_file(valid_file_name);

    EXPECT_EQ(fs->write(inode_n, wdata.data(), 0, 0), 0);
}

TEST_P(FileSystemTest, write_even_blocks) {
    int32_t data_len = block_size * 2;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_uneven_direct_blocks) {
    int32_t data_len = block_size * 2 + 1;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_single_indirect_blocks) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 1;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_append_data_uneven) {
    int32_t data_len = block_size * 2 + 1;
    int32_t total_len = data_len + data_len;
    DataBufferType ref_data(total_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    n_written += fs->write(inode_n, ref_data.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_nested_indirect_blocks) {
    Block block(disk, MB);
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size * (block.get_n_addreses_in_block() - 1);
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    EXPECT_EQ(n_written, data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_append_data_even) {
    int32_t data_len = block_size;
    int32_t total_len = data_len + data_len;
    DataBufferType ref_data(total_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    int32_t n_written = fs->write(inode_n, ref_data.data(), 0, data_len);
    n_written += fs->write(inode_n, ref_data.data() + data_len, 0, data_len);
    EXPECT_EQ(n_written, total_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_with_offset_greater_than_edit_length) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    constexpr auto edit_data_len = 30;
    constexpr auto offset = 20;
    dummy_edit(inode_n, edit_data_len, offset, ref_data);
    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_with_offset_same_as_edit_length) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    constexpr auto edit_data_len = 30;
    constexpr auto offset = edit_data_len;
    dummy_edit(inode_n, edit_data_len, offset, ref_data);
    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, write_with_offset_and_overflown_length) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    constexpr auto additional_data_len = 30;
    constexpr auto edit_data_len = 15;
    constexpr auto offset = edit_data_len;

    DataBufferType edit_data(edit_data_len);
    fill_dummy(edit_data);
    memcpy(&ref_data[ref_data.size() - offset], edit_data.data(), edit_data_len);
    ref_data.insert(ref_data.end(), additional_data_len, {0xFF});
    edit_data.insert(edit_data.end(), additional_data_len, {0xFF});
    auto n_written = fs->write(inode_n, edit_data.data(), offset, edit_data_len + additional_data_len);
    ASSERT_EQ(n_written, edit_data_len + additional_data_len);

    check_stored_blocks(inode_n, ref_data);
}

TEST_P(FileSystemTest, scan_blocks) {
    for (const auto inode_n : used_inode_blocks) {
        EXPECT_TRUE(fs->get_inode_bitmap().get_status(inode_n));
    }
    for (auto inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (std::find(used_inode_blocks.begin(), used_inode_blocks.end(), inode_n) != used_inode_blocks.end()) {
            continue;
        }
        EXPECT_FALSE(fs->get_inode_bitmap().get_status(inode_n));
    }

    for (auto i = 0; i < MB.n_data_blocks; i++) {
        EXPECT_EQ(fs->get_data_bitmap().get_status(i), test_data_bitmap->get_status(i));
    }
}

TEST_P(FileSystemTest, alloc_inode_too_long_name) {
    constexpr const char* too_long_name = "TOO LONG FILE NAME 012345678910 ABCDE";
    ASSERT_EQ(strnlen(too_long_name, meta_max_file_name_size), meta_max_file_name_size);
    EXPECT_EQ(fs->create_file(too_long_name), fs_nullptr);
}

TEST_P(FileSystemTest, alloc_inode_valid_data) {
    constexpr const char* inode_name = "test_inode name";
    ASSERT_LT(strnlen(inode_name, meta_max_file_name_size), meta_max_file_name_size);
    auto inode_n = fs->create_file(inode_name);
    EXPECT_TRUE(fs->get_inode_bitmap().get_status(inode_n));
}

TEST_P(FileSystemTest, dealloc_inode_invalid_inode) {
    int32_t invalid_inode_n = MB.n_inode_blocks - 1;
    EXPECT_EQ(fs->remove_file(invalid_inode_n), fs_nullptr);
}

TEST_P(FileSystemTest, dealloc_inode) {
    constexpr int32_t inode_to_dealloc = 2;
    EXPECT_EQ(fs->remove_file(inode_to_dealloc), inode_to_dealloc);
    EXPECT_FALSE(fs->get_inode_bitmap().get_status(inode_to_dealloc));

    auto n_dealocated_blocks = 0;
    for (auto i = 0; i < MB.n_data_blocks; i++) {
        if (fs->get_data_bitmap().get_status(i) != test_data_bitmap->get_status(i)) {
            n_dealocated_blocks++;
        }
    }
    constexpr int32_t n_indirect_data_blocks = 5;
    EXPECT_EQ(n_dealocated_blocks, ref_nested_inode_n_ptrs + n_indirect_data_blocks);
}

TEST_P(FileSystemTest, rename_inode) {
    constexpr const char* file_name_ref = "test_inode name_ref";
    int32_t addr = fs->create_file(file_name_ref);

    char file_name[meta_max_file_name_size] = {};
    fs->get_file_name(addr, file_name);
    EXPECT_STREQ(file_name, file_name_ref);

    constexpr const char* file_name_ref2 = "New test_inode name";
    fs->rename_file(addr, file_name_ref2);
    file_name[0] = '\0';
    fs->get_file_name(addr, file_name);
    EXPECT_STREQ(file_name, file_name_ref2);
}

TEST_P(FileSystemTest, read_invalid_inode) {
    DataBufferType rdata(1);
    int32_t invalid_inode = MB.n_inode_blocks - 1;
    EXPECT_EQ(fs->read(invalid_inode, rdata.data(), 0, 1), fs_nullptr);
}

TEST_P(FileSystemTest, read_full_file) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    int32_t n_read = fs->read(inode_n, rdata.data(), 0, data_len);
    EXPECT_EQ(n_read, data_len);

    EXPECT_TRUE(cmp_data(rdata, ref_data));
}

TEST_P(FileSystemTest, read_less_than_block) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    auto to_read = block_size / 2;
    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, to_read);

    int32_t n_read = fs->read(inode_n, rdata.data(), 0, to_read);
    EXPECT_EQ(n_read, to_read);

    EXPECT_TRUE(cmp_data(rdata.data(), ref_data.data(), n_read));
}

TEST_P(FileSystemTest, read_offset_within_one_block) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    auto offset = block_size / 4;
    int32_t n_read = fs->read(inode_n, rdata.data(), offset, block_size - offset);
    EXPECT_EQ(n_read, block_size - offset);

    EXPECT_TRUE(cmp_data(rdata.data(), &ref_data[offset], n_read));
}

TEST_P(FileSystemTest, read_offset_and_rest_of_the_file) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    auto offset = block_size / 4;
    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    int32_t n_read = fs->read(inode_n, rdata.data(), offset, data_len);
    EXPECT_EQ(n_read, data_len - offset);

    EXPECT_TRUE(cmp_data(rdata.data(), &ref_data[offset], n_read));
}

TEST_P(FileSystemTest, read_offset_length_greater_than_file_length) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    auto offset = block_size / 4 + block_size;
    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    int32_t n_read = fs->read(inode_n, rdata.data(), offset, data_len);
    EXPECT_EQ(n_read, data_len - offset);

    EXPECT_TRUE(cmp_data(rdata.data(), &ref_data[offset], n_read));
}

TEST_P(FileSystemTest, read_offset_greater_than_length) {
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len);
    fill_dummy(ref_data);

    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    int32_t n_read = fs->read(inode_n, rdata.data(), data_len - 1, data_len);
    EXPECT_EQ(n_read, 1);

    EXPECT_TRUE(cmp_data(rdata.data(), &ref_data[data_len - 1], n_read));
}

TEST_P(FileSystemTest, read_buffer_content_consistency) {
    constexpr auto guard_value = static_cast<DataBufferType::value_type>(0xDEAD);
    int32_t data_len = block_size * meta_n_direct_ptrs + 2 * block_size;
    DataBufferType ref_data(data_len);
    DataBufferType rdata(data_len, guard_value);
    fill_dummy(ref_data);

    auto data_to_read = data_len / 2;
    int32_t inode_n = fs->create_file(valid_file_name);
    fs->write(inode_n, ref_data.data(), 0, data_len);

    int32_t n_read = fs->read(inode_n, rdata.data(), 0, data_to_read);
    EXPECT_EQ(n_read, data_to_read);

    EXPECT_TRUE(cmp_data(rdata.data(), ref_data.data(), n_read));
    for (auto i = data_to_read; i < data_len; i++) {
        EXPECT_EQ(rdata[i], guard_value);
    }
}

INSTANTIATE_TEST_SUITE_P(BlockSize, FileSystemTest, testing::ValuesIn(valid_block_sizes));
}
