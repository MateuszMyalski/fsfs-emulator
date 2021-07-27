#include "memory_io.hpp"

#include <algorithm>
#include <cstring>

namespace FSFS {

void MemoryIO::init(const super_block& MB) {
    std::memcpy(&this->MB, &MB, sizeof(MB));
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);
    inode.reinit();
    iinode.reinit();
}

void MemoryIO::set_data_blocks_status(address inode_n, bool allocated) {
    fsize n_ptrs_used = bytes_to_blocks(inode.get(inode_n).file_len);
    fsize n_direct_ptrs = std::min(n_ptrs_used, meta_inode_ptr_len);
    for (auto i = 0; i < n_direct_ptrs; i++) {
        data_bitmap.set_block(inode.get(inode_n).block_ptr[i], allocated);
        n_ptrs_used--;
    }

    address indirect_address = inode.get(inode_n).indirect_inode_ptr;

    address nth_ptr_indirect = 0;
    fsize n_ptrs_indirect = MB.block_size / sizeof(address) - 2;
    for (auto i = 0; i < n_ptrs_used; i++) {
        nth_ptr_indirect++;
        data_bitmap.set_block(iinode.get_ptr(indirect_address, i), allocated);
        if (nth_ptr_indirect % n_ptrs_indirect == 0) {
            data_bitmap.set_block(iinode.get_block_address(indirect_address, i),
                                  allocated);
        }
    }
}

address MemoryIO::add_data(address inode_n, const data& wdata, fsize length) {}

address MemoryIO::edit_data(address inode_n, const data& wdata, address offset,
                            fsize length) {}

address MemoryIO::alloc_inode(const char* file_name) {
    address inode_n = inode_bitmap.next_free(0);

    /* No more free inodes */
    if (-1 == inode_n) {
        return fs_nullptr;
    }

    if (strlen(file_name) + 1 >= meta_file_name_len) {
        return fs_nullptr;
    }

    if (inode.alloc(inode_n) == fs_nullptr) {
        throw std::runtime_error("Error while allocating inode.");
    }

    strcpy(inode.update(inode_n).file_name, file_name);
    inode.commit();

    inode_bitmap.set_block(inode_n, 1);

    return inode_n;
}

address MemoryIO::dealloc_inode(address inode_n) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.update(inode_n).type = block_status::Free;
    inode.commit();

    set_data_blocks_status(inode_n, 0);

    return inode_n;
}

address MemoryIO::rename_inode(address inode_n, const char* file_name) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    if (strlen(file_name) + 1 >= meta_file_name_len) {
        return fs_nullptr;
    }

    strcpy(inode.update(inode_n).file_name, file_name);
    inode.commit();

    return inode_n;
}

fsize MemoryIO::get_inode_length(address inode_n) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    return inode.get(inode_n).file_len;
}

address MemoryIO::get_inode_file_name(address inode_n, char* file_name_buffer) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    strcpy(file_name_buffer, inode.get(inode_n).file_name);
    return inode_n;
}

void MemoryIO::scan_blocks() {
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (inode.get(inode_n).type != block_status::Used) {
            continue;
        }

        inode_bitmap.set_block(inode_n, 1);
        set_data_blocks_status(inode_n, 1);
    }
}

fsize MemoryIO::bytes_to_blocks(fsize length) {
    fsize n_ptrs_used = length / MB.block_size;
    n_ptrs_used += length % MB.block_size ? 1 : 0;
    return n_ptrs_used;
}
}
