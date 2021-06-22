#ifndef DISK_EMULATOR_DISK_HPP
#define DISK_EMULATOR_DISK_HPP
#include <fstream>

#include "common/types.hpp"
namespace FSFS {
class Disk {
   private:
    int32_t mounted;
    v_size block_size;
    v_size disk_img_size = -1;
    std::fstream disk_img;

   public:
    Disk(v_size block_size);
    ~Disk();
    void open(const char* path);
    v_size write(v_size block_n, data* data_block, v_size data_len);
    v_size read(v_size block_n, data* data_block);
    v_size size() const { return block_size; };
    static void create(const char* path, v_size n_blocks, v_size block_size);

    bool is_mounted() const { return mounted; };
    void mount() { mounted++; };
    void unmount() {
        if (mounted > 0) {
            mounted--;
        }
    };
};
}
#endif