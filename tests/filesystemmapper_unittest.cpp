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
#ifdef _UNIX
    ASSERT_EQ("/", FileSystemMapper::realPath("/"));
    ASSERT_EQ("/tmp", FileSystemMapper::realPath("/tmp"));
    ASSERT_EQ("/root", FileSystemMapper::realPath("/root"));
    ASSERT_EQ("/root", FileSystemMapper::realPath("/tmp/../root"));
#endif
#ifdef WIN32
    ASSERT_STREQ(":\\", FileSystemMapper::realPath("/").c_str() + 1); // The drive letter might vary.
#endif
    ASSERT_EQ("", FileSystemMapper::realPath("/nonexistent"));
}
