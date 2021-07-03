#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include <common/types.hpp>
#include <cstring>

#include "block_map.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class FileSystem {
   public:
    FileSystem(Disk& disk) : disk(disk), block_map() {
        std::memset(&MB, -1, sizeof(super_block));
    };
    void stats();

    v_size read(v_size inode, data& data, v_size length, v_size offset);
    v_size write(v_size inode, data& data, v_size length, v_size offset);

    void format();
    void mount();
    void unmount();

    v_size create();
    v_size remove();
    v_size erease();

   private:
    Disk& disk;
    BlockMap block_map;
    super_block MB;
};
}
#endif
