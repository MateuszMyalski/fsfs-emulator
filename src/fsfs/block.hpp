#ifndef FSFS_DATA_BLOCK_HPP
#define FSFS_DATA_BLOCK_HPP
#include <vector>

#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class Block {
   private:
    Disk& disk;
    const super_block& MB;
    std::vector<uint8_t> rwbuffer;
    int32_t casched_block;

    int32_t read_block(int32_t block_n);

   public:
    Block(Disk& disk, const super_block& MB);
    ~Block();

    void resize();
    int32_t write(int32_t block_n, const uint8_t* wdata, int32_t offset, int32_t length);
    int32_t read(int32_t block_n, uint8_t* rdata, int32_t offset, int32_t length);

    int32_t get_block_size();
    int32_t get_n_addreses_in_block();
    int32_t get_n_inodes_in_block();
    int32_t inode_n_to_block_n(int32_t inode_n);
    int32_t data_n_to_block_n(int32_t data_n);
    int32_t bytes_to_blocks(int32_t length);
};
}
#endif
