#ifndef DISK_EMULATOR_DISK_HPP
#define DISK_EMULATOR_DISK_HPP
#include <fstream>
#include <memory>

#include "common/types.hpp"
namespace FSFS {
constexpr v_size quant_block_size = 1024;

class Disk {
   private:
    int32_t mounted;
    v_size block_size;
    v_size disk_img_size;
    std::fstream disk_img;

   public:
    Disk(v_size block_size);
    ~Disk();
    void open(const char* path);
    v_size write(v_size block_n, data* data_block, v_size data_len);
    v_size read(v_size block_n, data* data_block, v_size data_len);
    v_size get_block_size() const { return block_size; };
    v_size get_disk_size() const { return (disk_img_size / block_size); };
    static void create(const char* path, v_size n_blocks, v_size block_size);

    bool is_mounted() const { return mounted; };
    void mount() { mounted++; };
    void unmount() {
        if (mounted > 0) {
            mounted--;
        }
    };
};

typedef std::unique_ptr<Disk> disk_ptr;
}
#endif