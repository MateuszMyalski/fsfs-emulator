#include "fsfs/file_system.hpp"

#include "gtest/gtest.h"
using namespace FSFS;
namespace {
class FileSystemTest : public ::testing::Test {
   protected:
   public:
    void SetUp() override {}
    void TearDown() override {}
};
TEST_F(FileSystemTest, constructor) {}

}