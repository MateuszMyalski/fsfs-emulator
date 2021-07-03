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

    void resize();

   public:
    constexpr static int32_t block_map_line_len =
        std::numeric_limits<uint64_t>::digits;

    enum class map_type { INODE, DATA };

    BlockMap(Disk& disk) : disk(disk), n_inode_blocks(-1), n_data_blocks(-1){};

    void initialize(v_size n_data_blocks, v_size n_inode_blocks);

    template <map_type T>
    void scan_block();

    template <map_type T>
    void set_block(v_size block_n, block_status status);

    template <map_type T>
    block_status get_block_status(v_size block_n);

    // template <map_type T>
    // void scan_block<map_type::DATA>();
    // template <map_type T>
    // void scan_block<map_type::INODE>();

    // template <map_type T>
    // void set_block<map_type::DATA>(v_size block_n, block_status status);
    // template <map_type T>
    // void set_block<map_type::INODE>(v_size block_n, block_status status);

    // template <map_type T>
    // block_status get_block_status<map_type::DATA>(v_size block_n);
    // template <map_type T>
    // block_status get_block_status<map_type::INODE>(v_size block_n);
};

}
#endif