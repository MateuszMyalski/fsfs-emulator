#include "block_map.hpp"

#include <iostream>

#include "data_structs.hpp"

namespace FSFS {
namespace {
inline int32_t calc_row(fsize block_n) {
    return block_n / BlockMap::block_map_line_len;
}
inline int32_t calc_pos(fsize block_n) {
    auto row = block_n / BlockMap::block_map_line_len;
    return block_n - (BlockMap::block_map_line_len * row);
}
}

void BlockMap::initialize(fsize n_data_blocks, fsize n_inode_blocks) {
    if (n_data_blocks <= 0 || n_inode_blocks <= 0) {
        throw std::invalid_argument("Size cannot be equal or lower than 0.");
    }

    this->n_data_blocks = n_data_blocks;
    this->n_inode_blocks = n_inode_blocks;

    fsize inode_block_map_size = n_inode_blocks / block_map_line_len;
    fsize data_block_map_size = n_data_blocks / block_map_line_len;

    if ((n_inode_blocks - inode_block_map_size * block_map_line_len) > 0) {
        inode_block_map_size += 1;
    }
    if ((n_data_blocks - data_block_map_size * block_map_line_len) > 0) {
        data_block_map_size += 1;
    }

    inode_block_map.resize(inode_block_map_size);
    data_block_map.resize(data_block_map_size);

    std::fill_n(inode_block_map.data(), inode_block_map_size, 0x00);
    std::fill_n(data_block_map.data(), data_block_map_size, 0x00);
}

uint64_t* BlockMap::get_map_line(address block_n, map_type map_type) {
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
void BlockMap::set_block(address block_n, block_status status) {
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
block_status BlockMap::get_block_status(address block_n) {
    constexpr uint64_t mask = (1UL << (block_map_line_len - 1));
    uint64_t* block_map = get_map_line(block_n, T);

    int32_t mask_shift = block_map_line_len - calc_pos(block_n);

    bool status = *block_map & (mask >> mask_shift);

    return status ? block_status::USED : block_status::FREE;
}

void BlockMap::indirect_data_scan(Disk& disk, address block_n,
                                  fsize block_size) {
    if (block_n == inode_empty_ptr) {
        return;
    }

    std::vector<data> block;
    block.resize(block_size);

    disk.mount();
    disk.read(block_n, block.data(), block_size);
    disk.unmount();

    const address* block_ptrs = reinterpret_cast<address*>(block.data());

    for (auto i = 0; i < block_size - 1; i++) {
        if (block_ptrs[i] == inode_empty_ptr) {
            return;
        }
        set_block<map_type::DATA>(block_ptrs[i], block_status::USED);
    }

    indirect_data_scan(disk, block_ptrs[block_size - 1], block_size);
}

void BlockMap::scan_blocks(Disk& disk, const super_block& MB) {
    int32_t inode_blocks_per_block = MB.block_size / meta_block_size;
    initialize(MB.n_data_blocks, MB.n_inode_blocks * inode_blocks_per_block);

    for (address nth_block = inode_blocks_offset; nth_block < MB.n_inode_blocks;
         nth_block++) {
        std::vector<data> block;
        block.resize(MB.block_size);
        const meta_block* inodes = reinterpret_cast<meta_block*>(block.data());

        disk.mount();
        disk.read(nth_block, block.data(), MB.block_size);
        disk.unmount();

        for (address nth_inode = 0; nth_inode < MB.block_size / meta_block_size;
             nth_inode++) {
            if (inodes[nth_inode].inode.type == block_status::FREE) {
                continue;
            }

            set_block<map_type::INODE>(
                nth_inode + nth_block - inode_blocks_offset,
                block_status::USED);

            for (auto i = 0; i < inode_n_block_ptr_len; i++) {
                address addr = inodes[nth_inode].inode.block_ptr[i];
                if (addr == inode_empty_ptr) {
                    break;
                }

                set_block<map_type::DATA>(addr, block_status::USED);
            }

            indirect_data_scan(disk, inodes[nth_inode].inode.indirect_inode_ptr,
                               MB.block_size);
        }
    }
}

template void BlockMap::set_block<map_type::DATA>(address block_n,
                                                  block_status status);
template void BlockMap::set_block<map_type::INODE>(address block_n,
                                                   block_status status);

template block_status BlockMap::get_block_status<map_type::DATA>(
    address block_n);
template block_status BlockMap::get_block_status<map_type::INODE>(
    address block_n);
}
