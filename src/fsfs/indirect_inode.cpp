#include "indirect_inode.hpp"

namespace FSFS {

IndirectInode::IndirectInode(const inode_block& inode) : inode(inode) { clear(); }

address IndirectInode::ptr(address ptr_n) const { return indirect_ptrs_list[ptr_n]; }

address IndirectInode::last_indirect_ptr(address indirect_ptr_n) const {
    auto it = std::next(indirect_block_n.cbegin(), indirect_ptr_n);
    if (it == indirect_block_n.cend()) {
        return fs_nullptr;
    }

    return *it;
}

fsize IndirectInode::commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate) {
    if (inode.indirect_inode_ptr == fs_nullptr) {
        throw std::runtime_error("Cannot access base address of indirect inode.");
    }

    if (ptrs_to_allocate.empty()) {
        return 0;
    }

    if (indirect_block_n.empty()) {
        indirect_block_n.push_front(inode.indirect_inode_ptr);
    }

    fsize n_used_indirect_ptrs = std::max(data_block.bytes_to_blocks(inode.file_len) - meta_inode_ptr_len, 0);
    fsize n_new_ptrs = std::distance(ptrs_to_allocate.begin(), ptrs_to_allocate.end());
    fsize n_to_write = n_new_ptrs;

    indirect_ptrs_list.resize(indirect_ptrs_list.size() + n_new_ptrs);
    for (auto i = 0; !ptrs_to_allocate.empty(); i++) {
        indirect_ptrs_list[n_used_indirect_ptrs + i] = ptrs_to_allocate.front();
        ptrs_to_allocate.pop_front();
    }

    fsize last_ptr_in_block = n_used_indirect_ptrs % (data_block.get_n_addreses_in_block() - 1);
    fsize n_free_slots = (data_block.get_n_addreses_in_block() - 1) - last_ptr_in_block;
    fsize n_ptrs_to_write = std::min(n_free_slots, n_to_write);
    if (n_ptrs_to_write > 0) {
        data* indirect_ptrs_list_p = cast_to_data(&indirect_ptrs_list[n_used_indirect_ptrs]);
        address addr = data_block.data_n_to_block_n(indirect_block_n.front());

        data_block.write(addr, indirect_ptrs_list_p, last_ptr_in_block * sizeof(address),
                         n_ptrs_to_write * sizeof(address));

        n_to_write -= n_ptrs_to_write;
        n_used_indirect_ptrs += n_ptrs_to_write;
    }

    while (n_to_write > 0) {
        address new_block_addr = data_bitmap.next_free(0);
        if (new_block_addr == fs_nullptr) {
            clear();
            return n_new_ptrs - n_to_write;
        }
        data_bitmap.set_block(new_block_addr, 1);
        address addr = data_block.data_n_to_block_n(indirect_block_n.front());
        indirect_block_n.push_front(new_block_addr);

        data_block.write(addr, cast_to_data(&new_block_addr), -static_cast<fsize>(sizeof(address)), sizeof(address));

        data* indirect_ptrs_list_p = cast_to_data(&indirect_ptrs_list[n_used_indirect_ptrs]);
        n_ptrs_to_write = std::min(data_block.get_n_addreses_in_block() - 1, n_to_write);
        addr = data_block.data_n_to_block_n(indirect_block_n.front());

        data_block.write(addr, cast_to_data(indirect_ptrs_list_p), 0, n_ptrs_to_write * sizeof(address));

        n_to_write -= n_ptrs_to_write;
        n_used_indirect_ptrs += n_ptrs_to_write;
    }

    clear();
    return n_new_ptrs - n_to_write;
}

void IndirectInode::clear() {
    indirect_ptrs_list.clear();
    indirect_block_n.clear();
}

void IndirectInode::load(Block& data_block) {
    fsize n_indirect_used_ptrs = data_block.bytes_to_blocks(inode.file_len) - meta_inode_ptr_len;
    if ((n_indirect_used_ptrs <= 0) || (inode.indirect_inode_ptr == fs_nullptr)) {
        return;
    }

    clear();
    indirect_ptrs_list.resize(n_indirect_used_ptrs);

    address indirect_block_ptr = inode.indirect_inode_ptr;
    indirect_block_n.push_front(indirect_block_ptr);

    fsize ptrs_length = data_block.get_block_size() - sizeof(address);

    fsize n_read_ptrs = 0;
    fsize n_indirect_blocks = n_indirect_used_ptrs / (data_block.get_n_addreses_in_block() - 1);
    for (address nth_block = 0; nth_block < n_indirect_blocks; nth_block++) {
        data* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        fsize addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, ptrs_length);
        data_block.read(addr, cast_to_data(&indirect_block_ptr), ptrs_length, sizeof(address));
        indirect_block_n.push_front(indirect_block_ptr);

        n_read_ptrs += (data_block.get_n_addreses_in_block() - 1);
    }

    if (n_read_ptrs < n_indirect_used_ptrs) {
        data* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        fsize n_ptrs_left = n_indirect_used_ptrs - n_read_ptrs;
        fsize addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, n_ptrs_left * sizeof(address));
    }

    // indirect_block_n.push_front(indirect_block_ptr);
}
}
