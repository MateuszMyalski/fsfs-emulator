#ifndef FSFS_INODE_HPP
#define FSFS_INODE_HPP
#include <vector>

#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class Inode {
   private:
    Disk& disk;
    const super_block& MB;

    std::vector<data> rwbuffer;
    inode_block* inodes;
    fsize n_inodes_in_block;
    struct {
        address nth_block;
        address low_block_n;
        address high_block_n;
    } casch_info;

    address read_inode(address inode_n);

   public:
    Inode(Disk& disk, const super_block& MB);
    ~Inode();
    const inode_block& get(address inode_n);
    inode_block& update(address inode_n);

    address alloc(address inode_n);

    void commit();
};
}

#endif