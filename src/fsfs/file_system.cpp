#include "file_system.hpp"

#include <algorithm>
#include <cstring>

namespace FSFS {

void FileSystem::mount() {
    disk.mount();
    read_super_block(disk, MB);
    inode_bitmap.resize(MB.n_inode_blocks);
    data_bitmap.resize(MB.n_data_blocks);
    block.resize();

    scan_blocks();
}

void FileSystem::read_super_block(Disk& disk, super_block& MB) {
    disk.mount();

    auto n_read = disk.read(fs_offset_super_block, cast_to_data(&MB), sizeof(super_block));

    if (n_read != sizeof(super_block)) {
        throw std::runtime_error("Cannot read super block");
    }

    for (int32_t i = 0; i < fs_data_row_size; i++) {
        if (MB.magic_number[i] != meta_magic_seq_lut[i]) {
            throw std::runtime_error("Super block corrupted. Magic number error.");
        }
    }

    if (MB.checksum != calc_mb_checksum(MB)) {
        throw std::runtime_error("Checksum error.");
    }

    if ((MB.block_size % quant_block_size) != 0) {
        throw std::runtime_error("Block size must be multiplication of 1024.");
    }
    if (MB.n_blocks <= 0) {
        throw std::runtime_error("Invalid amount of blocks number.");
    }

    disk.unmount();
}

void FileSystem::unmount() { disk.unmount(); }

void FileSystem::format(Disk& disk) {
    disk.mount();

    fsize real_disk_size = disk.get_disk_size() - 1;
    fsize meta_blocks = disk.get_block_size() / meta_fragm_size_bytes;
    fsize n_inode_blocks = real_disk_size * 0.1;
    std::vector<meta_block> block(meta_blocks);

    std::copy_n(meta_magic_seq_lut, fs_data_row_size, block[0].MB.magic_number);
    block[0].MB.block_size = disk.get_block_size();
    block[0].MB.n_blocks = disk.get_disk_size();
    block[0].MB.n_inode_blocks = n_inode_blocks;
    block[0].MB.n_data_blocks = real_disk_size - n_inode_blocks;
    block[0].MB.fs_ver_major = fs_system_major;
    block[0].MB.fs_ver_minor = fs_system_minor;
    block[0].MB.checksum = calc_mb_checksum(block[0].MB);
    disk.write(fs_offset_super_block, block[0].raw_data, meta_fragm_size_bytes);

    std::fill(block.begin(), block.end(), fs_nullptr);
    for (auto i = 0; i < meta_blocks; i++) {
        block[i].inode.status = block_status::Free;
        std::memcpy(block[i].inode.file_name, inode_default_file_name, meta_max_file_name_size);
    }
    for (auto i = 0; i < n_inode_blocks; i++) {
        disk.write(fs_offset_inode_block + i, cast_to_data(block.data()), disk.get_block_size());
    }

    disk.unmount();
}

uint32_t FileSystem::calc_mb_checksum(super_block& MB) {
    const uint8_t xor_word[] = {0xFE, 0xED, 0xC0, 0xDE};
    uint8_t* raw_data = cast_to_data(&MB);
    uint8_t checksum[fs_data_row_size] = {};

    fsize mb_size = meta_fragm_size_bytes - sizeof(MB.checksum);
    for (auto i = 0; i < mb_size; i++) {
        int32_t idx = i & 0x03;
        checksum[idx] ^= raw_data[i] ^ xor_word[idx];
    }

    return *reinterpret_cast<uint32_t*>(checksum);
}

void FileSystem::set_data_blocks_status(address inode_n, bool status) {
    inode.load(inode_n, block);
    fsize n_ptrs_used = block.bytes_to_blocks(inode.meta().file_len);

    for (fsize i = 0; i < n_ptrs_used; i++) {
        data_bitmap.set_status(inode.ptr(i), status);
    }

    fsize indirect_addr = inode.last_indirect_ptr(0);
    for (fsize indirect_block_n = 0; indirect_addr != fs_nullptr; indirect_block_n++) {
        data_bitmap.set_status(indirect_addr, status);
        indirect_addr = inode.last_indirect_ptr(indirect_block_n);
    }
}

fsize FileSystem::edit_data(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::min;

    if (offset == 0) {
        return 0;
    }

    inode.load(inode_n, block);

    // Step 1: Check if there is data to edit
    fsize abs_offset = inode.meta().file_len - offset;
    if (abs_offset < 0) {
        return fs_nullptr;
    }

    // Step 2: Edit tail in last data block
    address ptr_n = abs_offset / MB.block_size;
    fsize first_offset = abs_offset % MB.block_size;
    fsize n_written_bytes = block.write(block.data_n_to_block_n(inode.ptr(ptr_n)), wdata, first_offset,
                                        min(MB.block_size - first_offset, length));
    ptr_n += 1;

    // Step 3: Edit full blocks of data
    if (length - n_written_bytes > 0) {
        fsize blocks_to_edit = block.bytes_to_blocks(length);
        for (auto i = 0; i < blocks_to_edit; i++) {
            address addr = block.data_n_to_block_n(inode.ptr(ptr_n));
            fsize write_length = min(MB.block_size, length - n_written_bytes);
            n_written_bytes += block.write(addr, &wdata[n_written_bytes], 0, write_length);
            ptr_n++;
        }
    }

    return n_written_bytes;
}

fsize FileSystem::write(address inode_n, const data* wdata, fsize offset, fsize length) {
    using std::max;
    using std::min;

    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    inode.load(inode_n, block);

    // Step 1: Check if there is data to edit
    //
    fsize n_eddited_bytes = edit_data(inode_n, wdata, offset, min(length, offset));
    if (n_eddited_bytes == length || n_eddited_bytes == fs_nullptr) {
        return n_eddited_bytes;
    }

    // Step 2: Prepare informations to allocate new data blocks
    //
    const data* wdata_new_p = &wdata[n_eddited_bytes];
    fsize n_written = 0;
    fsize n_ptr_used = block.bytes_to_blocks(inode.meta().file_len);
    fsize free_bytes = n_ptr_used * MB.block_size - inode.meta().file_len;
    fsize blocks_of_new_data = block.bytes_to_blocks(max(0, length - free_bytes - n_eddited_bytes));

    // Step 3: Store new data in already allocated block
    //
    if (free_bytes > 0) {
        address last_ptr_n = max(0, n_ptr_used - 1);
        address addr = block.data_n_to_block_n(inode.ptr(last_ptr_n));
        n_written += block.write(addr, wdata_new_p, -free_bytes, free_bytes);
    }

    // Step 4: Store data in new allocated blocks
    //
    for (auto i = 0; i < blocks_of_new_data; i++) {
        // Allocate new block
        address data_n = data_bitmap.next_free(0);
        if (data_n == fs_nullptr) {
            break;
        }
        data_bitmap.set_status(data_n, 1);
        inode.add_data(data_n);

        // Store data in block
        address addr = block.data_n_to_block_n(data_n);
        fsize to_write = std::min(length - n_written - n_eddited_bytes, MB.block_size);
        n_written += block.write(addr, &wdata_new_p[n_written], 0, to_write);
    }

    // Step 5: Update inode meta
    //
    inode.meta().file_len += n_written;
    inode.commit(block, data_bitmap);
    // TODO add check form inode commit

    return n_written + n_eddited_bytes;
}

address FileSystem::read(address inode_n, data* rdata, address offset, fsize length) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    if (length <= 0) {
        return 0;
    }

