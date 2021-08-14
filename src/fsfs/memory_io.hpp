#ifndef FSFS_MEMORY_IO_HPP
#define FSFS_MEMORY_IO_HPP
#include "block_bitmap.hpp"
#include "common/types.hpp"
#include "data_block.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"
#include "indirect_inode.hpp"
#include "inode.hpp"

namespace FSFS {
class MemoryIO {
   private:
    Disk& disk;
    super_block MB;
    BlockBitmap inode_bitmap;
    BlockBitmap data_bitmap;
    DataBlock data_block;
    IndirectInode iinode;
    Inode inode;

    void set_data_blocks_status(address inode_n, bool allocated);

    decltype(auto) get_abs_addr(address inode_n, fsize ptr_n);
    decltype(auto) set_abs_addr(address inode_n, fsize ptr_n);

    address expand_indirect(address data_n);
    address expand_inode(address inode_n);
    address assign_data_block(address inode_n, fsize ptr_n);

    fsize store_data(address data_n, const data* wdata, fsize length);

    template <typename Self>
    static decltype(auto) get_inode_bitmap_common(Self* self) {
        return (self->inode_bitmap);
    }

    template <typename Self>
    static decltype(auto) get_data_bitmap_common(Self* self) {
        return (self->data_bitmap);
    }

   public:
    MemoryIO(Disk& disk)
        : disk(disk),
          MB(),
          inode_bitmap(),
          data_bitmap(),
          data_block(disk, MB),
          iinode(disk, MB),
          inode(disk, MB){};

    void init(const super_block& MB);

    address alloc_inode(const char* file_name);
    address dealloc_inode(address inode_n);

    address rename_inode(address inode_n, const char* file_name);
    fsize get_inode_length(address inode_n);
    address get_inode_file_name(address inode_n, char* file_name_buffer);

    fsize write_data(address inode_n, const data* wdata, address offset,
                     fsize length);
    fsize read_data(address inode_n, const data* wdata, address offset,
                    fsize length);

    void scan_blocks();
    fsize bytes_to_blocks(fsize length);

    decltype(auto) get_inode_bitmap() const {
        return get_inode_bitmap_common(this);
    }

    decltype(auto) get_data_bitmap() const {
        return get_data_bitmap_common(this);
    }
};
}
#endif