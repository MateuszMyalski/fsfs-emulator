#include "file_system.hpp"

#include <algorithm>
#include <exception>

#include "data_structs.hpp"

namespace FSFS {

void FileSystem::remove(address inode_n) {
    if (!io.get_inode_bitmap().get_block_status(inode_n)) {
        throw std::runtime_error("Cannot remove unused inode.");
    }
    inode_block inode = {};
    inode.type = block_status::Free;
    io.set_inode(inode_n, inode);
}

address FileSystem::create(const char* file_name) {
    address inode_addr = io.get_inode_bitmap().next_free(0);
    if (inode_addr == -1) {
        throw std::runtime_error("No more free inodes.");
    }

    inode_block inode = {};

    inode.type = block_status::Used;
    for (auto& ptr : inode.block_ptr) {
        ptr = fs_nullptr;
    }
    inode.indirect_inode_ptr = fs_nullptr;
    inode.file_len = 0;
    std::strcpy(inode.file_name, file_name);
    io.set_inode(inode_addr, inode);

    return inode_addr;
}

void FileSystem::mount() {
    read_super_block(disk, MB);
    io.init(MB);
    disk.mount();
    io.scan_blocks();
}

void FileSystem::read_super_block(Disk& disk, super_block& MB) {
    disk.mount();

    auto n_read = disk.read(fs_offset_super_block, reinterpret_cast<data*>(&MB),
                            sizeof(super_block));

    if (n_read != sizeof(super_block)) {
        throw std::runtime_error("Cannot read super block");
    }

    for (int32_t i = 0; i < fs_data_row_size; i++) {
        if (MB.magic_number[i] != meta_magic_num_lut[i]) {
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

    disk.unmount();
}

void FileSystem::unmount() { disk.unmount(); }

void FileSystem::format(Disk& disk) {
    disk.mount();

    fsize real_disk_size = disk.get_disk_size() - 1;
    fsize meta_blocks = disk.get_block_size() / meta_fragm_size;
    fsize n_inode_blocks = real_disk_size * 0.1;
    std::vector<meta_block> block(meta_blocks);

    std::copy_n(meta_magic_num_lut, fs_data_row_size, block[0].MB.magic_number);
    block[0].MB.block_size = disk.get_block_size();
    block[0].MB.n_blocks = disk.get_disk_size();
    block[0].MB.n_inode_blocks = n_inode_blocks;
    block[0].MB.n_data_blocks = real_disk_size - n_inode_blocks;
    block[0].MB.fs_ver_major = fs_system_major;
    block[0].MB.fs_ver_minor = fs_system_minor;
    block[0].MB.checksum = calc_mb_checksum(block[0].MB);
    disk.write(fs_offset_super_block, block[0].raw_data, meta_fragm_size);

    std::fill(block.begin(), block.end(), fs_nullptr);
    for (auto i = 0; i < meta_blocks; i++) {
        block[i].inode.type = block_status::Free;
        std::memcpy(block[i].inode.file_name, inode_def_file_name,
                    meta_file_name_len);
    }
    for (auto i = 0; i < n_inode_blocks; i++) {
        disk.write(fs_offset_inode_block + i,
                   reinterpret_cast<data*>(block.data()),
                   disk.get_block_size());
    }

    disk.unmount();
}

uint32_t FileSystem::calc_mb_checksum(super_block& MB) {
    const uint8_t xor_word[] = {0xFE, 0xED, 0xC0, 0xDE};
    uint8_t* raw_data = reinterpret_cast<data*>(&MB);
    uint8_t checksum[fs_data_row_size] = {};

    fsize mb_size = meta_fragm_size - sizeof(MB.checksum);
    for (auto i = 0; i < mb_size; i++) {
        int32_t idx = i & 0x03;
        checksum[idx] ^= raw_data[i] ^ xor_word[idx];
    }

    return *reinterpret_cast<uint32_t*>(checksum);
}

}
