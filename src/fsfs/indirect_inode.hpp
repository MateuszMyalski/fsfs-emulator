#ifndef FSFS_INDIRECT_INODE_HPP
#define FSFS_INDIRECT_INODE_HPP
#include <forward_list>
#include <vector>

#include "block.hpp"
#include "block_bitmap.hpp"
#include "common/types.hpp"
#include "data_structs.hpp"

namespace FSFS {
using PtrsLList = std::forward_list<int32_t>;
class IndirectInode {
   private:
    const inode_block& inode;
    std::forward_list<int32_t> indirect_block_n;
    std::vector<int32_t> indirect_ptrs_list;

   public:
    IndirectInode(const inode_block& inode);

    int32_t ptr(int32_t ptr_n) const;
    int32_t last_indirect_ptr(int32_t indirect_ptr_n) const;

    void clear();
    void load(Block& data_block);
    int32_t commit(Block& data_block, BlockBitmap& data_bitmap, PtrsLList& ptrs_to_allocate);
};
}
#endif