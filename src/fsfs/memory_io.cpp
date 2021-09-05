#include "memory_io.hpp"

#include <algorithm>
#include <cstring>

namespace FSFS {

void MemoryIO::init(const super_block& MB) {
    std::memcpy(&this->MB, &MB, sizeof(MB));
    inode_bitmap.resize(MB.n_inode_blocks);
    data_bitmap.resize(MB.n_data_blocks);
    block.resize();
}

void MemoryIO::set_data_blocks_status(address inode_n, bool status) {
    inode.load(inode_n, block);
    fsize n_ptrs_used = block.bytes_to_blocks(inode.meta().file_len);

    for (fsize i = 0; i < n_ptrs_used; i++) {
        data_bitmap.set_status(inode.ptr(i), status);
    }

    fsize indirect_addr = inode.last_indirect_ptr(0);
    for (fsize indirect_block_n = 0; indirect_addr != fs_nullptr; indirect_block_n++) {
        data_bitmap.set_status(indirect_addr, status);
        indirect_addr = inode.last_indirect_ptr(indirect_block_n);
    }
}

fsize MemoryIO::edit_data(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::min;

    if (offset == 0) {
        return 0;
    }

    inode.load(inode_n, block);

    // Step 1: Check if there is data to edit
    fsize abs_offset = inode.meta().file_len - offset;
    if (abs_offset < 0) {
        return fs_nullptr;
    }

    // Step 2: Edit tail in last data block
    address ptr_n = abs_offset / MB.block_size;
    fsize first_offset = abs_offset % MB.block_size;
    fsize n_written_bytes = block.write(block.data_n_to_block_n(inode.ptr(ptr_n)), wdata, first_offset,
                                        min(MB.block_size - first_offset, length));
    ptr_n += 1;

    // Step 3: Edit full blocks of data
    if (length - n_written_bytes > 0) {
        fsize blocks_to_edit = block.bytes_to_blocks(length);
        for (auto i = 0; i < blocks_to_edit; i++) {
            address addr = block.data_n_to_block_n(inode.ptr(ptr_n));
            fsize write_length = min(MB.block_size, length - n_written_bytes);
            n_written_bytes += block.write(addr, &wdata[n_written_bytes], 0, write_length);
            ptr_n++;
        }
    }

    return n_written_bytes;
}

fsize MemoryIO::write_data(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::max;
    using std::min;

    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    inode.load(inode_n, block);

    // Step 1: Check if there is data to edit
    //
    fsize n_eddited_bytes = edit_data(inode_n, wdata, offset, min(length, offset));
    if (n_eddited_bytes == length || n_eddited_bytes == fs_nullptr) {
        return n_eddited_bytes;
    }

    // Step 2: Prepare inforamtions to allocate new data blocks
    //
    const data* wdata_new_p = &wdata[n_eddited_bytes];
    fsize n_written = 0;
    fsize n_ptr_used = block.bytes_to_blocks(inode.meta().file_len);
    fsize free_bytes = n_ptr_used * MB.block_size - inode.meta().file_len;
    fsize blocks_of_new_data = block.bytes_to_blocks(max(0, length - free_bytes - n_eddited_bytes));

    // Step 3: Store new data in already allocated block
    //
    if (free_bytes > 0) {
        address last_ptr_n = max(0, n_ptr_used - 1);
        address addr = block.data_n_to_block_n(inode.ptr(last_ptr_n));
        n_written += block.write(addr, wdata_new_p, -free_bytes, free_bytes);
    }

    // Step 4: Store data in new allocated blocks
    //
    for (auto i = 0; i < blocks_of_new_data; i++) {
        // Allocate new block
        address data_n = data_bitmap.next_free(0);
        if (data_n == fs_nullptr) {
            break;
        }
        data_bitmap.set_status(data_n, 1);
        inode.add_data(data_n);

        // Store data in block
        address addr = block.data_n_to_block_n(data_n);
        fsize to_write = std::min(length - n_written - n_eddited_bytes, MB.block_size);
        n_written += block.write(addr, &wdata_new_p[n_written], 0, to_write);
    }

    // Step 5: Update inode meta
    //
    inode.meta().file_len += n_written;
    inode.commit(block, data_bitmap);
    // TODO add check form inode commit

    return n_written + n_eddited_bytes;
}

address MemoryIO::read_data(address inode_n, const data* wdata, address offset, fsize length) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    // TODO ...
}

address MemoryIO::alloc_inode(const char* file_name) {
    address inode_n = inode_bitmap.next_free(0);
    if (fs_nullptr == inode_n) {
        // No free inode blocks
        return fs_nullptr;
    }

    fsize file_name_len = strnlen(file_name, meta_max_file_name_size);
    if (file_name_len == meta_max_file_name_size) {
        return fs_nullptr;
    }

    inode.alloc_new(inode_n);
    memcpy(inode.meta().file_name, file_name, file_name_len);
    inode.commit(block, data_bitmap);

    inode_bitmap.set_status(inode_n, 1);

    return inode_n;
}

address MemoryIO::dealloc_inode(address inode_n) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    inode.meta().status = block_status::Free;
    inode.commit(block, data_bitmap);

    set_data_blocks_status(inode_n, 0);
    inode_bitmap.set_status(inode_n, 0);

    return inode_n;
}

address MemoryIO::rename_inode(address inode_n, const char* file_name) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    fsize file_name_len = strnlen(file_name, meta_max_file_name_size);
    if (file_name_len == meta_max_file_name_size - 1) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    memcpy(inode.meta().file_name, file_name, file_name_len);
    inode.commit(block, data_bitmap);

    return inode_n;
}

fsize MemoryIO::get_inode_length(address inode_n) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    return inode.meta().file_len;
}

address MemoryIO::get_inode_file_name(address inode_n, char* file_name_buffer) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    strcpy(file_name_buffer, inode.meta().file_name);
    return inode_n;
}

void MemoryIO::scan_blocks() {
    inode_bitmap.resize(MB.n_inode_blocks);
    data_bitmap.resize(MB.n_data_blocks);

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        inode.load(inode_n, block);
        if (inode.meta().status != block_status::Used) {
            continue;
        }
        inode_bitmap.set_status(inode_n, 1);

        set_data_blocks_status(inode_n, 1);
    }
}

}
