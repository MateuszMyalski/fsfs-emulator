#ifndef FSFS_MEMORY_IO_HPP
#define FSFS_MEMORY_IO_HPP
#include "block_bitmap.hpp"
#include "common/types.hpp"
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
    IndirectInode iinode;
    Inode inode;

    void set_data_blocks_status(address inode_n, bool allocated);

   public:
    MemoryIO(Disk& disk)
        : disk(disk),
          MB(),
          inode_bitmap(),
          data_bitmap(),
          iinode(disk, MB),
          inode(disk, MB){};

    void init(const super_block& MB);

    address alloc_inode(const char* file_name);
    address dealloc_inode(address inode_n);

    address rename_inode(address inode_n, const char* file_name);
    fsize get_inode_length(address inode_n);
    address get_inode_file_name(address inode_n, char* file_name_buffer);

    address add_data(address inode_n, const data& wdata, fsize length);
    address edit_data(address inode_n, const data& wdata, address offset,
                      fsize length);

    void scan_blocks();
    fsize bytes_to_blocks(fsize length);

    const BlockBitmap& get_inode_bitmap() const {
        return const_cast<const BlockBitmap&>(inode_bitmap);
    };
    const BlockBitmap& get_data_bitmap() const {
        return const_cast<const BlockBitmap&>(data_bitmap);
    };
};
}
#endif