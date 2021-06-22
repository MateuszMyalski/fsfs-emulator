#include "disk-emulator/disk.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class DiskTest : public ::testing::Test {
   public:
    void SetUp() override{

    };
};
TEST(disk_test, constructor) {
    EXPECT_THROW({ Disk tmp123_disk(123); }, std::invalid_argument);
    EXPECT_THROW({ Disk tmp512_disk(512); }, std::invalid_argument);
    Disk tmp1024_disk(1024);
    Disk tmp2048_disk(2048);
}

TEST(disk_test, mount) {
    Disk tmp_disk(1024);
    EXPECT_FALSE(tmp_disk.is_mounted());

    tmp_disk.mount();
    EXPECT_TRUE(tmp_disk.is_mounted());

    tmp_disk.unmount();
    EXPECT_FALSE(tmp_disk.is_mounted());

    tmp_disk.mount();
    tmp_disk.mount();
    tmp_disk.unmount();
    EXPECT_TRUE(tmp_disk.is_mounted());

    tmp_disk.unmount();
    EXPECT_FALSE(tmp_disk.is_mounted());
}

TEST(disk_test, size) {
    Disk tmp_disk(1024);
    ASSERT_EQ(tmp_disk.size(), 1024);
}

TEST(disk_test, create) {
    constexpr v_size block_size = 1024;
    constexpr v_size n_blocks = 5;

    std::remove("disk_test_create.img");
    Disk::create("disk_test_create.img", block_size, n_blocks);

    std::ifstream disk_file("disk_test_create.img", std::ios::binary);
    disk_file.seekg(0, std::ios::end);
    ASSERT_EQ(block_size * n_blocks, disk_file.tellg());
}

TEST(disk_test, open) {
    constexpr v_size block_size = 1024;
    constexpr v_size n_blocks = 5;

    std::remove("disk_test_open.img");
    Disk::create("disk_test_open.img", block_size, n_blocks);

    Disk tmp_disk(block_size);
    tmp_disk.open("disk_test_open.img");
    tmp_disk.mount();
    ASSERT_THROW(tmp_disk.open("xxx.img"), std::runtime_error);

    tmp_disk.unmount();
    ASSERT_THROW(tmp_disk.open("xxx.img"), std::runtime_error);
}

TEST(disk_test, open_invalid_size) {
    constexpr v_size block_size = 1024;
    std::remove("disk_test_open_invalid_size.img");
    std::fstream invalid_disk("disk_test_open_invalid_size.img",
                              std::ios::out | std::ios::binary);

    invalid_disk.seekp(block_size + 1);
    invalid_disk.write("", 1);
    invalid_disk.close();

    Disk disk(block_size);
    ASSERT_THROW(disk.open("disk_test_open_invalid_size.img"),
                 std::runtime_error);
}

TEST(disk_test, write) {
    constexpr v_size block_size = 1024;
    constexpr v_size n_blocks = 5;

    constexpr v_size w_data_size = block_size * 2;
    data w_data[w_data_size] = {};
    std::fill_n(w_data, w_data_size, 0xD);

    std::remove("disk_test_write.img");
    Disk::create("disk_test_write.img", block_size, n_blocks);
    Disk tmp_disk(block_size);
    tmp_disk.open("disk_test_write.img");
    tmp_disk.mount();

    auto n_written = tmp_disk.write(0, w_data, w_data_size);
    ASSERT_EQ(n_written, w_data_size);

    n_written = tmp_disk.write(n_blocks - 1, w_data, w_data_size);
    ASSERT_EQ(n_written, block_size);

    tmp_disk.unmount();
}
}