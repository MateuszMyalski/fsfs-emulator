#include "opt_parser.hpp"

#include <unistd.h>

#include <string>
namespace FSFS {
OptParser::OptParser(int argc, char* const* argv) { parse(argc, argv); }

void OptParser::parse(int argc, char* const* argv) {
    int opt;
    while ((opt = getopt(argc, argv, "h:c:r:w:x:l:d:f:b:s:i:o:n:q:")) != -1) {
        switch (opt) {
            case 'h':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::DISPLAY_HELP;
                }
                break;
            case 'c':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::CREATE_DISK;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'r':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::READ_DATA;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'w':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::WRITE_DATA;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'x':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::DISPLAY_STATS;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'l':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::DISPLAY_FILES;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'd':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::DELETE_FILE;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'q':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::RENAME_FILE;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'f':
                if (action_type == ActionType::INVALID_PARSING) {
                    action_type = ActionType::FORMAT_DISK;
                    parsed_args.disk_path = optarg;
                }
                break;
            case 'b':
                parsed_args.block_size = atoi(optarg);
                break;
            case 's':
                parsed_args.length = atoi(optarg);
                break;
            case 'i':
                parsed_args.in_file_name = optarg;
                break;
            case 'n':
                parsed_args.file_inode = atoi(optarg);
                break;

            default: /* '?' */
                action_type = ActionType::INVALID_PARSING;
        }
    }
}

void OptParser::print_help(FILE* buff) {
    char help[] =
        "Flash File System Emulator\n"
        "POC by Mateusz Waldemar Myalski 2021\n"
        "==============================\n"
        "The CLI allows to pack and unpack binary data. Features for writting/reading offests are avilable only "
        "through C++ API. The file system is based on inode numbers where files are stored. "
        "Filename is optional and helps in saving file extension.\n"
        "FOR EVERY OPERATION YOU NEED TO SPECIFY BLOCK SIZE  -b <block_size>.\n"
        "Usage: fsfs <option> <args>\n"
        "Options:\n"
        "\t-h : Displays this panel.\n"
        "\t-c <disk_path> -s <size> : Creates new disk with given block size and size.\n"
        "\t-r <disk_path> -n <file_inode> : Export file from disk.\n"
        "\t-w <disk_path> -n <file_inode> -i <file_name> : Writes input file and save it on disk. "
        "If file already exists the data will be appended to the end.\n"
        "\t-x <disk_path> : Displays stats about disk.\n"
        "\t\t Optional: -n <file_inode> : Displays stats about file.\n"
        "\t-l <disk_path> : Displays all stored files.\n"
        "\t-d <disk_path> -n <file_inode> : Delete file.\n"
        "\t-f <disk_path> : Format disk.\n"
        "\t-q <disk_path> -n <file_inode> -i <file_name> : Rename file.\n"
        "\n"
        "Args:\n"
        "\t-b : Block size in kb.\n"
        "\t-s : Size (must be multiply of block size) in kb.\n"
        "\t-i : File name.\n"
        "\t-n : File inode index.\n";

    fprintf(buff, "%s", help);
}
}