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
    std::vector<data> rwbuffer;
    address casched_block;

    address read_block(address block_n);

   public:
    Block(Disk& disk, const super_block& MB);
    ~Block();

    void reinit();
    fsize write(address block_n, const data* wdata, fsize offset, fsize length);
    fsize read(address block_n, data* rdata, fsize offset, fsize length);

    fsize get_block_size();
    fsize get_n_addreses_in_block();
    fsize get_n_inodes_in_block();
    address inode_n_to_block_n(address inode_n);
    address data_n_to_block_n(address data_n);
    fsize bytes_to_blocks(fsize length);
};
}
#endif
