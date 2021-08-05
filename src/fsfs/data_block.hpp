#ifndef FSFS_DATA_BLOCK_HPP
#define FSFS_DATA_BLOCK_HPP
#include <vector>

#include "common/types.hpp"
#include "data_structs.hpp"
#include "disk-emulator/disk.hpp"

namespace FSFS {
class DataBlock {
   private:
    Disk& disk;
    const super_block& MB;
    std::vector<data> rwbuffer;
    address casched_block;

    address read_block(address block_n);

   public:
    DataBlock(Disk& disk, const super_block& MB);
    ~DataBlock();

    void reinit();
    fsize write(address block_n, const data* wdata, fsize offset, fsize length);
    fsize read(address block_n, data* rdata, fsize offset, fsize length);
};
}
#endif
