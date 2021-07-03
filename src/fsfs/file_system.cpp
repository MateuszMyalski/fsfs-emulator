#include "file_system.hpp"

#include <exception>

#include "data_structs.hpp"

namespace FSFS {

void FileSystem::mount() {
    if (disk.is_mounted()) {
        throw std::runtime_error("Disk is already mounted.");
    }
    disk.mount();

    auto n_read = disk.read(super_block_offset, reinterpret_cast<data*>(&MB),
                            sizeof(super_block));

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

    // block_map.initialize(MB.block_size, MB.n_blocks);
}

void FileSystem::unmount() {
    disk.unmount();

    if (disk.is_mounted()) {
        throw std::runtime_error("Disk is not fully unmounted.");
    }
}
}