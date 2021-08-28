#include "indirect_inode.hpp"

namespace FSFS {

IndirectInode::IndirectInode(const inode_block& inode) : inode(inode) { clear(); }

address IndirectInode::ptr(address ptr_n) const { return indirect_ptrs_list[ptr_n]; }

void IndirectInode::commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate) {}

void IndirectInode::clear() {
    last_indirect_block_addr = fs_nullptr;
    indirect_ptrs_list.clear();
}

void IndirectInode::load(Block& data_block) {
    fsize n_ptrs = data_block.bytes_to_blocks(inode.file_len) - meta_inode_ptr_len;
    if (n_ptrs <= meta_inode_ptr_len || inode.indirect_inode_ptr == fs_nullptr) {
        return;
    }

    indirect_ptrs_list.resize(n_ptrs);

    address indirect_block_ptr = inode.indirect_inode_ptr;
    fsize ptrs_length = data_block.get_block_size() - sizeof(address);

    fsize n_read_ptrs = 0;
    fsize n_indirect_blocks = n_ptrs / (data_block.get_n_addreses_in_block() - 1);
    for (address nth_block = 0; nth_block < n_indirect_blocks; nth_block++) {
        data* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        fsize addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, ptrs_length);
        data_block.read(addr, cast_to_data(&indirect_block_ptr), ptrs_length, sizeof(address));

        n_read_ptrs += (data_block.get_n_addreses_in_block() - 1);
    }

    if (n_read_ptrs < n_ptrs) {
        data* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        fsize n_ptrs_left = n_ptrs - n_read_ptrs;
        fsize addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, n_ptrs_left * sizeof(address));
    }

    last_indirect_block_addr = indirect_block_ptr;
}
}
