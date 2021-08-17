#include <fstream>
#include <string_view>

#include "test_base.hpp"
using namespace FSFS;
namespace {
class DiskTest : public ::testing::Test, public TestBaseDisk {
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

    fsize get_file_size(std::string_view file_name) {
        std::ifstream file(file_name.data(), std::ios::binary);
        file.seekg(0, std::ios::end);

        return file.tellg();
    }
};

TEST_F(DiskTest, constructor_throw) {
    EXPECT_THROW({ Disk tmp123_disk(123); }, std::invalid_argument);
    EXPECT_THROW({ Disk tmp512_disk(512); }, std::invalid_argument);
}

TEST_F(DiskTest, constructor) {
    Disk tmp1024_disk(1024);
    Disk tmp2048_disk(2048);
    Disk tmp3075_disk(3072);
    Disk tmp4096_disk(4096);
}

TEST_F(DiskTest, mount) {
    EXPECT_FALSE(disk.is_mounted());

    disk.mount();
    EXPECT_TRUE(disk.is_mounted());

    disk.unmount();
    EXPECT_FALSE(disk.is_mounted());

    disk.mount();
    disk.mount();
    disk.unmount();
    EXPECT_TRUE(disk.is_mounted());

    disk.unmount();
    EXPECT_FALSE(disk.is_mounted());
}

TEST_F(DiskTest, block_size) { EXPECT_EQ(disk.get_block_size(), block_size); }
TEST_F(DiskTest, disk_size) { EXPECT_EQ(disk.get_disk_size(), n_blocks); }

TEST_F(DiskTest, create) { EXPECT_EQ(get_file_size(disk_name), disk_size); }

TEST_F(DiskTest, open_throw) {
    Disk tmp_disk(block_size);
    tmp_disk.mount();
    EXPECT_THROW(tmp_disk.open("xxx.img"), std::runtime_error);

    tmp_disk.unmount();
    EXPECT_THROW(tmp_disk.open("xxx.img"), std::runtime_error);
}

TEST_F(DiskTest, open_invalid_size) {
    create_dummy_file("tmp_invalid_size_disk.img", block_size + 1);

    EXPECT_THROW(disk.open("tmp_invalid_size_disk.img"), std::runtime_error);
}

TEST_F(DiskTest, write) {
    constexpr fsize w_data_size = block_size * 2;
    data w_data[w_data_size] = {};
    std::fill_n(w_data, w_data_size, 0x0D);

    disk.mount();

    auto n_written = disk.write(0, w_data, w_data_size);
    EXPECT_EQ(n_written, w_data_size);

    n_written = disk.write(n_blocks - 1, w_data, w_data_size);
    EXPECT_EQ(n_written, block_size);

    n_written = disk.write(n_blocks, w_data, w_data_size);
    EXPECT_EQ(n_written, 0);

    disk.unmount();
}

TEST_F(DiskTest, write_and_read) {
    constexpr fsize r_data_size = disk_size;

    data r_data[r_data_size] = {};
    data ref_data[r_data_size] = {};
    std::fill_n(ref_data, block_size, 0x0A);
    std::fill_n(ref_data + block_size, block_size, 0x0B);

    create_dummy_file("tmp_disk.img", disk_size);
    fill_file("tmp_disk.img", 0, ref_data, r_data_size);

    disk.mount();

    auto n_written = disk.write(0, ref_data, r_data_size);
    EXPECT_EQ(n_written, r_data_size);

    auto n_read = disk.read(0, r_data, r_data_size);
    EXPECT_EQ(n_read, r_data_size);

    for (auto i = 0; i < r_data_size; i++) {
        EXPECT_EQ(ref_data[i], r_data[i]);
    }

    std::fill_n(r_data, r_data_size, 0xFF);

    n_read = disk.read(1, r_data, r_data_size);
    EXPECT_EQ(n_read, block_size * (n_blocks - 1));

    for (auto i = 0; i < n_read; i++) {
        EXPECT_EQ(ref_data[block_size + i], r_data[i]);
    }
    for (auto i = 0; i < block_size; i++) {
        EXPECT_EQ(static_cast<data>(0xFF), r_data[n_read + i]);
    }

    disk.unmount();
}
}