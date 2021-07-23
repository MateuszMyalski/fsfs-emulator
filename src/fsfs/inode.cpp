#include "inode.hpp"
namespace FSFS {
Inode::Inode(Disk& disk, const super_block& MB) : disk(disk), MB(MB) {
    disk.mount();
    rwbuffer.resize(MB.block_size);
    inodes = reinterpret_cast<inode_block*>(rwbuffer.data());

    casched_inodes.high_block_n = fs_nullptr;
    casched_inodes.low_block_n = fs_nullptr;
    casched_n_block = fs_nullptr;

    n_inodes_in_block = MB.block_size / meta_fragm_size;
}
address Inode::read_inode(address inode_n) {
    if (((inode_n < casched_inodes.high_block_n) &&
         (inode_n >= casched_inodes.low_block_n)) &&
        casched_n_block != fs_nullptr) {
        return inode_n;
    }

    address inode_block_n = inode_n / n_inodes_in_block;
    address inode_addr = fs_offset_inode_block + inode_block_n;
    fsize n_read = disk.read(inode_addr, rwbuffer.data(), MB.block_size);
    if (n_read != MB.block_size) {
        throw std::runtime_error("Error while read operation.");
    }

    casched_inodes.low_block_n = inode_n * inode_block_n;
    casched_inodes.high_block_n =
        casched_inodes.low_block_n + n_inodes_in_block;
    casched_n_block = inode_addr;
}

Inode::~Inode() { disk.unmount(); }

const inode_block& Inode::get_inode(address inode_n) {
    if (inode_n < 0 || inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Invalid inode number.");
    }
    read_inode(inode_n);

    address nth_inode = inode_n - casched_inodes.low_block_n;
    return inodes[nth_inode];
}

inode_block& Inode::update_inode(address inode_n) {
    if (inode_n < 0 || inode_n >= MB.n_inode_blocks) {
        throw std::invalid_argument("Invalid inode number.");
    }
    read_inode(inode_n);

    address nth_inode = inode_n - casched_inodes.low_block_n;
    return inodes[nth_inode];
}

address Inode::alloc_inode(address inode_n) {
    if (get_inode(inode_n).type == block_status::Used) {
        return fs_nullptr;
    }

    update_inode(inode_n).type = block_status::Used;
    update_inode(inode_n).file_len = 0;
    update_inode(inode_n).file_name[0] = '\0';
    update_inode(inode_n).indirect_inode_ptr = fs_nullptr;
    for (auto& ptr_n : update_inode(inode_n).block_ptr) {
        ptr_n = fs_nullptr;
    }
}

void Inode::commit_inode() {
    if (casched_n_block == fs_nullptr) {
        return;
    }

    fsize n_write = disk.write(casched_n_block, rwbuffer.data(), MB.block_size);
    if (n_write != MB.block_size) {
        throw std::runtime_error("Error while write operation.");
    }
}
}
