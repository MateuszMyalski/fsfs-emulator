#ifndef FSFS_BLOCK_BITMAP_HPP
#define FSFS_BLOCK_BITMAP_HPP
#include <limits>
#include <vector>

#include "common/types.hpp"

namespace FSFS {
using bitmap_t = uint64_t;
class BlockBitmap {
   private:
    int32_t n_blocks;
    std::vector<bitmap_t> bitmap;

    constexpr static auto bitmap_row_length = std::numeric_limits<bitmap_t>::digits;

    inline int32_t calc_pos(int32_t block_n) const;
    const bitmap_t& get_map_row(int32_t block_n) const;
    bitmap_t* get_map_row(int32_t block_n);

   public:
    BlockBitmap() : n_blocks(-1){};
    BlockBitmap(int32_t n_blocks) : n_blocks(n_blocks) { resize(n_blocks); };

    void resize(int32_t n_blocks);
    void set_status(int32_t block_n, bool status);
    bool get_status(int32_t block_n) const;

    int32_t next_free(int32_t block_offset) const;
};
}
#endif
