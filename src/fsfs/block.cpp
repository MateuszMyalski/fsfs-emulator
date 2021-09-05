#include "block.hpp"

#include <cstring>

namespace FSFS {
Block::Block(Disk& disk, const super_block& MB) : disk(disk), MB(MB) {
    disk.mount();
    reinit();
}

void Block::reinit() {
    casched_block = fs_nullptr;
    rwbuffer.resize(MB.block_size);
}

Block::~Block() { disk.unmount(); }

address Block::read_block(address block_n) {
    if (block_n == casched_block) {
        return block_n;
    }

    auto n_read = disk.read(block_n, rwbuffer.data(), MB.block_size);
    if (n_read != MB.block_size) {
        throw std::runtime_error("Error while read operaion.");
    }
    casched_block = block_n;

    return block_n;
}

fsize Block::write(address block_n, const data* wdata, fsize offset, fsize length) {
    if (block_n >= MB.n_blocks || block_n < 0) {
        throw std::invalid_argument("Invalid data block number.");
    }

    if (std::abs(offset) >= MB.block_size) {
        throw std::invalid_argument("Offest cannot be equal or greater than block size.");
    }

    if (length < 0) {
        throw std::invalid_argument("Length lower than zero.");
    }

    if (length == 0) {
        return length;
    }

    fsize real_offset = offset >= 0 ? offset : MB.block_size + offset;
    fsize writtable_space = MB.block_size - real_offset;
    length = std::min(length, writtable_space);

    read_block(block_n);
    std::memcpy(rwbuffer.data() + real_offset, wdata, length);
    auto n_write = disk.write(casched_block, rwbuffer.data(), MB.block_size);
    if (n_write != MB.block_size) {
        throw std::runtime_error("Error while write operaion.");
    }

    return length;
}

fsize Block::read(address block_n, data* rdata, fsize offset, fsize length) {
    if (block_n >= MB.n_blocks || block_n < 0) {
        throw std::invalid_argument("Invalid data block number.");
    }

    if (std::abs(offset) >= MB.block_size) {
        throw std::invalid_argument("Offest cannot be equal or greater than block size.");
    }

    fsize real_offset = offset >= 0 ? offset : MB.block_size + offset;
    fsize readable_space = MB.block_size - real_offset;
    if (length > readable_space || length < 0) {
        throw std::invalid_argument(
            "Length cannot be greater than readable space and lower than "
            "zero.");
    }

    if (length == 0) {
        return length;
    }

    read_block(block_n);
    std::memcpy(rdata, rwbuffer.data() + real_offset, length);
    return length;
}

address Block::data_n_to_block_n(address data_n) {
    if (data_n >= MB.n_data_blocks || data_n < 0) {
        throw std::invalid_argument("Invalid data block number.");
    }

    return data_n + MB.n_inode_blocks + fs_offset_inode_block;
}

address Block::inode_n_to_block_n(address inode_n) {
    if (inode_n >= MB.n_inode_blocks || inode_n < 0) {
        throw std::invalid_argument("Invalid inode block number.");
    }

    return inode_n / get_n_inodes_in_block() + fs_offset_inode_block;
}

fsize Block::get_n_inodes_in_block() { return MB.block_size / meta_fragm_size_bytes; }

fsize Block::get_n_addreses_in_block() { return MB.block_size / sizeof(address); }

fsize Block::get_block_size() { return MB.block_size; }

fsize Block::bytes_to_blocks(fsize length) {
    length = std::max(0, length);
    fsize n_ptrs_used = length / MB.block_size;
    n_ptrs_used += length % MB.block_size ? 1 : 0;
    return n_ptrs_used;
}
}