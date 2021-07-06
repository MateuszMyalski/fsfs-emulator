#ifndef DISK_EMULATOR_DISK_HPP
#define DISK_EMULATOR_DISK_HPP
#include <fstream>
#include <memory>

#include "common/types.hpp"
namespace FSFS {
constexpr fsize quant_block_size = 1024;

class Disk {
   private:
    int32_t mounted;
    fsize block_size;
    fsize disk_img_size;
    std::fstream disk_img;

   public:
    Disk(fsize block_size);
    ~Disk();
    void open(const char* path);
    fsize write(address block_n, const data* data_block, fsize data_len);
    fsize read(address block_n, data* data_block, fsize data_len);
    fsize get_block_size() const { return block_size; };
    fsize get_disk_size() const { return (disk_img_size / block_size); };
    static void create(const char* path, fsize n_blocks, fsize block_size);

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