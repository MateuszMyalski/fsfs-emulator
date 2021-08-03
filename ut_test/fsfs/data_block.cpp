#include "fsfs/data_block.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class DataBlockTest : public ::testing::Test {
   protected:
    constexpr static fsize block_size = 1024;
    Disk* disk;
    super_block MB;
    DataBlock* data_block;

   public:
    void SetUp() override {
        disk = new Disk(block_size);
        Disk::create("tmp_disk.img", 1024 * 10, block_size);
        disk->open("tmp_disk.img");

        FileSystem::format(*disk);
        FileSystem::read_super_block(*disk, MB);

        data_block = new DataBlock(*disk, MB);
    }
    void TearDown() override {
        delete data_block;

        ASSERT_FALSE(disk->is_mounted());
        delete disk;

        std::remove("tmp_disk.img");
    }
};

TEST_F(DataBlockTest, write_throw_test) {}

TEST_F(DataBlockTest, read_throw_test) {}

TEST_F(DataBlockTest, write_test) {}

TEST_F(DataBlockTest, read_test) {}

}