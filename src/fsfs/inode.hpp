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
    address casched_n_block;
    inode_block* inodes;
    fsize n_inodes_in_block;
    struct {
        address low_block_n;
        address high_block_n;
    } cashed_inodes;

    address read_inode(address inode_n);

   public:
    Inode(Disk& disk, const super_block& MB);
    ~Inode();
    const inode_block& get_inode(address inode_n);
    inode_block& update_inode(address inode_n);

    address alloc_inode(address inode_n);

    address get_nth_ptr(address inode_n, address ptr_n);
    address set_nth_ptr(address inode_n, address ptr_n, address ptr);

    void commit_inode();
};
}

#endif