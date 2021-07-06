#ifndef FSFS_BLOCK_MAP_HPP
#define FSFS_BLOCK_MAP_HPP
#include <exception>
#include <limits>
#include <vector>

#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class BlockMap {
   private:
    fsize n_inode_blocks;
    fsize n_data_blocks;

    std::vector<uint64_t> inode_block_map;
    std::vector<uint64_t> data_block_map;

    uint64_t* get_map_line(address block_n, map_type map_type);
    void resize();

   public:
    constexpr static int32_t block_map_line_len =
        std::numeric_limits<uint64_t>::digits;

    BlockMap() : n_inode_blocks(-1), n_data_blocks(-1){};

    void initialize(fsize n_data_blocks, fsize n_inode_blocks);

    template <map_type T>
    void scan_block(Disk& disk);

    template <map_type T>
    void set_block(address block_n, block_status status);

    template <map_type T>
    block_status get_block_status(address block_n);
};

}
#endif