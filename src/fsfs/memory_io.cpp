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
    data_block.reinit();
}

decltype(auto) MemoryIO::get_abs_addr(address inode_n, fsize abs_ptr_n) {
    if (abs_ptr_n < meta_inode_ptr_len) {
        return inode.get(inode_n).block_ptr[abs_ptr_n];
    }

    abs_ptr_n -= meta_inode_ptr_len;
    return iinode.get_ptr(inode.get(inode_n).indirect_inode_ptr, abs_ptr_n);
}

decltype(auto) MemoryIO::set_abs_addr(address inode_n, fsize abs_ptr_n) {
    if (abs_ptr_n < meta_inode_ptr_len) {
        return inode.update(inode_n).block_ptr[abs_ptr_n];
    }

    abs_ptr_n -= meta_inode_ptr_len;
    return iinode.set_ptr(inode.get(inode_n).indirect_inode_ptr, abs_ptr_n);
}

void MemoryIO::set_data_blocks_status(address inode_n, bool status) {
    fsize n_ptrs_used = bytes_to_blocks(inode.get(inode_n).file_len);

    fsize n_direct_ptrs = std::min(n_ptrs_used, meta_inode_ptr_len);
    for (auto i = 0; i < n_direct_ptrs; i++) {
        data_bitmap.set_block(inode.get(inode_n).block_ptr[i], status);
        n_ptrs_used--;
    }

    address indirect_address = inode.get(inode_n).indirect_inode_ptr;
    for (auto i = 0; i < n_ptrs_used; i++) {
        data_bitmap.set_block(iinode.get_ptr(indirect_address, i), status);
        if (i % iinode.get_n_ptrs_in_block() == 0) {
            data_bitmap.set_block(iinode.get_block_address(indirect_address, i), status);
        }
    }
}

address MemoryIO::expand_inode(address inode_n) {
    auto new_block_addr = data_bitmap.next_free(0);
    if (new_block_addr == fs_nullptr) {
        return fs_nullptr;
    }

    inode.update(inode_n).indirect_inode_ptr = new_block_addr;
    iinode.alloc(new_block_addr);
    data_bitmap.set_block(new_block_addr, 1);
    return new_block_addr;
}

address MemoryIO::expand_indirect(address data_n) {
    auto new_block_addr = data_bitmap.next_free(0);
    if (new_block_addr == fs_nullptr) {
        return fs_nullptr;
    }

    iinode.set_indirect_address(data_n, new_block_addr);
    data_bitmap.set_block(new_block_addr, 1);
    return new_block_addr;
}

address MemoryIO::assign_data_block(address inode_n, fsize abs_ptr_n) {
    auto new_block_addr = data_bitmap.next_free(0);
    if (new_block_addr == fs_nullptr) {
        return fs_nullptr;
    }

    set_abs_addr(inode_n, abs_ptr_n) = new_block_addr;
    data_bitmap.set_block(new_block_addr, 1);
    return new_block_addr;
}

fsize MemoryIO::store_data(address data_n, const data* wdata, fsize length) {
    fsize to_write = std::min(length, MB.block_size);
    return data_block.write(data_n, wdata, 0, to_write);
}

