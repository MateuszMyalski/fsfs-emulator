#include "disk.hpp"

#include <iostream>
#include <stdexcept>
#include <vector>
namespace FSFS {
constexpr v_size quant_block_size = 1024;
Disk::Disk(v_size block_size) : mounted(0), block_size(block_size) {
    if ((block_size % quant_block_size) != 0) {
        throw std::invalid_argument(
            "Block size must to be multiplication of 1024.");
    }
}

Disk::~Disk() { disk_img.close(); }

void Disk::open(const char* path) {
    if (disk_img.is_open() || is_mounted()) {
        throw std::runtime_error("Image already opened.");
    }

    disk_img.open(path, std::ios::out | std::ios::in | std::ios::binary);

    disk_img.seekg(0, std::ios::end);
    disk_img_size = disk_img.tellg();

    if ((disk_img_size % quant_block_size) != 0) {
        throw std::runtime_error("Image invalid size.");
    }
}

void Disk::create(const char* path, v_size n_blocks, v_size block_size) {
    std::fstream disk(path, std::ios::out | std::ios::binary);
    v_size disk_size = block_size * n_blocks;

    disk.seekp(disk_size - 1);
    disk.write("", 1);
}

v_size Disk::write(v_size block_n, data* data_block, v_size data_len) {
    if (!(disk_img.is_open() && is_mounted())) {
        return -1;
    }

    v_size offset = block_n * block_size;
    v_size data_overflow = std::min(0, disk_img_size - (offset + data_len));
    data_len -= std::abs(data_overflow);

    disk_img.seekp(offset, disk_img.beg);
    disk_img.write(data_block, data_len);

    return data_len;
}

v_size Disk::read(v_size block_n, data* data_block) {}

}