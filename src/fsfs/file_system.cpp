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

    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    // TODO:
    // Scan blocks for bitmat
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

uint32_t FileSystem::calc_mb_checksum(super_block& MB) {
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

void FileSystem::set_inode(address inode_n, const inode_block& data_block) {
    if (inode_n < 0) {
        throw std::invalid_argument("Inode number must not be negative.");
    }

    if (inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Inode number out of range.");
    }

    fsize n_meta_in_block = MB.block_size / meta_block_size;
    address inode_block_n = inode_n / n_meta_in_block;
    address nth_inode = inode_n - inode_block_n * n_meta_in_block;

    std::vector<data> inode_data(MB.block_size);
    inode_block* const inodes =
        reinterpret_cast<inode_block*>(inode_data.data());

    disk.read(inode_blocks_offset + inode_block_n, inode_data.data(),
              MB.block_size);

    std::memcpy(&inodes[nth_inode], &data_block, meta_block_size);

    auto n_written = disk.write(inode_blocks_offset + inode_block_n,
                                inode_data.data(), MB.block_size);

    if (n_written != MB.block_size) {
        throw std::runtime_error("Error while write operation.");
    }

    inode_bitmap.set_block(inode_n, static_cast<bool>(data_block.type));
}

void FileSystem::get_inode(address inode_n, inode_block& data_block) {
    if (inode_n < 0) {
        throw std::invalid_argument("Inode number must not be negative.");
    }

    if (inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Inode number out of range.");
    }

    fsize n_meta_in_block = disk.get_block_size() / meta_block_size;
    address inode_block_n = inode_n / n_meta_in_block;
    address nth_inode = inode_n - inode_block_n * n_meta_in_block;

    std::vector<data> inode_data(disk.get_block_size());
    inode_block* const inodes =
        reinterpret_cast<inode_block*>(inode_data.data());

    disk.read(inode_blocks_offset + inode_block_n, inode_data.data(),
              disk.get_block_size());

    std::memcpy(&data_block, &inodes[nth_inode], meta_block_size);
}

void FileSystem::set_data_block(address data_n, const data& data_block) {
    if (data_n < 0) {
        throw std::invalid_argument("Data block number must not be negative.");
    }

    if (data_n >= MB.n_data_blocks) {
        throw std::invalid_argument("Data block number out of range.");
    }

    address data_offset = inode_blocks_offset + MB.n_inode_blocks;

    auto n_written =
        disk.write(data_offset + data_n, &data_block, MB.block_size);

    if (n_written != MB.block_size) {
        throw std::runtime_error("Error while write operation.");
    }
}

void FileSystem::get_data_block(address data_n, data& data_block) {
    if (data_n < 0) {
        throw std::invalid_argument("Data block number must not be negative.");
    }

    if (data_n >= MB.n_data_blocks) {
        throw std::invalid_argument("Data block number out of range.");
    }

    address data_offset = inode_blocks_offset + MB.n_inode_blocks;

    disk.read(data_offset + data_n, &data_block, MB.block_size);
}
}
