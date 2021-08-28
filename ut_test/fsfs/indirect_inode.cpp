#include "fsfs/indirect_inode.hpp"

#include "test_base.hpp"

using namespace FSFS;
namespace {
class IndirectInodeTest : public ::testing::TestWithParam<fsize>, public TestBaseFileSystem {
   protected:
    IndirectInode* indirect_inode;

   public:
    void SetUp() override { indirect_inode = new IndirectInode(); }
    void TearDown() override { delete indirect_inode; }
};

TEST(IndirectInodeTest, get_ptr_operator_throw_out_of_bound) {
    Inode inode();
    // TODO add indirect example
    // EXPECT_THROW(inode[], std::runtime_error);
}

TEST_P(IndirectInodeTest, load_and_check) {}

TEST_P(IndirectInodeTest, load_clear_and_check) {}

TEST_P(IndirectInodeTest, add_data_and_commit) {}

INSTANTIATE_TEST_SUITE_P(BlockSize, IndirectInodeTest, testing::ValuesIn(valid_block_sizes));
}