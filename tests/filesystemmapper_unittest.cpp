#include <gtest/gtest.h>

#include "mapper.h"

class FileSystemMapperFixture : public ::testing::Test {
public:
    FileSystemMapperFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~FileSystemMapperFixture()
    {
    }
};

TEST_F(FileSystemMapperFixture, realPath)
{
    ASSERT_EQ("C:\\", FileSystemMapper::realPath("/"));
    ASSERT_EQ("", FileSystemMapper::realPath("/tmp"));
    ASSERT_EQ("", FileSystemMapper::realPath("/root"));
    ASSERT_EQ("", FileSystemMapper::realPath("/tmp/../root"));
    ASSERT_EQ("", FileSystemMapper::realPath("/nonexistent"));
}
