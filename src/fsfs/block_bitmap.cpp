#include "block_bitmap.hpp"

#include <stdexcept>

namespace FSFS {
inline address BlockBitmap::calc_pos(address block_n) const {
    auto row = block_n / bitmap_row_length;
    return block_n - (bitmap_row_length * row);
}

bitmap_t* BlockBitmap::get_map_row(address block_n) {
    if (block_n >= n_blocks || block_n < 0) {
        throw std::invalid_argument("Block idx out of bound.");
    }
    return &bitmap.at(block_n / bitmap_row_length);
}

const bitmap_t& BlockBitmap::get_map_row(address block_n) const {
    if (block_n >= n_blocks || block_n < 0) {
        throw std::invalid_argument("Block idx out of bound.");
    }
    return bitmap.at(block_n / bitmap_row_length);
}

void BlockBitmap::resize(address n_blocks) {
    if (n_blocks <= 0) {
        throw std::invalid_argument("Size number cannot be equal or lower than 0.");
    }

    // Step 1: calculate needed space and prepare free
    //
    this->n_blocks = n_blocks;
    fsize map_size = n_blocks / bitmap_row_length;
    if ((n_blocks - map_size * bitmap_row_length) > 0) {
        map_size += 1;
    }
    bitmap.resize(map_size);
    std::fill_n(bitmap.data(), map_size, 0x00);

    // Step 2: Set as no free additional unused blocks
    //
    fsize n_unused_bits = map_size * bitmap_row_length - n_blocks;
    bitmap_t unused_bits_mask = (1UL << n_unused_bits) - 1;
    bitmap.at(map_size - 1) = unused_bits_mask << (std::numeric_limits<bitmap_t>::digits - n_unused_bits - 1);
}

void BlockBitmap::set_status(address block_n, bool status) {
    if (block_n < 0) {
        throw std::invalid_argument("Size number cannot be equal or lower than 0.");
    }

    if (n_blocks < 0) {
        throw std::runtime_error("Bitmap is not initialized.");
    }

    constexpr bitmap_t mask = (1UL << (bitmap_row_length - 1));
    bitmap_t* block_map = get_map_row(block_n);

    int32_t mask_shift = bitmap_row_length - calc_pos(block_n);
    if (status) {
        *block_map |= (mask >> mask_shift);
    } else {
        *block_map &= ~(mask >> mask_shift);
    }
}

bool BlockBitmap::get_status(address block_n) const {
    if (block_n < 0) {
        throw std::invalid_argument("Size number cannot be equal or lower than 0.");
    }

    if (n_blocks < 0) {
        throw std::runtime_error("Bitmap is not initialized.");
    }

    constexpr bitmap_t mask = (1UL << (bitmap_row_length - 1));
    auto block_map = get_map_row(block_n);

    int32_t mask_shift = bitmap_row_length - calc_pos(block_n);
    return block_map & (mask >> mask_shift);
}

address BlockBitmap::next_free(address block_offset) const {
    if (block_offset < 0) {
        throw std::invalid_argument("Size number cannot be equal or lower than 0.");
    }

    if (block_offset >= n_blocks) {
        throw std::runtime_error("Block offset is greater than block map size.");
    }

    // Step 1: Find row in bitmap with free blocks
    //
    size_t row = block_offset / bitmap_row_length;
    for (; row < bitmap.size(); row++) {
        if (bitmap.at(row) != std::numeric_limits<bitmap_t>::max()) {
            break;
        }
    }

    // Step 2: Check if no free blocks
    //
    if (row == bitmap.size()) {
        return -1;
    }

    // Step 3: Find free block in row
    //
    address pos_offset = calc_pos(block_offset);
    address real_idx = row * bitmap_row_length + pos_offset;
    for (auto i = pos_offset; i < bitmap_row_length; i++) {
        if (!get_status(real_idx)) {
            break;
        }
        real_idx++;
    }
    return real_idx;
}
}
