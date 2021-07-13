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

   public:
    FileSystem(Disk& disk) : disk(disk), inode_bitmap(), data_bitmap() {
        std::memset(&MB, -1, sizeof(super_block));
    };
    void stats();

    fsize read(address inode, data& data, fsize length, address offset);
    fsize write(address inode, data& data, fsize length, address offset);

    static void format(Disk& disk);
    void mount();
    void unmount();

    fsize create();
    fsize remove();
    fsize erease();

    void set_inode(address inode_n, const inode_block& inode_block);
    void get_inode(address inode_n, inode_block& inode_block);

    void set_data_block(address data_n, const data& data_block);
    void get_data_block(address data_n, data& data_block);

    const super_block& get_super_block() const {
        return const_cast<const super_block&>(MB);
    }
};
}
#endif
