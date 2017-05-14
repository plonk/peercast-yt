#include <gtest/gtest.h>

#include "mkv.h"

class MKVStreamFixture : public ::testing::Test {
public:
    MKVStreamFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~MKVStreamFixture()
    {
    }
};

TEST_F(MKVStreamFixture, unpackUnsignedInt)
{
    ASSERT_THROW(MKVStream::unpackUnsignedInt(""), std::runtime_error);
    ASSERT_EQ(1, MKVStream::unpackUnsignedInt("\x01"));
    ASSERT_EQ(258, MKVStream::unpackUnsignedInt("\x01\x02"));
}
