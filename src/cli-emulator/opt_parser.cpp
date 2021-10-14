#include "opt_parser.hpp"

#include <unistd.h>

#include <string>
namespace FSFS {
OptParser::OptParser(int argc, char* const* argv) { parse(argc, argv); }

void OptParser::parse(int argc, char* const* argv) {
    int opt;
    while ((opt = getopt(argc, argv, "h:c:r:w:x:l:d:f:b:s:i:o:p:n:")) != -1) {
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
            case 'o':
                parsed_args.out_file_name = optarg;
                break;
            case 'p':
                parsed_args.offset = atoi(optarg);
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
        "Usage: fsfs <option> <args>\n"
        "Options:\n"
        "\t-h : Displays this panel.\n"
        "\t-c <disk_path> -b <block_size> -s <size> : Creates new disk with given block size and size.\n"
        "\t-r <disk_path> -n <file_inode> -o <output_file> : Export input file from disk and save it "
        "as output file.\n"
        "\t\t Optional: (file must already exist): -p <offset> -s <length>\n"
        "\t-w <disk_path> -n <file_inode> -o <output_file> : Write input file and save it on disk as output file.\n"
        "\t\t Optional: -i <file_name>\n"
        "\t\t Optional (file must already exist): -p <offset> -s <length>\n"
        "\t-x <disk_path> : Displays stats about disk.\n"
        "\t\t Optional: -n <file_inode>\n"
        "\t-l <disk_path> : Displays all stored files.\n"
        "\t-d <disk_path> -n <file_inode> : Delete file.\n"
        "\t-f <disk_path> -b <block_size> : Format disk.\n"
        "\t-r <disk_path> -n <file_inode> -i <new_file_name> : Rename file.\n"
        "\n"
        "Args:\n"
        "\t-b : Block size in kb.\n"
        "\t-s : Size (must be multiply of block size) in kb.\n"
        "\t-i : Input file name.\n"
        "\t-n : File inode index.\n"
        "\t-o : Output file name.\n"
        "\t-p : Offset from end.\n";

    fprintf(buff, "%s", help);
}
}