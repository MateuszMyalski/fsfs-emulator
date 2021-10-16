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

    static void read_super_block(Disk& disk, super_block& MB);
    static uint32_t calc_mb_checksum(super_block& MB);
    int32_t edit_data(int32_t inode_n, const uint8_t* wdata, int32_t offset, int32_t length);
    void scan_blocks();
    void set_data_blocks_status(int32_t inode_n, bool status);

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

    void mount();
    void unmount();

    int32_t create_file(const char* file_name);
    int32_t remove_file(int32_t inode_n);

    int32_t rename_file(int32_t inode_n, const char* file_name);
    int32_t get_file_length(int32_t inode_n);
    int32_t get_file_name(int32_t inode_n, char* file_name_buffer);

    int32_t write(int32_t inode_n, const uint8_t* wdata, int32_t offset, int32_t length);
    int32_t read(int32_t inode_n, uint8_t* rdata, int32_t offset, int32_t length);

    decltype(auto) get_inode_bitmap() const { return get_inode_bitmap_common(this); }
    decltype(auto) get_data_bitmap() const { return get_data_bitmap_common(this); }
};
}
#endif