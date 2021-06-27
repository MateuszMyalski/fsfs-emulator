#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include <common/types.hpp>
#include <cstring>

#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"
namespace FSFS {
class FileSystem {
   public:
    FileSystem() {
        mounted_disk.reset(nullptr);
        std::memset(&MB, -1, sizeof(super_block));
    };
    void stats();

    v_size read(v_size inode, data& data, v_size length, v_size offset);
    v_size write(v_size inode, data& data, v_size length, v_size offset);

    void format();
    void mount(disk_ptr disk);
    void unmount();

    v_size create();
    v_size remove();
    v_size erease();

   private:
    disk_ptr mounted_disk;
    super_block MB;
};
}
#endif
