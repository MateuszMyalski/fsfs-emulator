#include "block_map.hpp"

#include <iostream>

#include "data_structs.hpp"

namespace FSFS {
namespace {
inline int32_t calc_row(v_size block_n) {
    return block_n / BlockMap::block_map_line_len;
}
inline int32_t calc_pos(v_size block_n) {
    auto row = block_n / BlockMap::block_map_line_len;
    return block_n - (BlockMap::block_map_line_len * row);
}
}

void BlockMap::initialize(v_size n_data_blocks, v_size n_inode_blocks) {
    if (n_data_blocks <= 0 || n_inode_blocks <= 0) {
        throw std::invalid_argument("Size cannot be equal or lower than 0.");
    }

    this->n_data_blocks = n_data_blocks;
    this->n_inode_blocks = n_inode_blocks;

    resize();
    std::fill_n(inode_block_map.data(), inode_block_map.capacity(), 0x00);
    std::fill_n(data_block_map.data(), data_block_map.capacity(), 0x00);
}

void BlockMap::resize() {
    int32_t inode_block_map_size = n_inode_blocks / block_map_line_len;
    int32_t data_block_map_size = n_data_blocks / block_map_line_len;

    if ((n_inode_blocks - inode_block_map_size * block_map_line_len) > 0) {
        inode_block_map_size += 1;
    }
    if ((n_data_blocks - data_block_map_size * block_map_line_len) > 0) {
        data_block_map_size += 1;
    }

    inode_block_map.resize(inode_block_map_size);
    data_block_map.resize(data_block_map_size);
}

uint64_t* BlockMap::get_map_line(v_size block_n, map_type map_type) {
    if (map_type == map_type::DATA) {
        if (block_n >= n_data_blocks || block_n < 0) {
            throw std::invalid_argument("Block idx out of bound.");
        }
        return &data_block_map.at(calc_row(block_n));
    } else {
        if (block_n >= n_inode_blocks || block_n < 0) {
            throw std::invalid_argument("Block idx out of bound.");
        }
        return &inode_block_map.at(calc_row(block_n));
    }
}

template <map_type T>
void BlockMap::set_block(v_size block_n, block_status status) {
    constexpr uint64_t mask = (1UL << (block_map_line_len - 1));
    uint64_t* block_map = get_map_line(block_n, T);

    int32_t mask_shift = block_map_line_len - calc_pos(block_n);

    if (status == block_status::USED) {
        *block_map |= (mask >> mask_shift);
    } else {
        *block_map &= ~(mask >> mask_shift);
    }
}

template <map_type T>
block_status BlockMap::get_block_status(v_size block_n) {
    constexpr uint64_t mask = (1UL << (block_map_line_len - 1));
    uint64_t* block_map = get_map_line(block_n, T);

    int32_t mask_shift = block_map_line_len - calc_pos(block_n);

    bool status = *block_map & (mask >> mask_shift);

    return status ? block_status::USED : block_status::FREE;
}

template void BlockMap::set_block<map_type::DATA>(v_size block_n,
                                                  block_status status);
template void BlockMap::set_block<map_type::INODE>(v_size block_n,
                                                   block_status status);

template block_status BlockMap::get_block_status<map_type::DATA>(
    v_size block_n);
template block_status BlockMap::get_block_status<map_type::INODE>(
    v_size block_n);

// template void BlockMap::scan_block<map_type::DATA>();
// template void BlockMap::scan_block<map_type::INODE>();

}
