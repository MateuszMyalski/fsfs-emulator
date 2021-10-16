#ifndef FSFS_INODE_HPP
#define FSFS_INODE_HPP
#include "block.hpp"
#include "block_bitmap.hpp"
#include "indirect_inode.hpp"
namespace FSFS {
using PtrsLList = std::forward_list<int32_t>;
class Inode {
   private:
    int32_t loaded_inode_n;

    IndirectInode indirect_inode;
    inode_block inode;
    inode_block inode_buf;
    PtrsLList ptrs_to_allocate;

    void load_direct(int32_t inode_n, Block& data_block);

   public:
    Inode();

    inode_block const& meta() const;
    inode_block& meta();

    int32_t ptr(int32_t ptr_n) const;
    int32_t last_indirect_ptr(int32_t indirect_ptr_n) const;

    void add_data(int32_t new_data_n);
    void alloc_new(int32_t inode_n);

    void clear();
    void load(int32_t inode_n, Block& data_block);
    int32_t commit(Block& data_block, BlockBitmap& data_bitmap);
};
}

#endif