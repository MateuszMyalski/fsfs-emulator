#include "block_bitmap.hpp"

#include <stdexcept>

namespace FSFS {
inline address BlockBitmap::calc_pos(address block_n) {
    auto row = block_n / bitmap_row_length;
    return block_n - (bitmap_row_length * row);
}

bitmap_t* BlockBitmap::get_map_row(address block_n) {
    if (block_n >= n_blocks || block_n < 0) {
        throw std::invalid_argument("Block idx out of bound.");
    }
    return &bitmap.at(block_n / bitmap_row_length);
}

void BlockBitmap::init(address n_blocks) {
    if (n_blocks <= 0) {
        throw std::invalid_argument(
            "Size number cannot be equal or lower than 0.");
    }

    this->n_blocks = n_blocks;
    bitmap.resize(n_blocks);
    std::fill_n(bitmap.data(), n_blocks, 0x00);
}

void BlockBitmap::set_block(address block_n, bool status) {
    if (block_n < 0) {
        throw std::invalid_argument(
            "Size number cannot be equal or lower than 0.");
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

bool BlockBitmap::get_block_status(address block_n) {
    if (block_n < 0) {
        throw std::invalid_argument(
            "Size number cannot be equal or lower than 0.");
    }

    if (n_blocks < 0) {
        throw std::runtime_error("Bitmap is not initialized.");
    }

    constexpr bitmap_t mask = (1UL << (bitmap_row_length - 1));
    bitmap_t* block_map = get_map_row(block_n);

    int32_t mask_shift = bitmap_row_length - calc_pos(block_n);

    return *block_map & (mask >> mask_shift);
}
}