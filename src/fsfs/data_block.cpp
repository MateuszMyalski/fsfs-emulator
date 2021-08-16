#include "data_block.hpp"

#include <cstring>

namespace FSFS {
DataBlock::DataBlock(Disk& disk, const super_block& MB) : disk(disk), MB(MB) {
    disk.mount();
    reinit();
}

void DataBlock::reinit() {
    casched_block = fs_nullptr;
    rwbuffer.resize(MB.block_size);
}

DataBlock::~DataBlock() { disk.unmount(); }

address DataBlock::read_block(address block_n) {
    address block_addr = block_n + fs_offset_inode_block + MB.n_inode_blocks;
    if (block_addr == casched_block) {
        return block_n;
    }

    auto n_read = disk.read(block_addr, rwbuffer.data(), MB.block_size);
    if (n_read != MB.block_size) {
        throw std::runtime_error("Error while read operaion.");
    }
    casched_block = block_addr;

    return block_n;
}

fsize DataBlock::write(address block_n, const data* wdata, fsize offset, fsize length) {
    if (block_n >= MB.n_data_blocks || block_n < 0) {
        throw std::invalid_argument("Invalid data block number.");
    }

    if (std::abs(offset) >= MB.block_size) {
        throw std::invalid_argument("Offest cannot be equal or greater than block size.");
    }

    fsize real_offset = offset >= 0 ? offset : MB.block_size + offset;
    fsize writtable_space = MB.block_size - real_offset;
    if (length > writtable_space || length < 0) {
        throw std::invalid_argument(
            "Length cannot be greater than writtable space and lower than "
            "zero.");
    }

    if (length == 0) {
        return length;
    }

    read_block(block_n);
    std::memcpy(rwbuffer.data() + real_offset, wdata, length);
    auto n_write = disk.write(casched_block, rwbuffer.data(), MB.block_size);
    if (n_write != MB.block_size) {
        throw std::runtime_error("Error while write operaion.");
    }

    return length;
}

fsize DataBlock::read(address block_n, data* rdata, fsize offset, fsize length) {
    if (block_n >= MB.n_data_blocks || block_n < 0) {
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
}