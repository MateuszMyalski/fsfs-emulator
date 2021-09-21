#ifndef UT_TEST_TEST_BASE_HPP
#define UT_TEST_TEST_BASE_HPP
#include <cstring>
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

    bool cmp_data(DataBufferType& data_lhs, DataBufferType& data_rhs) {
        return std::memcmp(data_lhs.data(), data_rhs.data(), data_lhs.size()) == 0;
    }

    template <typename T>
    bool cmp_data(const T* data_lhs, const T* data_rhs, size_t length) {
        return std::memcmp(data_lhs, data_rhs, length) == 0;
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
    const fsize n_meta_blocks_in_block = block_size / meta_fragm_size_bytes;
    const fsize n_indirect_ptrs_in_block = block_size / sizeof(address);

   public:
    TestBaseFileSystem() : TestBaseDisk(), file_system(disk) {
        FileSystem::format(disk);
        disk.read(fs_offset_super_block, cast_to_data(&MB), sizeof(super_block));
    }
    ~TestBaseFileSystem() {}
};
}
#endif
