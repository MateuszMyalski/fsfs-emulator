#ifndef CLI_EMULATOR_OPT_PARSER_HPP
#define CLI_EMULATOR_OPT_PARSER_HPP
#include <stdio.h>

namespace FSFS {
enum class ActionType {
    INVALID_PARSING,
    DISPLAY_STATS,
    DISPLAY_FILES,
    DISPLAY_HELP,
    WRITE_DATA,
    READ_DATA,
    DELETE_FILE,
    RENAME_FILE,
    FORMAT_DISK,
    CREATE_DISK,
};

class OptParser {
   public:
    OptParser() = default;
    OptParser(int argc, char* const* argv);

    void parse(int argc, char* const* argv);
    void print_help(FILE* buff);

    ActionType action_type = ActionType::INVALID_PARSING;
    struct {
        char* disk_path = nullptr;
        char* in_file_name = nullptr;
        char* out_file_name = nullptr;
        int offset = -1;
        int block_size = -1;
        int length = -1;
        int file_inode = -1;
    } parsed_args;
};
}
#endif