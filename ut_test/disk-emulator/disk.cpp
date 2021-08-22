#include <fstream>
#include <string_view>

#include "test_base.hpp"
using namespace FSFS;
namespace {
class DiskTest : public ::testing::TestWithParam<fsize>, public TestBaseDisk {
   private:
    std::vector<std::string_view> created_files;

   protected:
   public:
    void SetUp() override {}

    void TearDown() override {
        for (auto& file_name : created_files) {
            std::remove(file_name.data());
        }
    }

    void create_dummy_file(std::string_view file_name, int32_t size) {
        std::remove(file_name.data());
        std::fstream file(file_name.data(), std::ios::out | std::ios::binary);

        file.seekp(size + 1);
        file.write("", 1);
        file.close();

        created_files.push_back(file_name);
    }

    void fill_file(std::string_view file_name, int32_t offset, data* data, int32_t len) {
        std::fstream file(file_name.data(), std::ios::out | std::ios::binary);
        file.seekp(offset);
        file.write(reinterpret_cast<char*>(data), len);
        file.close();
    }
};

TEST(DiskTest, constructor_throw) {
    EXPECT_THROW({ Disk tmp_disk(-1); }, std::invalid_argument);
    EXPECT_THROW({ Disk tmp_disk(block_size_quant - 1); }, std::invalid_argument);
    EXPECT_THROW({ Disk tmp_disk(block_size_quant + 1); }, std::invalid_argument);
}

TEST(DiskTest, constructor_valid_disk_sizes) {
    for (auto disk_size : valid_block_sizes) {
        EXPECT_NO_THROW({ Disk tmp_disk(disk_size); });
    }
}

TEST(DiskTest, mount_is_not_mounted_by_default) {
    Disk disk(block_size_quant);
    EXPECT_FALSE(disk.is_mounted());
}

TEST(DiskTest, mount_and_unmount) {
    Disk disk(block_size_quant);
    disk.mount();
    EXPECT_TRUE(disk.is_mounted());

    disk.unmount();
    EXPECT_FALSE(disk.is_mounted());
}

TEST(DiskTest, mount_multiple_times) {
    Disk disk(block_size_quant);

    disk.mount();
    disk.mount();
    disk.unmount();
    EXPECT_TRUE(disk.is_mounted());

    disk.unmount();
    EXPECT_FALSE(disk.is_mounted());
}

TEST_P(DiskTest, get_block_size) { EXPECT_EQ(disk.get_block_size(), block_size); }

TEST_P(DiskTest, get_disk_size) { EXPECT_EQ(disk.get_disk_size(), n_blocks); }

TEST_P(DiskTest, create_requested_disk_size_test) {
    std::ifstream file(disk_name, std::ios::binary);
    file.seekg(0, std::ios::end);

    auto real_disk_size = file.tellg();
    EXPECT_EQ(real_disk_size, disk_size);
}

TEST_P(DiskTest, open_throw_when_mounted_already) {
    Disk tmp_disk(block_size);
    tmp_disk.mount();
    EXPECT_THROW(tmp_disk.open(disk_name), std::runtime_error);
}

TEST_P(DiskTest, open_throw_when_invalid_img_name) {
    Disk tmp_disk(block_size);

    EXPECT_THROW(tmp_disk.open("xxx.img"), std::runtime_error);
}

TEST_P(DiskTest, open_throw_invalid_size) {
    create_dummy_file("tmp_invalid_size_disk.img", block_size_quant + 1);

    EXPECT_THROW(disk.open("tmp_invalid_size_disk.img"), std::runtime_error);
}

TEST_P(DiskTest, write_first_two_block) {
    auto w_data_size = block_size * 2;
    DataBufferType w_data(w_data_size);
    fill_dummy(w_data);

    disk.mount();
    auto n_written = disk.write(0, w_data.data(), w_data_size);
    EXPECT_EQ(n_written, w_data_size);
    disk.unmount();
}

TEST_P(DiskTest, write_last_block_with_overflow) {
    auto w_data_size = block_size * 2;
    DataBufferType w_data(w_data_size);
    fill_dummy(w_data);

    disk.mount();
    auto n_written = disk.write(n_blocks - 1, w_data.data(), w_data_size);
    EXPECT_EQ(n_written, block_size);
    disk.unmount();
}

TEST_P(DiskTest, write_invalid_block) {
    auto w_data_size = block_size * 2;
    DataBufferType w_data(w_data_size);
    fill_dummy(w_data);

    disk.mount();
    auto n_written = disk.write(n_blocks, w_data.data(), w_data_size);
    EXPECT_EQ(n_written, 0);
    disk.unmount();
}

TEST_P(DiskTest, write_and_read_full_disk_size) {
    auto tmp_disk_size = block_size * 5;
    DataBufferType r_data(tmp_disk_size);
    DataBufferType ref_data(tmp_disk_size);
    fill_dummy(ref_data);

    constexpr char ut_tmp_disk_name[] = "__tmp_disk.img";
    create_dummy_file(ut_tmp_disk_name, disk_size);
    fill_file(ut_tmp_disk_name, 0, ref_data.data(), tmp_disk_size);

    disk.open(ut_tmp_disk_name);
    disk.mount();
    auto n_written = disk.write(0, ref_data.data(), tmp_disk_size);
    ASSERT_EQ(n_written, tmp_disk_size);

    auto n_read = disk.read(0, r_data.data(), tmp_disk_size);
    ASSERT_EQ(n_read, tmp_disk_size);
    disk.unmount();

    EXPECT_TRUE(cmp_data(ref_data, r_data));
}

TEST_P(DiskTest, write_and_read_test_unchanged_other_blocks) {
    auto tmp_disk_size = block_size * 5;
    DataBufferType r_data(tmp_disk_size);
    DataBufferType ref_data(tmp_disk_size);
    fill_dummy(ref_data);

    constexpr char ut_tmp_disk_name[] = "__tmp_disk.img";
    create_dummy_file(ut_tmp_disk_name, disk_size);
    fill_file(ut_tmp_disk_name, 0, ref_data.data(), tmp_disk_size);

    /* Guard value should remain unchanged after not full read */
    constexpr data guard_value = 0xFF;
    memset(&r_data[tmp_disk_size - block_size], guard_value, block_size);

    disk.open(ut_tmp_disk_name);
    disk.mount();

    auto n_read = disk.read(1, r_data.data(), tmp_disk_size);
    ASSERT_EQ(n_read, tmp_disk_size - block_size);
    disk.unmount();

    for (auto i = 0; i < n_read; i++) {
        ASSERT_EQ(ref_data[block_size + i], r_data[i]);
    }
    for (auto i = 0; i < block_size; i++) {
        ASSERT_EQ(guard_value, r_data[n_read + i]);
    }
}

INSTANTIATE_TEST_SUITE_P(BlockSize, DiskTest, testing::ValuesIn(valid_block_sizes));

}