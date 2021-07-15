#ifndef FSFS_MEMORY_IO_HPP
#define FSFS_MEMORY_IO_HPP
#include "block_bitmap.hpp"
#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class MemoryIO {
   private:
    Disk& disk;
    super_block MB;
    BlockBitmap inode_bitmap;
    BlockBitmap data_bitmap;

   public:
    MemoryIO(Disk& disk) : disk(disk), MB(), inode_bitmap(), data_bitmap() {
        disk.mount();
    };
    ~MemoryIO() { disk.unmount(); };

    void init(const super_block& MB);

    void set_inode(address inode_n, const inode_block& inode_block);
    void get_inode(address inode_n, inode_block& inode_block);

    void set_data_block(address data_n, const data& data_block);
    void get_data_block(address data_n, data& data_block);

    void scan_blocks();
    void set_data_blocks_status(const inode_block& inode_data, bool allocated);

    const BlockBitmap& get_inode_bitmap() const {
        return const_cast<const BlockBitmap&>(inode_bitmap);
    };
    const BlockBitmap& get_data_bitmap() const {
        return const_cast<const BlockBitmap&>(data_bitmap);
    };
};
}
#endif