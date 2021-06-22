#ifndef FSFS_FILE_SYSTEM_HPP
#define FSFS_FILE_SYSTEM_HPP
#include <common/types.hpp>
namespace FSFS {
class FileSystem {
   public:
    static void stats();

    v_size read(v_size inode, uint8_t& data, v_size length, v_size offset);
    v_size write(v_size inode, uint8_t& data, v_size length, v_size offset);

    void format();
    void mount(Disk& disk);
    void unmount();

    v_size create();
    v_size remove();
    v_size erease();

   private:
};
}

#endif