#include "disk-emulator/disk.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string_view>

#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class DiskTest : public ::testing::Test {
   private:
    std::vector<std::string_view> created_files;

   protected:
    constexpr static bool clean_test_files = true;
    constexpr static v_size block_size = 1024;
    constexpr static v_size n_blocks = 5;
    constexpr static char disk_name[] = "tmp_disk.img";

    Disk* test_disk;

   public:
    void SetUp() override { test_disk = new Disk(block_size); }
    void TearDown() override {
        if constexpr (clean_test_files) {
            clean_up_test_files();
        }

        std::remove(disk_name);
        delete test_disk;
    }

    void recreate_test_file(std::string_view file_name, int32_t size) {
        std::remove(file_name.data());
        std::fstream file(file_name.data(), std::ios::out | std::ios::binary);

        file.seekp(size + 1);
        file.write("", 1);
        file.close();

        created_files.push_back(file_name);
    }

    void clean_up_test_files() {
        for (auto& file_name : created_files) {
            std::remove(file_name.data());
        }
    }

    v_size get_file_size(std::string_view file_name) {
        std::ifstream file(file_name.data(), std::ios::binary);
        file.seekg(0, std::ios::end);

        return file.tellg();
    }
};
TEST(disk_test, constructor) {
    EXPECT_THROW({ Disk tmp123_disk(123); }, std::invalid_argument);
    EXPECT_THROW({ Disk tmp512_disk(512); }, std::invalid_argument);
    Disk tmp1024_disk(1024);
    Disk tmp2048_disk(2048);
}

TEST_F(DiskTest, mount) {
    EXPECT_FALSE(test_disk->is_mounted());

    test_disk->mount();
    EXPECT_TRUE(test_disk->is_mounted());

    test_disk->unmount();
    EXPECT_FALSE(test_disk->is_mounted());

    test_disk->mount();
    test_disk->mount();
    test_disk->unmount();
    EXPECT_TRUE(test_disk->is_mounted());

    test_disk->unmount();
    EXPECT_FALSE(test_disk->is_mounted());
}

TEST_F(DiskTest, size) { ASSERT_EQ(test_disk->size(), 1024); }

TEST_F(DiskTest, create) {
    Disk::create(disk_name, block_size, n_blocks);

    ASSERT_EQ(block_size * n_blocks, get_file_size(disk_name));
}

TEST_F(DiskTest, open) {
    Disk::create(disk_name, block_size, n_blocks);

    test_disk->open(disk_name);
    test_disk->mount();
    ASSERT_THROW(test_disk->open("xxx.img"), std::runtime_error);

    test_disk->unmount();
    ASSERT_THROW(test_disk->open("xxx.img"), std::runtime_error);
}

TEST_F(DiskTest, open_invalid_size) {
    recreate_test_file("tmp_invalid_size_disk.img", block_size + 1);

    ASSERT_THROW(test_disk->open("tmp_invalid_size_disk.img"),
                 std::runtime_error);
}

TEST_F(DiskTest, write) {
    Disk::create(disk_name, block_size, n_blocks);

    constexpr v_size w_data_size = block_size * 2;
    data w_data[w_data_size] = {};
    std::fill_n(w_data, w_data_size, 0xD);

    test_disk->open(disk_name);
    test_disk->mount();

    auto n_written = test_disk->write(0, w_data, w_data_size);
    ASSERT_EQ(n_written, w_data_size);

    n_written = test_disk->write(n_blocks - 1, w_data, w_data_size);
    ASSERT_EQ(n_written, block_size);

    test_disk->unmount();
}
}