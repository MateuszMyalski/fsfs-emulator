#include "file_system.hpp"

#include <exception>

#include "data_structs.hpp"

namespace FSFS {

void FileSystem::mount(disk_ptr disk) {
    mounted_disk = std::move(disk);
    if (mounted_disk->is_mounted()) {
        mounted_disk.release();
        throw std::runtime_error("Disk is already mounted.");
    }
    mounted_disk->mount();

    auto n_read = mounted_disk->read(
        super_block_offset, reinterpret_cast<data*>(&MB), sizeof(super_block));

    if (n_read != sizeof(super_block)) {
        throw std::runtime_error("Cannot read super block");
    }

    // TODO check magic number

    if ((MB.block_size % quant_block_size) != 0) {
        throw std::runtime_error("Block size must be multiplication of 1024.");
    }

    if (MB.n_blocks <= 0) {
        throw std::runtime_error("Invalid amount of blocks number.");
    }
}

void FileSystem::unmount() {
    mounted_disk->unmount();

    if (mounted_disk->is_mounted()) {
        mounted_disk.release();
        throw std::runtime_error("Disk is not fully unmounted.");
    }
    mounted_disk.release();
}
}