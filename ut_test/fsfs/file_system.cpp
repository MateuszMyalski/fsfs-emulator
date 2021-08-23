#include "fsfs/file_system.hpp"

#include "test_base.hpp"
using namespace FSFS;
namespace {
class FileSystemTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
   public:
    void SetUp() override {}
    void TearDown() override {}
};
TEST_P(FileSystemTest, format) {
    fsize real_disk_size = disk.get_disk_size() - 1;
    fsize n_inode_blocks = real_disk_size * 0.1;
    fsize n_data_blocks = real_disk_size - n_inode_blocks;
    ASSERT_EQ(n_inode_blocks + n_data_blocks + 1, disk.get_disk_size());

    disk.mount();
    DataBufferType r_data(block_size);
    auto n_read = disk.read(fs_offset_super_block, r_data.data(), block_size);
    ASSERT_EQ(n_read, block_size);

    super_block *MB = reinterpret_cast<super_block *>(r_data.data());
    EXPECT_EQ(MB[0].magic_number[0], meta_magic_num_lut[0]);
    EXPECT_EQ(MB[0].magic_number[1], meta_magic_num_lut[1]);
    EXPECT_EQ(MB[0].magic_number[2], meta_magic_num_lut[2]);
    EXPECT_EQ(MB[0].magic_number[3], meta_magic_num_lut[3]);
    EXPECT_EQ(MB[0].n_blocks, disk.get_disk_size());
    EXPECT_EQ(MB[0].n_data_blocks, n_data_blocks);
    EXPECT_EQ(MB[0].n_inode_blocks, n_inode_blocks);
    EXPECT_EQ(MB[0].fs_ver_major, fs_system_major);
    EXPECT_EQ(MB[0].fs_ver_minor, fs_system_minor);

    for (auto i = 0; i < n_inode_blocks; i++) {
        n_read = disk.read(inode_block_to_addr(i), r_data.data(), block_size);
        ASSERT_EQ(n_read, block_size);

        for (auto j = 0; j < block_size / meta_fragm_size; j++) {
            inode_block *inode = reinterpret_cast<inode_block *>(r_data.data());
            for (auto ptr_idx = 0; ptr_idx < meta_inode_ptr_len; ptr_idx++) {
                EXPECT_EQ(inode[j].block_ptr[ptr_idx], fs_nullptr);
            }
            EXPECT_STREQ(inode[j].file_name, inode_def_file_name);
            EXPECT_EQ(inode[j].type, block_status::Free);
            EXPECT_EQ(inode[j].indirect_inode_ptr, fs_nullptr);
        }
    }
    disk.unmount();
}

INSTANTIATE_TEST_SUITE_P(BlockSize, FileSystemTest, testing::ValuesIn(valid_block_sizes));
}