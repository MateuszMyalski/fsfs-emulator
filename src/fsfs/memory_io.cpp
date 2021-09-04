#include "memory_io.hpp"

#include <algorithm>
#include <cstring>

namespace FSFS {

void MemoryIO::init(const super_block& MB) {
    std::memcpy(&this->MB, &MB, sizeof(MB));
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    data_block.reinit();
}

void MemoryIO::set_data_blocks_status(address inode_n, bool status) {
    inode.load(inode_n, data_block);
    fsize n_ptrs_used = bytes_to_blocks(inode.meta().file_len);

    for (auto i = 0; i < n_ptrs_used; i++) {
        data_bitmap.set_block(inode.ptr(i), status);
    }

    auto indirect_addr = inode.last_indirect_ptr(0);
    for (auto indirect_block_n = 0; indirect_addr != fs_nullptr; indirect_block_n++) {
        data_bitmap.set_block(indirect_block_n, status);
        indirect_addr = inode.last_indirect_ptr(indirect_block_n);
    }
}

fsize MemoryIO::store_data(address data_n, const data* wdata, fsize length) {
    fsize to_write = std::min(length, MB.block_size);
    return data_block.write(data_n, wdata, 0, to_write);
}

fsize MemoryIO::edit_data(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::min;

    if (offset == 0) {
        return 0;
    }

    inode.load(inode_n, data_block);

    fsize abs_offset = inode.meta().file_len - offset;
    if (abs_offset < 0) {
        return fs_nullptr;
    }

    fsize n_written = 0;

    address ptr_n = abs_offset / MB.block_size;
    fsize first_offset = abs_offset % MB.block_size;
    n_written += data_block.write(data_block.data_n_to_block_n(inode.ptr(ptr_n)), wdata, first_offset,
                                  min(MB.block_size - first_offset, length));

    ptr_n += 1;
    if (length - n_written > 0) {
        fsize blocks_to_edit = bytes_to_blocks(length);
        for (auto i = 0; i < blocks_to_edit; i++) {
            address addr = data_block.data_n_to_block_n(inode.ptr(ptr_n));
            fsize write_length = min(MB.block_size, length - n_written);
            n_written += data_block.write(addr, &wdata[n_written], 0, write_length);
            ptr_n++;
        }
    }

    return n_written;
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

    inode.load(inode_n, data_block);

    fsize n_eddited = edit_data(inode_n, wdata, offset, min(length, offset));
    if (n_eddited == length || n_eddited == fs_nullptr) {
        return n_eddited;
    }

    const data* wdata_new_p = &wdata[n_eddited];
    fsize n_written = 0;
    fsize n_ptr_used = bytes_to_blocks(inode.meta().file_len);
    fsize free_bytes = n_ptr_used * MB.block_size - inode.meta().file_len;
    fsize blocks_of_new_data = bytes_to_blocks(max(0, length - free_bytes - n_eddited));

    if (free_bytes > 0) {
        address last_ptr_n = max(0, n_ptr_used - 1);
        address addr = data_block.data_n_to_block_n(inode.ptr(last_ptr_n));
        n_written += data_block.write(addr, wdata_new_p, -free_bytes, free_bytes);
    }

    for (auto i = 0; i < blocks_of_new_data; i++) {
        address data_n = data_bitmap.next_free(0);
        if (data_n == fs_nullptr) {
            break;
        }
        data_bitmap.set_block(data_n, 1);
        inode.add_data(data_n);

        address addr = data_block.data_n_to_block_n(data_n);
        fsize to_write = std::min(length - n_written - n_eddited, MB.block_size);
        n_written += data_block.write(addr, &wdata_new_p[n_written], 0, to_write);
    }

    inode.meta().file_len += n_written;
    inode.commit(data_block, data_bitmap);
    // TODO add check form inode commit

    return n_written + n_eddited;
}

address MemoryIO::read_data(address inode_n, const data* wdata, address offset, fsize length) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    // TODO ...
}

address MemoryIO::alloc_inode(const char* file_name) {
    address inode_n = inode_bitmap.next_free(0);

    /* No more free inodes */
    if (fs_nullptr == inode_n) {
        return fs_nullptr;
    }

    if (strlen(file_name) + 1 >= meta_file_name_len) {
        return fs_nullptr;
    }

    inode.alloc_new(inode_n);
    strcpy(inode.meta().file_name, file_name);
    inode.commit(data_block, data_bitmap);

    inode_bitmap.set_block(inode_n, 1);

    return inode_n;
}

address MemoryIO::dealloc_inode(address inode_n) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, data_block);
    inode.meta().type = block_status::Free;
    inode.commit(data_block, data_bitmap);

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

    inode.load(inode_n, data_block);
    strcpy(inode.meta().file_name, file_name);
    inode.commit(data_block, data_bitmap);

    return inode_n;
    // TODO strcpy/strlen EVIL!
}

fsize MemoryIO::get_inode_length(address inode_n) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, data_block);
    return inode.meta().file_len;
}

address MemoryIO::get_inode_file_name(address inode_n, char* file_name_buffer) {
    if (!inode_bitmap.get_block_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, data_block);
    strcpy(file_name_buffer, inode.meta().file_name);
    return inode_n;
}

void MemoryIO::scan_blocks() {
    inode_bitmap.init(MB.n_inode_blocks);
    data_bitmap.init(MB.n_data_blocks);

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        inode.load(inode_n, data_block);
        if (inode.meta().type != block_status::Used) {
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
