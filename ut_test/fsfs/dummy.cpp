#include "fsfs/data_block.hpp"
#include "test_base.hpp"
using namespace FSFS;
namespace {
class DummyTest : public ::testing::TestWithParam<int>, public DummyExt {
   protected:
   public:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_P(DummyTest, testDumm) { EXPECT_EQ(block_size, GetParam()); }
const int valid_block_sizes[] = {1024, 2048, 3096};
INSTANTIATE_TEST_SUITE_P(MeenyMinyMoe, DummyTest, testing::ValuesIn(valid_block_sizes));
}