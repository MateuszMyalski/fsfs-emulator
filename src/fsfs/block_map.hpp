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
    Disk& disk;
    v_size n_inode_blocks;
    v_size n_data_blocks;

    std::vector<uint64_t> inode_block_map;
    std::vector<uint64_t> data_block_map;

    uint64_t* get_map_line(v_size block_n, map_type map_type);
    void resize();

   public:
    constexpr static int32_t block_map_line_len =
        std::numeric_limits<uint64_t>::digits;

    BlockMap(Disk& disk) : disk(disk), n_inode_blocks(-1), n_data_blocks(-1){};

    void initialize(v_size n_data_blocks, v_size n_inode_blocks);

    template <map_type T>
    void scan_block();

    template <map_type T>
    void set_block(v_size block_n, block_status status);

    template <map_type T>
    block_status get_block_status(v_size block_n);
};

}
#endif