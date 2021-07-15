#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include <common/types.hpp>
#include <cstring>

#include "block_bitmap.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class FileSystem {
   private:
    Disk& disk;
    super_block MB;
    BlockBitmap inode_bitmap;
    BlockBitmap data_bitmap;

    static uint32_t calc_mb_checksum(super_block& MB);
    void scan_blocks();

   public:
    FileSystem(Disk& disk) : disk(disk), inode_bitmap(), data_bitmap() {
        std::memset(&MB, -1, sizeof(super_block));
    };

    fsize read(address inode, data& data, fsize length, address offset);
    fsize write(address inode, data& data, fsize length, address offset);

    static void format(Disk& disk);
    void mount();
    void unmount();

    address create(const char* file_name);
    void remove(address inode_n);

    void set_inode(address inode_n, const inode_block& inode_block);
    void get_inode(address inode_n, inode_block& inode_block);

    void set_data_block(address data_n, const data& data_block);
    void get_data_block(address data_n, data& data_block);

    void stats();
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
