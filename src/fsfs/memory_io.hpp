#ifndef FSFS_MEMORY_IO_HPP
#define FSFS_MEMORY_IO_HPP
#include "block.hpp"
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
    Block block;
    Inode inode;

    void set_data_blocks_status(address inode_n, bool status);

    fsize edit_data(address inode_n, const data* wdata, fsize offset, fsize length);

    template <typename Self>
    static decltype(auto) get_inode_bitmap_common(Self* self) {
        return (self->inode_bitmap);
    }

    template <typename Self>
    static decltype(auto) get_data_bitmap_common(Self* self) {
        return (self->data_bitmap);
    }

   public:
    MemoryIO(Disk& disk) : disk(disk), MB(), inode_bitmap(), data_bitmap(), block(disk, MB), inode(){};

    void init(const super_block& MB);

    address alloc_inode(const char* file_name);
    address dealloc_inode(address inode_n);

    address rename_inode(address inode_n, const char* file_name);
    fsize get_inode_length(address inode_n);
    address get_inode_file_name(address inode_n, char* file_name_buffer);

    fsize write_data(address inode_n, const data* wdata, address offset, fsize length);
    fsize read_data(address inode_n, data* rdata, address offset, fsize length);

    void scan_blocks();

    decltype(auto) get_inode_bitmap() const { return get_inode_bitmap_common(this); }

    decltype(auto) get_data_bitmap() const { return get_data_bitmap_common(this); }
};
}
#endif