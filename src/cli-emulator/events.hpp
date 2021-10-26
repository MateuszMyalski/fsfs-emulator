#ifndef CLI_EMULATOR_EVENTS_HPP
#define CLI_EMULATOR_EVENTS_HPP

namespace FSFS {
void event_invalid_parsing();
void event_display_help();
void event_display_stats(const char* disk_name, int block_size, int inode_n);
void event_display_files(const char* disk_name, int block_size);
void event_write_data(const char* disk_name, int block_size, const char* file_name, int inode_n);
void event_read_data(const char* disk_name, int block_size, int inode_n);
void event_delete_file(const char* disk_name, int block_size, int inode_n);
void event_rename_file(const char* disk_name, int block_size, int inode_n, const char* new_file_name);
void event_format_disk(const char* disk_name, int block_size);
void event_create_disk(const char* disk_name, int block_size, int size);
}
#endif