#include "memory_io.hpp"

#include <algorithm>
#include <cstring>
namespace FSFS {

void MemoryIO::init(const super_block& MB) {
    std::memcpy(&this->MB, &MB, sizeof(MB));
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);
}

void MemoryIO::scan_blocks() {
    inode_block inode_data = {};
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        get_inode(inode_n, inode_data);
        if (inode_data.type != block_status::Used) {
            continue;
        }

        inode_bitmap.set_block(inode_n, 1);
        set_data_blocks_status(inode_data, 1);
    }
}

void MemoryIO::set_data_blocks_status(const inode_block& inode_data,
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

void MemoryIO::set_inode(address inode_n, const inode_block& data_block) {
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

    fsize status = disk.read(fs_offset_inode_block + inode_block_n,
                             inode_data.data(), MB.block_size);
    if (status == -1) {
        throw std::runtime_error("Error while read operation");
    }

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

void MemoryIO::get_inode(address inode_n, inode_block& data_block) {
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

void MemoryIO::set_data_block(address data_n, const data& data_block) {
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

void MemoryIO::get_data_block(address data_n, data& data_block) {
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