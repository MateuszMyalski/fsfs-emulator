#ifndef FSFS_INDIRECT_INODE_HPP
#define FSFS_INDIRECT_INODE_HPP
#include <forward_list>
#include <vector>

#include "block.hpp"
#include "block_bitmap.hpp"
#include "common/types.hpp"
#include "data_structs.hpp"

namespace FSFS {
using PtrsLList = std::forward_list<address>;
class IndirectInode {
   private:
    const inode_block& inode;
    std::forward_list<address> indirect_block_n;
    std::vector<address> indirect_ptrs_list;

   public:
    IndirectInode(const inode_block& inode);

    address ptr(address ptr_n) const;
    address last_indirect_ptr(address indirect_ptr_n) const;

    void clear();
    void load(Block& data_block);
    fsize commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate);
};
}
#endif