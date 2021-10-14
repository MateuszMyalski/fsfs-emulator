#include "cli-emulator/events.hpp"
#include "cli-emulator/opt_parser.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    FSFS::OptParser parser(argc, argv);

    if (parser.action_type == FSFS::ActionType::INVALID_PARSING) {
        parser.print_help(stdout);
    }

    auto& args = parser.parsed_args;

    switch (parser.action_type) {
        case FSFS::ActionType::DISPLAY_STATS:
            FSFS::event_display_stats(args.disk_path, args.block_size, args.file_inode);
            break;
        case FSFS::ActionType::DISPLAY_FILES:
            FSFS::event_display_files(args.disk_path, args.block_size);
            break;
        case FSFS::ActionType::WRITE_DATA:
            FSFS::event_write_data(args.disk_path, args.block_size, args.in_file_name, args.file_inode, args.offset,
                                   args.length);
            break;
        case FSFS::ActionType::READ_DATA:
            FSFS::event_write_data(args.disk_path, args.block_size, args.in_file_name, args.file_inode, args.offset,
                                   args.length);
            break;
        case FSFS::ActionType::DELETE_FILE:
            FSFS::event_delete_file(args.disk_path, args.block_size, args.file_inode);
            break;
        case FSFS::ActionType::RENAME_FILE:
            FSFS::event_rename_file(args.disk_path, args.block_size, args.file_inode, args.in_file_name);
            break;
        case FSFS::ActionType::FORMAT_DISK:
            FSFS::event_format_disk(args.disk_path, args.block_size);
            break;
        case FSFS::ActionType::CREATE_DISK:
            FSFS::event_create_disk(args.disk_path, args.block_size, args.length);
            break;
        case FSFS::ActionType::DISPLAY_HELP:
        case FSFS::ActionType::INVALID_PARSING:
        default:
            parser.print_help(stdout);
            break;
    }

    return 1;
}