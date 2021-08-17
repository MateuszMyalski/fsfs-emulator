#ifndef UT_TEST_TEST_BASE_HPP
#define UT_TEST_TEST_BASE_HPP
#include <exception>
#include <vector>

#include "common/types.hpp"
#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"
#include "gtest/gtest.h"

namespace FSFS {
class TestBaseBasic {
   protected:
    constexpr static fsize block_size = 1024;
    constexpr static fsize n_blocks = 2000;
    constexpr static fsize disk_size = block_size * n_blocks;
    constexpr static char disk_name[] = "_tmp_disk.img";

    constexpr static auto rnd_seed = 0xCAFE;

   public:
    template <typename T>
    void fill_dummy(T& data) {
        srand(rnd_seed);  // use current time as seed for random generator
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

}
#endif