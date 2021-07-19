#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include <common/types.hpp>
#include <cstring>

#include "disk-emulator/disk.hpp"
#include "memory_io.hpp"

namespace FSFS {
class FileSystem {
   private:
    Disk& disk;
    super_block MB;
    MemoryIO io;

    static uint32_t calc_mb_checksum(super_block& MB);

   public:
    FileSystem(Disk& disk) : disk(disk), io(disk) {
        std::memset(&MB, -1, sizeof(super_block));
    };

    fsize read(address inode_n, data& rdata, fsize length, address offset);
    fsize write(address inode_n, const data& wdata, fsize length,
                address offset);

    static void format(Disk& disk);
    static void read_super_block(Disk& disk, super_block& MB);
    void mount();
    void unmount();

    address create(const char* file_name);
    void remove(address inode_n);

    void stats();
};
}
#endif
