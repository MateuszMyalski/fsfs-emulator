#include "disk.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace FSFS {

Disk::Disk(fsize block_size) : mounted(0), block_size(block_size) {
    if ((block_size % quant_block_size) != 0) {
        throw std::invalid_argument(
            "Block size must be multiplication of 1024.");
    }
}

Disk::~Disk() { disk_img.close(); }

void Disk::open(const char* path) {
    if (disk_img.is_open() && is_mounted()) {
        throw std::runtime_error("Image already opened.");
    }

    disk_img.open(path, std::ios::out | std::ios::in | std::ios::binary);

    disk_img.seekg(0, std::ios::end);
    disk_img_size = disk_img.tellg();

    if ((disk_img_size % quant_block_size) != 0) {
        throw std::runtime_error("Image invalid size.");
    }
}

void Disk::create(const char* path, fsize n_blocks, fsize block_size) {
    std::fstream disk(path, std::ios::out | std::ios::binary);
    fsize disk_size = block_size * n_blocks;

    disk.seekp(disk_size - 1);
    disk.write("", 1);
}

fsize Disk::write(address block_n, const data* data_block, fsize data_len) {
    if (!(disk_img.is_open() && is_mounted())) {
        return -1;
    }

    address offset = block_n * block_size;
    fsize data_overflow = std::min(0, disk_img_size - (offset + data_len));
    data_len -= std::abs(data_overflow);

    disk_img.seekp(offset, disk_img.beg);
    disk_img.write(reinterpret_cast<const char*>(data_block), data_len);

    return data_len;
}

fsize Disk::read(address block_n, data* data_block, fsize data_len) {
    if (!(disk_img.is_open() && is_mounted())) {
        return -1;
    }

    address offset = block_n * block_size;
    fsize data_overflow = std::min(0, disk_img_size - (offset + data_len));
    data_len -= std::abs(data_overflow);

    disk_img.seekg(offset, disk_img.beg);
    disk_img.read(reinterpret_cast<char*>(data_block), data_len);

    return data_len;
}
}