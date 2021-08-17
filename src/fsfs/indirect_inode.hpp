#ifndef FSFS_INDIRECT_INODE_HPP
#define FSFS_INDIRECT_INODE_HPP
#include <vector>

#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class IndirectInode {
   private:
    Disk& disk;
    const super_block& MB;
    address data_block_offset;

    std::vector<data> rwbuffer;
    address* indirect_ptr;
    fsize n_ptrs_in_block;

    struct {
        address nth_block;
        address nth_indirect;
        address base_addr;
        address low_ptr_n;
        address high_ptr_n;

        inline void reset() {
            nth_block = fs_nullptr;
            high_ptr_n = fs_nullptr;
            low_ptr_n = fs_nullptr;
            base_addr = fs_nullptr;
            nth_indirect = fs_nullptr;
        }

        inline bool is_valid() {
            return (nth_block == fs_nullptr) || (base_addr == fs_nullptr) || (nth_indirect == fs_nullptr);
        }

        inline bool is_cashed(address ptr_n) { return (ptr_n <= high_ptr_n) && (ptr_n >= low_ptr_n); }
    } casch_info;

    address read_indirect(address base_addr, address ptr_n);
    address random_read(address data_n);

   public:
    IndirectInode(Disk& disk, const super_block& MB);
    ~IndirectInode();

    const address& get_ptr(address base_addr, address ptr_n);
    address& set_ptr(address base_addr, address ptr_n);

    address get_block_address(address base_addr, address ptr_n);
    address set_indirect_address(address data_n, address ptr_addr);

    address get_n_ptrs_in_block();
    address alloc(address data_n);
    void commit();
    void reinit();
};
}
#endif