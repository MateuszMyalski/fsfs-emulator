#ifndef UT_TEST_TEST_BASE_HPP
#define UT_TEST_TEST_BASE_HPP
#include <exception>
#include <vector>

#include "common/types.hpp"
#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"

namespace FSFS {

constexpr fsize block_size_quant = 1024;
const fsize valid_block_sizes[] = {block_size_quant, block_size_quant * 2, block_size_quant * 3, block_size_quant * 4};
class TestBaseBasic : public testing::WithParamInterface<fsize> {
   protected:
    fsize block_size = GetParam();
    fsize n_blocks = GetParam() * (GetParam() / block_size_quant + 1);
    fsize disk_size = block_size * n_blocks;
    constexpr static char disk_name[] = "_tmp_disk.img";

    constexpr static auto rnd_seed = 0xCAFE;

   public:
    using DataBufferType = std::vector<data>;

    template <typename T>
    void fill_dummy(T& data) {
        srand(rnd_seed);
        for (auto& el : data) {
            int rnd_data = rand();
            el = static_cast<typeof(el)>(rnd_data);
        }
    }
};

class TestBaseDisk : public TestBaseBasic {
   protected:
    Disk disk;

   public:
    TestBaseDisk() : disk(block_size) {
        std::remove(disk_name);

        Disk::create(disk_name, n_blocks, block_size);
        disk.open(disk_name);
    }

    ~TestBaseDisk() {
        EXPECT_FALSE(disk.is_mounted());

        std::remove(disk_name);
    }
};

class TestBaseFileSystem : public TestBaseDisk {
   protected:
    super_block MB;
    FileSystem file_system;

    address data_block_to_addr(address block_n) { return get_data_block_offset() + block_n; }
    address inode_block_to_addr(address block_n) { return fs_offset_inode_block + block_n; }

    address get_data_block_offset() { return fs_offset_inode_block + MB.n_inode_blocks; }

   public:
    TestBaseFileSystem() : TestBaseDisk(), file_system(disk) {
        FileSystem::format(disk);
        FileSystem::read_super_block(disk, MB);
        // TODO ...
    }
    ~TestBaseFileSystem() {}
};

class DummyBase : public testing::WithParamInterface<fsize> {
   public:
    fsize block_size = GetParam();
};

class DummyExt : public DummyBase {};
}
#endif