fsize MemoryIO::write_data(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::max;
    using std::min;

    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    inode.reinit();
    iinode.reinit();

    fsize n_written = 0;
    fsize n_ptr_used = bytes_to_blocks(inode.get(inode_n).file_len);
    address last_ptr_n = max(0, n_ptr_used - 1);
    address base_indirect = inode.get(inode_n).indirect_inode_ptr;

    fsize free_bytes = n_ptr_used * MB.block_size - inode.get(inode_n).file_len;
    if (free_bytes > 0) {
        address block_addr = get_abs_addr(inode_n, last_ptr_n);
        n_written += data_block.write(block_addr, wdata, -free_bytes, free_bytes);
        last_ptr_n++;
    }

    fsize blocks_to_allocate = bytes_to_blocks(max(0, length - free_bytes));
    fsize n_direct_blocks = max(min(meta_inode_ptr_len - last_ptr_n, blocks_to_allocate), 0);

    for (auto i = 0; i < n_direct_blocks; i++) {
        address data_n = assign_data_block(inode_n, last_ptr_n);
        if (data_n == fs_nullptr) {
            inode.update(inode_n).file_len += n_written;
            inode.commit();
            return n_written;
        }

        n_written += store_data(data_n, &wdata[n_written], length - n_written);
        last_ptr_n += 1;
    }
    blocks_to_allocate -= n_direct_blocks;

    fsize n_indirect_blocks = blocks_to_allocate / iinode.get_n_ptrs_in_block();
    n_indirect_blocks += blocks_to_allocate % iinode.get_n_ptrs_in_block() ? 1 : 0;
    if ((base_indirect == fs_nullptr) && (n_indirect_blocks > 0)) {
        address indirect_block = expand_inode(inode_n);
        if (indirect_block == fs_nullptr) {
            inode.update(inode_n).file_len += n_written;
            inode.commit();
            return n_written;
        }
        base_indirect = indirect_block;
    }
    inode.commit();

    for (auto nth_indirect = 0; nth_indirect < n_indirect_blocks; nth_indirect++) {
        fsize n_blocks_to_write = min(blocks_to_allocate, iinode.get_n_ptrs_in_block());
        for (auto i = 0; i < n_blocks_to_write; i++) {
            address data_n = assign_data_block(inode_n, last_ptr_n);
            if (data_n == fs_nullptr) {
                inode.update(inode_n).file_len += n_written;
                iinode.commit();
                inode.commit();
                return n_written;
            }

            n_written += store_data(data_n, &wdata[n_written], length - n_written);
            last_ptr_n += 1;
        }
        blocks_to_allocate -= n_blocks_to_write;

        if (blocks_to_allocate > 0) {
            address data_n = iinode.get_block_address(base_indirect, last_ptr_n - meta_inode_ptr_len - 1);
            address indirect_block = expand_indirect(data_n);
            if (indirect_block == fs_nullptr) {
                inode.update(inode_n).file_len += n_written;
                inode.commit();
                iinode.commit();
                return n_written;
            }
        }
        iinode.commit();
    }
    inode.update(inode_n).file_len += n_written;
    inode.commit();

    return n_written;
}

address MemoryIO::read_data(address inode_n, const data* wdata, address offset, fsize length) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    inode.reinit();
    iinode.reinit();

    if (offset >= inode.get(inode_n).file_len) {
    }
}

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

    inode.reinit();
    iinode.reinit();
    inode.update(inode_n).type = block_status::Free;
    inode.commit();

    set_data_blocks_status(inode_n, 0);
    inode_bitmap.set_block(inode_n, 0);

    return inode_n;
}

address MemoryIO::rename_inode(address inode_n, const char* file_name) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    if (strlen(file_name) + 1 >= meta_file_name_len) {
        return fs_nullptr;
    }

    inode.reinit();
    strcpy(inode.update(inode_n).file_name, file_name);
    inode.commit();

    return inode_n;
}

fsize MemoryIO::get_inode_length(address inode_n) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.reinit();
    return inode.get(inode_n).file_len;
}

address MemoryIO::get_inode_file_name(address inode_n, char* file_name_buffer) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.reinit();
    strcpy(file_name_buffer, inode.get(inode_n).file_name);
    return inode_n;
}

void MemoryIO::scan_blocks() {
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);
    inode.reinit();

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        if (inode.get(inode_n).type != block_status::Used) {
            continue;
        }

        inode_bitmap.set_block(inode_n, 1);
        set_data_blocks_status(inode_n, 1);
    }
}

fsize MemoryIO::bytes_to_blocks(fsize length) {
    length = std::max(0, length);
    fsize n_ptrs_used = length / MB.block_size;
    n_ptrs_used += length % MB.block_size ? 1 : 0;
    return n_ptrs_used;
}
}
