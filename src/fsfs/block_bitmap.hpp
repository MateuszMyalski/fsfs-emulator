#ifndef FSFS_BLOCK_BITMAP_HPP
#define FSFS_BLOCK_BITMAP_HPP
#include <limits>
#include <vector>

#include "common/types.hpp"

namespace FSFS {
using bitmap_t = uint64_t;
class BlockBitmap {
   private:
    address n_blocks;
    std::vector<bitmap_t> bitmap;

    constexpr static auto bitmap_row_length =
        std::numeric_limits<bitmap_t>::digits;

    inline address calc_pos(address block_n);
    bitmap_t* get_map_row(address block_n);

   public:
    BlockBitmap() : n_blocks(-1){};

    void init(address n_blocks);
    void set_block(address block_n, bool status);
    bool get_block_status(address block_n);
};
}
#endif
