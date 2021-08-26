#include "inode.hpp"

#include <cstring>
namespace FSFS {
Inode::Inode() : loaded_inode_n(fs_nullptr) { clear(); }

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

address const& Inode::operator[](address ptr_n) const {
    if (loaded_inode_n == fs_nullptr) {
        throw std::runtime_error("Inode not initialized.");
    }

    if (ptr_n < meta_inode_ptr_len) {
        return inode.block_ptr[ptr_n];
    }

    return indirect_inode[ptr_n - meta_inode_ptr_len];
}

void Inode::add_data(address new_data_n) { ptrs_to_allocate.push_front(new_data_n); }

void Inode::load_direct(address inode_n, Block& data_block) {
    address block_n = data_block.inode_n_to_block_n(inode_n);
    fsize offset = inode_n % data_block.get_n_inodes_in_block() * meta_fragm_size;
    data* data_inode_p = cast_to_data(&inode);

    fsize n_read = data_block.read(block_n, data_inode_p, offset, meta_fragm_size);
    if (n_read != meta_fragm_size) {
        throw std::runtime_error("Error while read operation.");
    }
    memcpy(&inode_buf, &inode, sizeof(inode_block));
}

void Inode::load(address inode_n, Block& data_block) {
    if (loaded_inode_n == inode_n) {
        return;
    }

    load_direct(inode_n, data_block);

    fsize n_ptrs = data_block.bytes_to_blocks(inode.file_len) - meta_inode_ptr_len;
    if (n_ptrs > 0 && inode.indirect_inode_ptr != fs_nullptr) {
        indirect_inode.load(inode.indirect_inode_ptr, data_block, n_ptrs);
    }

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
    inode.type = block_status::Used;
    inode.file_len = 0;
    inode.file_name[0] = '\0';
    inode.indirect_inode_ptr = fs_nullptr;
    for (auto& ptr_n : inode.block_ptr) {
        ptr_n = fs_nullptr;
    }
}

void Inode::commit(Block& data_block) {
    if (loaded_inode_n == fs_nullptr) {
        return;
    }

    address block_n = data_block.inode_n_to_block_n(loaded_inode_n);
    fsize offset = loaded_inode_n % data_block.get_n_inodes_in_block() * meta_fragm_size;
    data* data_inode_p = cast_to_data(&inode_buf);

    if (!ptrs_to_allocate.empty()) {
        fsize ptrs_used = data_block.bytes_to_blocks(inode.file_len);
        if (ptrs_used >= meta_inode_ptr_len) {
            // TODO pass to indirect
        }

        ptrs_to_allocate.reverse();
        while (!ptrs_to_allocate.empty()) {
            if (ptrs_used >= meta_inode_ptr_len) {
                // TODO pass to indirect
            }
            inode_buf.block_ptr[ptrs_used] = ptrs_to_allocate.front();
            ptrs_to_allocate.pop_front();
            ptrs_used++;
        }
    }

    fsize n_written = data_block.write(block_n, data_inode_p, offset, meta_fragm_size);
    if (n_written != meta_fragm_size) {
        throw std::runtime_error("Error while read operation.");
    }

    indirect_inode.commit(data_block);
}
}
