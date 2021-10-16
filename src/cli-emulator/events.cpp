#include "events.hpp"

#include "disk-emulator/disk.hpp"
#include "fsfs/file_system.hpp"

namespace FSFS {
namespace {
void display_critical_error(const std::exception& e) {
    fprintf(stdout, "Critical error occured with message:\n\t%s\nAction terminated!\n", e.what());
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
        // TODO

        fs.unmount();
        fprintf(stdout, "Not implemented!\n");
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
        // TODO

        fs.unmount();
        fprintf(stdout, "Not implemented!\n");
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_write_data(const char* disk_path, int block_size, const char* file_name, int inode_n, int offset,
                      int length) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();
        // TODO
        fs.unmount();
        fprintf(stdout, "%d bytes of uint8_t written from %s\n", 0, file_name);
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_read_data(const char* disk_path, int block_size, const char* file_name, int inode_n, int offset,
                     int length) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem fs(disk);
        fs.mount();
        // TODO
        fs.unmount();
        fprintf(stdout, "%d bytes of uint8_t writen to %s.\n", 0, file_name);
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
        fprintf(stdout, "File removed.\n");
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
        fprintf(stdout, "File renamed.\n");
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}

void event_format_disk(const char* disk_path, int block_size) {
    try {
        Disk disk(block_size);
        disk.open(disk_path);
        FileSystem::format(disk);

        fprintf(stdout, "Disk formatted with block size: %dkb and total size of %dkb.\n", block_size,
                disk.get_disk_size());
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

        fprintf(stdout, "Disk created as: %s, with block size: %dkb and total size of %dkb.\n", disk_path,
                disk.get_block_size(), disk.get_disk_size());
    } catch (const std::exception& e) {
        display_critical_error(e);
    }
}
}
