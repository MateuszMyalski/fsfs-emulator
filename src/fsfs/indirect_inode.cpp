#include "indirect_inode.hpp"

namespace FSFS {

IndirectInode::IndirectInode(const inode_block& inode) : inode(inode) { clear(); }

int32_t IndirectInode::ptr(int32_t ptr_n) const { return indirect_ptrs_list[ptr_n]; }

int32_t IndirectInode::last_indirect_ptr(int32_t indirect_ptr_n) const {
    auto it = std::next(indirect_block_n.cbegin(), indirect_ptr_n);
    if (it == indirect_block_n.cend()) {
        return fs_nullptr;
    }

    return *it;
}

int32_t IndirectInode::commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate) {
    if (inode.indirect_inode_ptr == fs_nullptr) {
        throw std::runtime_error("Cannot access base int32_t of indirect inode.");
    }

    if (ptrs_to_allocate.empty()) {
        return 0;
    }

    if (indirect_block_n.empty()) {
        indirect_block_n.push_front(inode.indirect_inode_ptr);
    }

    // Step 1: Prepare new ptrs in list
    //
    int32_t n_used_indirect_ptrs = std::max(data_block.bytes_to_blocks(inode.file_len) - meta_n_direct_ptrs, 0);
    int32_t n_new_ptrs = std::distance(ptrs_to_allocate.begin(), ptrs_to_allocate.end());
    int32_t n_ptrs_left_to_write = n_new_ptrs;

    indirect_ptrs_list.resize(indirect_ptrs_list.size() + n_new_ptrs);
    for (auto i = 0; !ptrs_to_allocate.empty(); i++) {
        indirect_ptrs_list[n_used_indirect_ptrs + i] = ptrs_to_allocate.front();
        ptrs_to_allocate.pop_front();
    }

    // Step 2: Insert new ptrs in already allocated indirect block
    //
    int32_t last_ptr_in_block = n_used_indirect_ptrs % (data_block.get_n_addreses_in_block() - 1);
    int32_t n_free_ptr_slots = (data_block.get_n_addreses_in_block() - 1) - last_ptr_in_block;
    int32_t n_ptrs_to_write = std::min(n_free_ptr_slots, n_ptrs_left_to_write);
    if (n_ptrs_to_write > 0) {
        uint8_t* indirect_ptrs_list_p = cast_to_data(&indirect_ptrs_list[n_used_indirect_ptrs]);
        int32_t addr = data_block.data_n_to_block_n(indirect_block_n.front());
        data_block.write(addr, indirect_ptrs_list_p, last_ptr_in_block * sizeof(int32_t),
                         n_ptrs_to_write * sizeof(int32_t));

        n_ptrs_left_to_write -= n_ptrs_to_write;
        n_used_indirect_ptrs += n_ptrs_to_write;
    }

    // Step 3: Allocate new indirect uint8_t blocks and insert new ptrs
    //
    while (n_ptrs_left_to_write > 0) {
        // Prepare new uint8_t block
        int32_t new_block_addr = data_bitmap.next_free(0);
        if (new_block_addr == fs_nullptr) {
            clear();
            return n_new_ptrs - n_ptrs_left_to_write;
        }
        data_bitmap.set_status(new_block_addr, 1);
        int32_t addr = data_block.data_n_to_block_n(indirect_block_n.front());
        indirect_block_n.push_front(new_block_addr);

        // Link new uint8_t block to previous indirect block
        data_block.write(addr, cast_to_data(&new_block_addr), -static_cast<int32_t>(sizeof(int32_t)), sizeof(int32_t));

        // Store new ptrs
        uint8_t* indirect_ptrs_list_p = cast_to_data(&indirect_ptrs_list[n_used_indirect_ptrs]);
        n_ptrs_to_write = std::min(data_block.get_n_addreses_in_block() - 1, n_ptrs_left_to_write);
        addr = data_block.data_n_to_block_n(indirect_block_n.front());
        data_block.write(addr, cast_to_data(indirect_ptrs_list_p), 0, n_ptrs_to_write * sizeof(int32_t));

        n_ptrs_left_to_write -= n_ptrs_to_write;
        n_used_indirect_ptrs += n_ptrs_to_write;
    }

    clear();
    return n_new_ptrs - n_ptrs_left_to_write;
}

void IndirectInode::clear() {
    indirect_ptrs_list.clear();
    indirect_block_n.clear();
}

void IndirectInode::load(Block& data_block) {
    // Step 1: Check if inode has any indirect blocks
    //
    int32_t n_indirect_used_ptrs = data_block.bytes_to_blocks(inode.file_len) - meta_n_direct_ptrs;
    if ((n_indirect_used_ptrs <= 0) || (inode.indirect_inode_ptr == fs_nullptr)) {
        return;
    }

    clear();
    indirect_ptrs_list.resize(n_indirect_used_ptrs);

    int32_t indirect_block_ptr = inode.indirect_inode_ptr;
    indirect_block_n.push_front(indirect_block_ptr);

    // Step 2: Read full indirect blocks
    //
    int32_t indirect_ptrs_block_len = data_block.get_block_size() - sizeof(int32_t);
    int32_t n_read_ptrs = 0;
    int32_t n_indirect_blocks = n_indirect_used_ptrs / (data_block.get_n_addreses_in_block() - 1);
    for (int32_t nth_block = 0; nth_block < n_indirect_blocks; nth_block++) {
        uint8_t* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        int32_t addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, indirect_ptrs_block_len);
        data_block.read(addr, cast_to_data(&indirect_block_ptr), indirect_ptrs_block_len, sizeof(int32_t));
        indirect_block_n.push_front(indirect_block_ptr);

        n_read_ptrs += (data_block.get_n_addreses_in_block() - 1);
    }

    // Step 3: Read tail in the last indirect block
    //
    if (n_read_ptrs < n_indirect_used_ptrs) {
        uint8_t* data_indirect_p = cast_to_data(&indirect_ptrs_list[n_read_ptrs]);
        int32_t n_ptrs_left = n_indirect_used_ptrs - n_read_ptrs;
        int32_t addr = data_block.data_n_to_block_n(indirect_block_ptr);

        data_block.read(addr, data_indirect_p, 0, n_ptrs_left * sizeof(int32_t));
    }
}
}
