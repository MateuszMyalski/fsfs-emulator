#include "file_system.hpp"

#include <algorithm>
#include <exception>

#include "data_structs.hpp"

namespace FSFS {

void FileSystem::mount() {
    if (disk.is_mounted()) {
        throw std::runtime_error("Disk is already mounted.");
    }
    disk.mount();

    auto n_read = disk.read(super_block_offset, reinterpret_cast<data*>(&MB),
                            sizeof(super_block));

    if (n_read != sizeof(super_block)) {
        throw std::runtime_error("Cannot read super block");
    }

    for (int32_t i = 0; i < row_size; i++) {
        if (MB.magic_number[i] != magic_number_lut[i]) {
            throw std::runtime_error(
                "Super block corrupted. Magic number error.");
        }
    }

    if (MB.checksum != calc_mb_checksum(MB)) {
        throw std::runtime_error("Checksum error.");
    }

    if ((MB.block_size % quant_block_size) != 0) {
        throw std::runtime_error("Block size must be multiplication of 1024.");
    }
    if (MB.n_blocks <= 0) {
        throw std::runtime_error("Invalid amount of blocks number.");
    }

    block_map.initialize(MB.n_data_blocks, MB.n_inode_blocks);
}

void FileSystem::unmount() {
    disk.unmount();

    if (disk.is_mounted()) {
        throw std::runtime_error("Disk is not fully unmounted.");
    }
}

void FileSystem::format(Disk& disk) {
    disk.mount();

    fsize real_disk_size = disk.get_disk_size() - 1;
    fsize meta_blocks = disk.get_block_size() / meta_block_size;
    fsize n_inode_blocks = real_disk_size * 0.1;
    std::vector<meta_block> block = {};
    block.resize(meta_blocks);

    std::copy_n(magic_number_lut, row_size, block[0].MB.magic_number);
    block[0].MB.block_size = disk.get_block_size();
    block[0].MB.n_blocks = disk.get_disk_size();
    block[0].MB.n_inode_blocks = n_inode_blocks;
    block[0].MB.n_data_blocks = real_disk_size - n_inode_blocks;
    block[0].MB.fs_ver_major = fs_system_major;
    block[0].MB.fs_ver_minor = fs_system_minor;
    block[0].MB.checksum = calc_mb_checksum(block[0].MB);
    disk.write(super_block_offset, block[0].raw_data, meta_block_size);

    std::fill(block.begin(), block.end(), inode_empty_ptr);
    for (auto i = 0; i < meta_blocks; i++) {
        block[i].inode.type = block_status::FREE;
        std::memcpy(block[i].inode.file_name, dummy_file_name, file_name_len);
    }
    for (auto i = 0; i < n_inode_blocks; i++) {
        disk.write(inode_blocks_offset + i,
                   reinterpret_cast<data*>(block.data()),
                   disk.get_block_size());
    }

    disk.unmount();
}

uint32_t FSFS::FileSystem::calc_mb_checksum(super_block& MB) {
    const uint8_t xor_word[] = {0xFE, 0xED, 0xC0, 0xDE};
    uint8_t* raw_data = reinterpret_cast<data*>(&MB);
    uint8_t checksum[row_size] = {};

    fsize mb_size = meta_block_size - sizeof(MB.checksum);
    for (auto i = 0; i < mb_size; i++) {
        int32_t idx = i & 0x03;
        checksum[idx] ^= raw_data[i] ^ xor_word[idx];
    }

    return *reinterpret_cast<uint32_t*>(checksum);
}
}