    fsize n_read = 0;

    // Step 1: Load inode & calculate absolute offset
    //
    inode.load(inode_n, block);

    if ((offset < 0) || (offset > inode.meta().file_len)) {
        return fs_nullptr;
    }

    // Step 2: Calculate absolute offset & max length to read
    //
    if (offset + length > inode.meta().file_len) {
        length = inode.meta().file_len - offset;
    }
    address offset_ptr = std::max(0, block.bytes_to_blocks(offset) - 1);

    // Step 3 : Read first block and attach to the rdata buffor
    //
    address addr = block.data_n_to_block_n(inode.ptr(offset_ptr));
    fsize first_offset = offset % block.get_block_size();
    fsize to_read = std::min(length, MB.block_size - first_offset);
    n_read += block.read(addr, &rdata[n_read], first_offset, to_read);
    offset_ptr += 1;

    // Step 4: Read N full blocks and attach to the rdata buffor
    //
    for (fsize ptr_n = 0; n_read < length; ptr_n++) {
        addr = block.data_n_to_block_n(inode.ptr(offset_ptr + ptr_n));
        to_read = std::min(length - n_read, MB.block_size);
        n_read += block.read(addr, &rdata[n_read], 0, to_read);
    }

    return n_read;
}

address FileSystem::create_file(const char* file_name) {
    address inode_n = inode_bitmap.next_free(0);
    if (fs_nullptr == inode_n) {
        // No free inode blocks
        return fs_nullptr;
    }

    fsize file_name_len = strnlen(file_name, meta_max_file_name_size);
    if (file_name_len == meta_max_file_name_size) {
        return fs_nullptr;
    }

    inode.alloc_new(inode_n);
    memcpy(inode.meta().file_name, file_name, file_name_len);
    inode.commit(block, data_bitmap);

    inode_bitmap.set_status(inode_n, 1);

    return inode_n;
}

address FileSystem::remove_file(address inode_n) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    inode.meta().status = block_status::Free;
    inode.commit(block, data_bitmap);

    set_data_blocks_status(inode_n, 0);
    inode_bitmap.set_status(inode_n, 0);

    return inode_n;
}

address FileSystem::rename_file(address inode_n, const char* file_name) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    fsize file_name_len = strnlen(file_name, meta_max_file_name_size);
    if (file_name_len == meta_max_file_name_size - 1) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    memcpy(inode.meta().file_name, file_name, file_name_len);
    inode.commit(block, data_bitmap);

    return inode_n;
}

fsize FileSystem::get_file_length(address inode_n) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    return inode.meta().file_len;
}

address FileSystem::get_file_name(address inode_n, char* file_name_buffer) {
    if (!inode_bitmap.get_status(inode_n)) {
        return fs_nullptr;
    }

    inode.load(inode_n, block);
    strcpy(file_name_buffer, inode.meta().file_name);
    return inode_n;
}

void FileSystem::scan_blocks() {
    inode_bitmap.resize(MB.n_inode_blocks);
    data_bitmap.resize(MB.n_data_blocks);

    for (fsize inode_n = 0; inode_n < MB.n_inode_blocks; inode_n++) {
        inode.load(inode_n, block);
        if (inode.meta().status != block_status::Used) {
            continue;
        }
        inode_bitmap.set_status(inode_n, 1);

        set_data_blocks_status(inode_n, 1);
    }
}

}
