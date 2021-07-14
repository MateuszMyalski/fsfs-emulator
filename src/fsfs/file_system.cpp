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

    scan_blocks();
}

void FileSystem::scan_blocks() {
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    inode_block inode_data = {};
    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        get_inode(inode_n, inode_data);
        if (inode_data.type != block_status::Used) {
            continue;
        }

        inode_bitmap.set_block(inode_n, 1);
        set_data_blocks_status(inode_data, 1);
    }
}

void FileSystem::set_data_blocks_status(const inode_block& inode_data,
                                        bool allocated) {
    for (fsize ptr_n = 0; ptr_n < meta_inode_ptr_len; ptr_n++) {
        if (inode_data.block_ptr[ptr_n] == fs_nullptr) {
            break;
        }
        data_bitmap.set_block(inode_data.block_ptr[ptr_n], allocated);
    }

    address indirect_block = inode_data.indirect_inode_ptr;
    while (indirect_block != fs_nullptr) {
        data_bitmap.set_block(indirect_block, allocated);
        std::vector<data> data(MB.block_size, 0x0);
        get_data_block(indirect_block, *data.data());
        fsize n_addr = MB.block_size / sizeof(address);
        address* addr = reinterpret_cast<address*>(data.data());

        for (auto idx = 0; idx < n_addr - 1; idx++) {
            if (addr[idx] == fs_nullptr) {
                indirect_block = fs_nullptr;
                break;
            }
            data_bitmap.set_block(addr[idx], allocated);
        }
        indirect_block = addr[n_addr - 1];
    }
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

void FileSystem::set_inode(address inode_n, const inode_block& data_block) {
    if (inode_n < 0) {
        throw std::invalid_argument("Inode number must not be negative.");
    }

    if (inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Inode number out of range.");
    }

    fsize n_meta_in_block = MB.block_size / meta_fragm_size;
    address inode_block_n = inode_n / n_meta_in_block;
    address nth_inode = inode_n - inode_block_n * n_meta_in_block;

    std::vector<data> inode_data(MB.block_size);
    inode_block* const inodes =
        reinterpret_cast<inode_block*>(inode_data.data());

    disk.read(fs_offset_inode_block + inode_block_n, inode_data.data(),
              MB.block_size);

    set_data_blocks_status(inodes[nth_inode], 0);

    if (data_block.type == block_status::Free) {
        inode_bitmap.set_block(inode_n, 0);
    } else {
        inode_bitmap.set_block(inode_n, 1);
        set_data_blocks_status(data_block, 1);
    }

    std::memcpy(&inodes[nth_inode], &data_block, meta_fragm_size);

    auto n_written = disk.write(fs_offset_inode_block + inode_block_n,
                                inode_data.data(), MB.block_size);

    if (n_written != MB.block_size) {
        throw std::runtime_error("Error while write operation.");
    }
}

void FileSystem::get_inode(address inode_n, inode_block& data_block) {
    if (inode_n < 0) {
        throw std::invalid_argument("Inode number must not be negative.");
    }

    if (inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Inode number out of range.");
    }

    fsize n_meta_in_block = disk.get_block_size() / meta_fragm_size;
    address inode_block_n = inode_n / n_meta_in_block;
    address nth_inode = inode_n - inode_block_n * n_meta_in_block;

    std::vector<data> inode_data(disk.get_block_size());
    inode_block* const inodes =
        reinterpret_cast<inode_block*>(inode_data.data());

    disk.read(fs_offset_inode_block + inode_block_n, inode_data.data(),
              disk.get_block_size());

    std::memcpy(&data_block, &inodes[nth_inode], meta_fragm_size);
}

void FileSystem::set_data_block(address data_n, const data& data_block) {
    if (data_n < 0) {
        throw std::invalid_argument("Data block number must not be negative.");
    }

    if (data_n >= MB.n_data_blocks) {
        throw std::invalid_argument("Data block number out of range.");
    }

    address data_offset = fs_offset_inode_block + MB.n_inode_blocks;

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

    address data_offset = fs_offset_inode_block + MB.n_inode_blocks;

    disk.read(data_offset + data_n, &data_block, MB.block_size);
}
}
