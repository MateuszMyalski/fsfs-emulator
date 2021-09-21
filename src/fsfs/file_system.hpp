#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include "block.hpp"
#include "block_bitmap.hpp"
#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"
#include "indirect_inode.hpp"
#include "inode.hpp"

namespace FSFS {
class FileSystem {
   private:
    Disk& disk;
    super_block MB;
    BlockBitmap inode_bitmap;
    BlockBitmap data_bitmap;
    Block block;
    Inode inode;

    void set_data_blocks_status(address inode_n, bool status);
    static uint32_t calc_mb_checksum(super_block& MB);
    fsize edit_data(address inode_n, const data* wdata, fsize offset, fsize length);
    void scan_blocks();

    template <typename Self>
    static decltype(auto) get_inode_bitmap_common(Self* self) {
        return (self->inode_bitmap);
    }

    template <typename Self>
    static decltype(auto) get_data_bitmap_common(Self* self) {
        return (self->data_bitmap);
    }

   public:
    FileSystem(Disk& disk) : disk(disk), MB(), inode_bitmap(), data_bitmap(), block(disk, MB), inode(){};

    static void format(Disk& disk);
    static void read_super_block(Disk& disk, super_block& MB);

    void mount();
    void unmount();

    address create_file(const char* file_name);
    address remove_file(address inode_n);

    address rename_file(address inode_n, const char* file_name);
    fsize get_file_length(address inode_n);
    address get_file_name(address inode_n, char* file_name_buffer);

    fsize write(address inode_n, const data* wdata, address offset, fsize length);
    fsize read(address inode_n, data* rdata, address offset, fsize length);

    decltype(auto) get_inode_bitmap() const { return get_inode_bitmap_common(this); }
    decltype(auto) get_data_bitmap() const { return get_data_bitmap_common(this); }
};
}
#endif