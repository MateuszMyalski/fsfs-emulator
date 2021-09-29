#include "inode.hpp"

#include <cstring>
namespace FSFS {
Inode::Inode() : loaded_inode_n(fs_nullptr), indirect_inode(inode) { clear(); }

inode_block const& Inode::meta() const {
    if (loaded_inode_n == fs_nullptr) {
        throw std::runtime_error("Inode not initialized.");
    }
    return inode;
}

inode_block& Inode::meta() {
    if (loaded_inode_n == fs_nullptr) {
        throw std::runtime_error("Inode not allocated.");
    }
    return inode_buf;
}

address Inode::ptr(address ptr_n) const {
    if (loaded_inode_n == fs_nullptr) {
        throw std::runtime_error("Inode not initialized.");
    }

    if (ptr_n < meta_n_direct_ptrs) {
        return inode.direct_ptr[ptr_n];
    }

    return indirect_inode.ptr(ptr_n - meta_n_direct_ptrs);
}

void Inode::add_data(address new_data_n) { ptrs_to_allocate.push_front(new_data_n); }

void Inode::load_direct(address inode_n, Block& data_block) {
    address block_n = data_block.inode_n_to_block_n(inode_n);
    fsize offset = inode_n % data_block.get_n_inodes_in_block() * meta_fragm_size_bytes;
    data* data_inode_p = cast_to_data(&inode);

    data_block.read(block_n, data_inode_p, offset, meta_fragm_size_bytes);
    memcpy(&inode_buf, &inode, sizeof(inode_block));
}

void Inode::load(address inode_n, Block& data_block) {
    if (loaded_inode_n == inode_n) {
        return;
    }

    load_direct(inode_n, data_block);
    indirect_inode.load(data_block);

    loaded_inode_n = inode_n;
}

void Inode::clear() {
    memset(&inode, 0x00, sizeof(inode_block));
    memset(&inode_buf, 0x00, sizeof(inode_block));
    loaded_inode_n = fs_nullptr;
    ptrs_to_allocate.clear();
    indirect_inode.clear();
}

void Inode::alloc_new(address inode_n) {
    clear();

    loaded_inode_n = inode_n;
    inode_buf.status = block_status::Used;
    inode_buf.file_len = 0;
    inode_buf.file_name[0] = '\0';
    inode_buf.indirect_inode_ptr = fs_nullptr;
    for (address i = 0; i < meta_n_direct_ptrs; i++) {
        inode_buf.direct_ptr[i] = fs_nullptr;
    }

    memcpy(&inode, &inode_buf, sizeof(inode_block));
}

address Inode::last_indirect_ptr(address indirect_ptr_n) const {
    return indirect_inode.last_indirect_ptr(indirect_ptr_n);
}

fsize Inode::commit(Block& data_block, BlockBitmap& data_bitmap) {
    if (loaded_inode_n == fs_nullptr) {
        return 0;
    }

    ptrs_to_allocate.reverse();  // Adding data to the forward list is in reversed order
    fsize n_ptrs_written = 0;

    address addr = data_block.inode_n_to_block_n(loaded_inode_n);
    fsize inode_n_offset = loaded_inode_n % data_block.get_n_inodes_in_block() * meta_fragm_size_bytes;
    fsize ptrs_used = data_block.bytes_to_blocks(inode.file_len);
    while (!ptrs_to_allocate.empty()) {
        if (ptrs_used >= meta_n_direct_ptrs) {
            if (inode.indirect_inode_ptr == fs_nullptr) {
                // No more direct ptr slots, allocate new indirect slot if needed or use already alloceted one
                address new_block_n = data_bitmap.next_free(0);
                if (new_block_n == fs_nullptr) {
                    break;
                }
                data_bitmap.set_status(new_block_n, 1);
                meta().indirect_inode_ptr = new_block_n;
                inode.indirect_inode_ptr = new_block_n;
            }
            n_ptrs_written += indirect_inode.commit(data_block, data_bitmap, ptrs_to_allocate);
            break;
        }
        inode_buf.direct_ptr[ptrs_used] = ptrs_to_allocate.front();
        ptrs_to_allocate.pop_front();

        ptrs_used++;
        n_ptrs_written++;
    }

    data_block.write(addr, cast_to_data(&inode_buf), inode_n_offset, meta_fragm_size_bytes);
    clear();
    return n_ptrs_written;
}
}
