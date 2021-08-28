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
    address last_indirect_block_addr;
    std::vector<address> indirect_ptrs_list;

   public:
    IndirectInode(const inode_block& inode);

    const address& operator[](address ptr_n) const;

    void clear();
    void load(Block& data_block);
    void commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate);
};
}
#endif