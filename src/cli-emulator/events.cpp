#include "events.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "common/types.hpp"
#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"

namespace FSFS {
constexpr size_t chunk_size = 4096;
namespace {
void display_critical_error(const std::exception& e) {
    printf("Critical error occured with message:\n\t%s\nAction terminated!\n", e.what());
}
}
void event_invalid_parsing() {
    // No action required
}

void event_display_help() {
    // No action required
}

void event_display_stats(const char* disk_path, int block_size, int inode_n) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();
        auto inode_bitmap = fs.get_inode_bitmap();
        auto block_bitmap = fs.get_data_bitmap();

        if (inode_n == -1) {
            auto used_inode_blocks = 0;
            for (auto i = 0; i < fs.get_inode_blocks_ammount(); i++) {
                used_inode_blocks += inode_bitmap.get_status(i) ? 1 : 0;
            }
            auto used_data_blocks = 0;
            for (auto i = 0; i < fs.get_data_blocks_ammount(); i++) {
                used_data_blocks += block_bitmap.get_status(i) ? 1 : 0;
            }

            printf("Disk stats:\n");
            printf("\tDisk block size: %d kb.\n", disk.get_block_size());
            printf("\tDisk total size: %d kb.\n", disk.get_disk_size());
            printf("\tInode blocks: %d segments.\n", fs.get_inode_blocks_ammount());
            printf("\tData blocks: %d segments.\n", fs.get_data_blocks_ammount());
            printf("\tUsed inodes blocks: %d\n", used_inode_blocks);
            printf("\tUsed data blocks: %d\n", used_data_blocks);

        } else {
            printf("File stats:\n");
            printf("\tFile inode number: %d\n", inode_n);

            if (inode_bitmap.get_status(inode_n)) {
                char file_name_buf[32] = {};
                fs.get_file_name(inode_n, file_name_buf);
                printf("\tFile name: %s\n", file_name_buf);
                printf("\tFile length: %d bytes\n", fs.get_file_length(inode_n));
            } else {
                printf("\tInode empty. No file to display info.\n");
            }
        }

        fs.unmount();
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_display_files(const char* disk_path, int block_size) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();
        auto inode_bitmap = fs.get_inode_bitmap();

        size_t files_found = 0;
        for (auto inode_n = 0; inode_n < fs.get_inode_blocks_ammount(); inode_n++) {
            if (inode_bitmap.get_status(inode_n)) {
                char file_name_buf[32] = {};
                fs.get_file_name(inode_n, file_name_buf);
                printf("\t[%d]\t%s\t%d bytes\n", inode_n, file_name_buf, fs.get_file_length(inode_n));
                files_found += 1;
            }
        }

        fs.unmount();
        printf("Files found: %ld\n", files_found);
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_write_data(const char* disk_path, int block_size, const char* file_name, int inode_n) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();

        // 1. Open file on hosts disk
        //
        printf("Opening file %s\n", file_name);
        std::fstream in_file;
        in_file.open(file_name, std::ios::in | std::ios::binary);
        if (!in_file.is_open()) {
            fs.unmount();
            throw std::runtime_error("Unable to open file.");
        }

        in_file.seekg(0, std::ios::end);
        size_t host_file_size = in_file.tellg();
        in_file.seekg(0, in_file.beg);

        // 2. Prepare buffor for file system
        //
        printf("Reading file...\n");
        char r_char_buffer[chunk_size];

        std::vector<uint8_t> r_buffer;
        r_buffer.resize(chunk_size * sizeof(char));

        // 3. Check if file already exists
        int32_t write_inode_n = inode_n;
        if (write_inode_n == -1) {
            write_inode_n = fs.create_file("NO_NAME.bin");
            printf("File created with name: 'NO_NAME.bin'\n");
            printf("File inode number: %d\n", write_inode_n);
        } else {
            if (fs.get_file_length(write_inode_n) != -1) {
                printf("File already exists append file to the end? [y/n] (n) ");
                char ans = 'n';
                std::cin >> ans;
                if (tolower(ans) != 'y') {
                    return;
                }
            }
        }

