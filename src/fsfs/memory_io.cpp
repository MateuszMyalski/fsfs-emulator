#include "memory_io.hpp"

#include <algorithm>
#include <cstring>
namespace FSFS {

address MemoryIO::set_nth_ptr(address inode_n, address ptr_n,
                              address data_ptr) {
    if ((inode_n < 0) || (ptr_n < 0) || (data_ptr < 0)) {
        throw std::invalid_argument("Values cannot be negative");
    }

    if (!inode_bitmap.get_block_status(inode_n)) {
        throw std::runtime_error("Inode is not allocated.");
    }

    inode_block inode = {};
    get_inode(inode_n, inode);
    if (ptr_n < meta_inode_ptr_len) {
        inode.block_ptr[ptr_n] = data_ptr;
        set_inode(inode_n, inode);
        return data_ptr;
    }

    fsize n_ptr_in_indirect = (MB.block_size / sizeof(address) - 1);
    int32_t nth_indirect = (ptr_n - meta_inode_ptr_len) / n_ptr_in_indirect;
    std::vector<data> rwdata(MB.block_size);
    address* indirect_addr = reinterpret_cast<address*>(rwdata.data());

    address indir_node = inode.indirect_inode_ptr;
    address curr_addr = data_bitmap.next_free(0);
    if (inode.indirect_inode_ptr == fs_nullptr) {
        inode.indirect_inode_ptr = curr_addr;
    }

    int32_t i = 0;
    do {
        if (indir_node == fs_nullptr) {
            std::fill_n(indirect_addr, n_ptr_in_indirect + 1, fs_nullptr);
            if ((i + 1) < nth_indirect) {
                indirect_addr[n_ptr_in_indirect + 1] =
                    data_bitmap.next_free(curr_addr + 1);
            }
            set_data_block(curr_addr, *rwdata.data());
            curr_addr = indirect_addr[n_ptr_in_indirect + 1];
        } else {
            get_data_block(indir_node, *rwdata.data());
            indir_node = indirect_addr[n_ptr_in_indirect + 1];
        }
        i++;
    } while (i < nth_indirect);
    int32_t indirect_ptr =
        ptr_n - meta_inode_ptr_len - nth_indirect * n_ptr_in_indirect;
    indirect_addr[indirect_ptr] = data_ptr;
    set_data_block(curr_addr, *rwdata.data());
    set_inode(inode_n, inode);
    return data_ptr;

    // if (inode.indirect_inode_ptr == fs_nullptr) {
    //     std::fill_n(indirect_addr, n_ptr_in_indirect + 1, fs_nullptr);
    //     address last_addr = data_bitmap.next_free(0);
    //     address curr_addr = last_addr;
    //     inode.indirect_inode_ptr = curr_addr;
    //     if (nth_indirect > 0) {
    //         last_addr = data_bitmap.next_free(last_addr + 1);
    //         indirect_addr[n_ptr_in_indirect + 1] = last_addr;
    //     }

    //     for (int32_t i = 0; i < nth_indirect; i++) {
    //         set_data_block(curr_addr, *rwdata.data());
    //         curr_addr = last_addr;
    //         std::fill_n(indirect_addr, n_ptr_in_indirect + 1, fs_nullptr);
    //         if ((i + 1) < nth_indirect) {
    //             last_addr = data_bitmap.next_free(curr_addr);
    //             indirect_addr[n_ptr_in_indirect + 1] = last_addr;
    //         }
    //     }
    //     int32_t indirect_ptr =
    //         ptr_n - meta_inode_ptr_len - nth_indirect * n_ptr_in_indirect;
    //     indirect_addr[indirect_ptr] = data_ptr;
    //     set_data_block(curr_addr, *rwdata.data());
    //     set_inode(inode_n, inode);
    //     return data_ptr;
    // }
}
address MemoryIO::get_nth_ptr(address inode_n, address ptr_n) {
    if ((inode_n < 0) || (ptr_n < 0)) {
        throw std::invalid_argument("Values cannot be negative");
    }

    if (!inode_bitmap.get_block_status(inode_n)) {
        throw std::runtime_error("Inode is not allocated.");
    }

    inode_block inode = {};
    get_inode(inode_n, inode);
    int32_t n_used_ptrs = inode.file_len / MB.block_size;
    n_used_ptrs += n_used_ptrs % MB.block_size ? 1 : 0;
    if ((n_used_ptrs - 1) < ptr_n) {
        return fs_nullptr;
    }

    if (ptr_n < meta_inode_ptr_len) {
        return inode.block_ptr[ptr_n];
    }

    fsize n_ptr_in_indirect = (MB.block_size / sizeof(address) - 1);
    int32_t nth_indirect = (ptr_n - meta_inode_ptr_len) / n_ptr_in_indirect;
    std::vector<data> rdata(MB.block_size);
    address* indirect_addr = reinterpret_cast<address*>(rdata.data());
    get_data_block(inode.indirect_inode_ptr, *rdata.data());
    for (int32_t i = 0; i < nth_indirect; i++) {
        get_data_block(indirect_addr[n_ptr_in_indirect], *rdata.data());
    }
    int32_t indirect_ptr =
        ptr_n - meta_inode_ptr_len - nth_indirect * n_ptr_in_indirect;

    return indirect_addr[indirect_ptr];
}

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