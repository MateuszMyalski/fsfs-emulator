#ifndef FSFS_INODE_HPP
#define FSFS_INODE_HPP
#include "indirect_inode.hpp"
namespace FSFS {
class Inode {
   private:
    address loaded_inode_n;

    IndirectInode indirect_inode;
    inode_block inode;
    inode_block inode_buf;
    std::forward_list<address> ptrs_to_allocate;

    void load_direct(address inode_n, Block& data_block);

   public:
    Inode();

    inode_block const& meta() const;
    inode_block& meta();
    address const& operator[](address ptr_n) const;
    void add_data(address new_data_n);
    void alloc_new(address inode_n);

    void clear();
    void load(address inode_n, Block& data_block);
    void commit(Block& data_block);
};

}

#endif