        // 4. Perform write operation
        //
        size_t n_read = 0;
        size_t to_write = host_file_size;
        while (to_write > 0) {
            // Calc the minimal chunk the file that can be read
            size_t to_read = std::min(chunk_size, to_write);
            to_write -= to_read;

            // Read file, fill the buffor and append to filesystem's inode
            // We cannot trust that char is 8-bits, we need to type pun here
            in_file.read(r_char_buffer, to_read);
            memcpy(r_buffer.data(), r_char_buffer, chunk_size * sizeof(char));
            if (fs.write(write_inode_n, r_buffer.data(), 0, to_read) == -1) {
                throw std::runtime_error("Cannot write to the filesystem image.");
            }
            n_read += to_read;
            printf("\rWritting: [%ld/%ld]", n_read, host_file_size);
        }
        printf("\n");

        in_file.close();
        fs.unmount();

        printf("%ld bytes of data written from %s on inode number: %d.\n", n_read, file_name, write_inode_n);
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_read_data(const char* disk_path, int block_size, int inode_n) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();

        // 1. Check if file even exists
        //
        auto file_size = fs.get_file_length(inode_n);
        if (file_size == -1) {
            printf("File in inode number %d does not exists.\n", inode_n);
            return;
        }

        // 2. Create file
        //
        char out_file_name[32] = {};
        fs.get_file_name(inode_n, out_file_name);

        printf("Creating file %s\n", out_file_name);
        std::fstream out_file;
        out_file.open(out_file_name, std::ios::out | std::ios::binary);
        if (!out_file.is_open()) {
            fs.unmount();
            throw std::runtime_error("Unable to create file.");
        }
        out_file.seekg(0, std::ios::end);

        // 3. Prepare buffers
        char w_char_buffer[chunk_size];

        std::vector<uint8_t> w_buffer;
        w_buffer.resize(chunk_size * sizeof(char));

        // 4. Read file and write to the end of host's file
        //
        size_t n_written = 0;
        size_t to_read = file_size;
        auto cnt = 0;
        while (to_read > 0) {
            // Calc the minimal chunk the file that can be read
            size_t to_write = std::min(chunk_size, to_read);
            to_read -= to_write;
            if (fs.read(inode_n, w_buffer.data(), n_written, to_write) == -1) {
                throw std::runtime_error("Cannot write to the filesystem image.");
            }

            // We cannot trust that char is 8-bits, we need to type pun here
            memcpy(w_char_buffer, w_buffer.data(), chunk_size * sizeof(char));
            out_file.write(w_char_buffer, to_write);
            n_written += to_write;
            printf("\rReading: [%ld/%d]", n_written, file_size);
        }
        printf("\n");

        out_file.close();
        fs.unmount();

        printf("%ld bytes of data writen to %s.\n", n_written, out_file_name);
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_delete_file(const char* disk_path, int block_size, int inode_n) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();

        fs.remove_file(inode_n);
        fs.unmount();
        printf("File removed.\n");
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_rename_file(const char* disk_path, int block_size, int inode_n, const char* new_file_name) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();
        fs.rename_file(inode_n, new_file_name);
        fs.unmount();
        printf("File renamed.\n");
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_format_disk(const char* disk_path, int block_size) {
    printf("Thios will erease whiel disk, continue? [y/n] (n) ");
    char ans = 'n';
    std::cin >> ans;
    if (tolower(ans) != 'y') {
        return;
    }

    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem::format(disk);

        printf("Disk formatted with block size: %dkb and total size of %dkb.\n", block_size, disk.get_disk_size());
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_create_disk(const char* disk_path, int block_size, int size) {
    try {
        Disk::create(disk_path, size, block_size);

        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem::format(disk);

        printf("Disk created as: %s, with block size: %dkb and total size of %dkb.\n", disk_path, disk.get_block_size(),
               disk.get_disk_size());
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}
}
