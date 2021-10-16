#ifndef DISK_EMULATOR_DISK_HPP
#define DISK_EMULATOR_DISK_HPP
#include <fstream>
#include <memory>

#include "common/types.hpp"
namespace FSFS {
constexpr int32_t quant_block_size = 1024;

class Disk {
   private:
    int32_t mounted;
    int32_t block_size;
    int32_t disk_img_size;
    std::fstream disk_img;

   public:
    Disk(int32_t block_size);
    ~Disk();
    void open(const char* path);
    int32_t write(int32_t block_n, const uint8_t* data_block, int32_t data_len);
    int32_t read(int32_t block_n, uint8_t* data_block, int32_t data_len);
    int32_t get_block_size() const { return block_size; };
    int32_t get_disk_size() const { return (disk_img_size / block_size); };
    static void create(const char* path, int32_t n_blocks, int32_t block_size);

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
