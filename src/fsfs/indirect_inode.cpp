#include "indirect_inode.hpp"

namespace FSFS {

IndirectInode::IndirectInode(Disk& disk, const super_block& MB)
    : disk(disk), MB(MB) {
    disk.mount();
    rwbuffer.resize(MB.block_size);
    indirect_ptr = reinterpret_cast<address*>(rwbuffer.data());

    casch_info.reset();

    n_ptrs_in_block = MB.block_size / sizeof(address);
}

IndirectInode::~IndirectInode() { disk.unmount(); }

address IndirectInode::read_indirect(address base_addr, address ptr_n) {
    if (((ptr_n <= casch_info.high_ptr_n) && (ptr_n >= casch_info.low_ptr_n)) &&
        (casch_info.nth_block != fs_nullptr) &&
        (casch_info.base_addr != fs_nullptr) &&
        (casch_info.nth_indirect != fs_nullptr)) {
        return ptr_n;
    }

    address nth_indirect = ptr_n / (n_ptrs_in_block - 1);
    address addr_to_read = base_addr;
    address block_idx = 0;

    if ((base_addr == casch_info.base_addr) &&
        (ptr_n > casch_info.high_ptr_n)) {
        addr_to_read = indirect_ptr[n_ptrs_in_block - 1];
        block_idx = casch_info.nth_indirect + 1;
    }

    do {
        if (addr_to_read == fs_nullptr) {
            casch_info.reset();
            return fs_nullptr;
        }

        casch_info.nth_block = data_block_offset + addr_to_read;
        fsize n_read =
            disk.read(casch_info.nth_block, rwbuffer.data(), MB.block_size);
        if (n_read != MB.block_size) {
            throw std::runtime_error("Error while read operation.");
        }

        addr_to_read = indirect_ptr[n_ptrs_in_block - 1];
        block_idx++;
    } while (block_idx <= nth_indirect);

    casch_info.low_ptr_n = (n_ptrs_in_block - 1) * nth_indirect;
    casch_info.high_ptr_n = casch_info.low_ptr_n + (n_ptrs_in_block - 2);
    casch_info.nth_indirect = nth_indirect;
    casch_info.base_addr = base_addr;
}

address IndirectInode::random_read(address data_n) {
    address nth_block = data_block_offset + data_n;

    if (nth_block == casch_info.nth_block) {
        return data_n;
    }

    fsize n_read = disk.read(nth_block, rwbuffer.data(), MB.block_size);
    if (n_read != MB.block_size) {
        throw std::runtime_error("Error while read operation.");
    }

    casch_info.reset();
    casch_info.nth_block = nth_block;

    return data_n;
}

const address& IndirectInode::get_ptr(address base_addr, address ptr_n) {
    if ((base_addr < 0) || (base_addr >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid block number.");
    }
    if (ptr_n < 0) {
        throw std::invalid_argument("Invalid ptr number.");
    }

    if (read_indirect(base_addr, ptr_n) == fs_nullptr) {
        throw std::runtime_error("Unable to read indirect block.");
    }

    address nth_ptr = ptr_n - casch_info.low_ptr_n;
    return indirect_ptr[nth_ptr];
}

address& IndirectInode::set_ptr(address base_addr, address ptr_n) {
    if ((base_addr < 0) || (base_addr >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid block number.");
    }
    if (ptr_n < 0) {
        throw std::invalid_argument("Invalid ptr number.");
    }

    if (read_indirect(base_addr, ptr_n) == fs_nullptr) {
        throw std::runtime_error("Unable to read indirect block.");
    }

    address nth_ptr = ptr_n - casch_info.low_ptr_n;
    return indirect_ptr[nth_ptr];
}

address IndirectInode::get_block_address(address base_addr, address ptr_n) {
    if ((base_addr < 0) || (base_addr >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid block number.");
    }
    if (ptr_n < 0) {
        throw std::invalid_argument("Invalid ptr number.");
    }

    if (read_indirect(base_addr, ptr_n) == fs_nullptr) {
        throw std::runtime_error("Unable to read indirect block.");
    }

    return casch_info.nth_block - data_block_offset;
}

address IndirectInode::set_indirect_address(address data_n, address ptr_addr) {
    if ((data_n < 0) || (data_n >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid block number.");
    }
    if ((ptr_addr < 0) || (ptr_addr >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid ptr address.");
    }

    if (random_read(data_n) == fs_nullptr) {
        throw std::runtime_error("Unable to read indirect block.");
    }

    indirect_ptr[n_ptrs_in_block - 1] = ptr_addr;
    return indirect_ptr[n_ptrs_in_block - 1];
}

address IndirectInode::alloc(address data_n) {
    if ((data_n < 0) || (data_n >= MB.n_data_blocks)) {
        throw std::invalid_argument("Invalid block number.");
    }

    casch_info.reset();
    casch_info.nth_block = data_block_offset + data_n;

    for (auto i = 0; i < n_ptrs_in_block; i++) {
        indirect_ptr[i] = fs_nullptr;
    }
}

void IndirectInode::commit() {
    if (casch_info.nth_block == fs_nullptr) {
        return;
    }

    fsize n_write =
        disk.write(casch_info.nth_block, rwbuffer.data(), MB.block_size);
    if (n_write != MB.block_size) {
        throw std::runtime_error("Error while write operation.");
    }
}
}