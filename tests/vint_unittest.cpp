#include <gtest/gtest.h>

#include "matroska.h"
using namespace matroska;

class VIntFixture : public ::testing::Test {
public:
    VIntFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~VIntFixture()
    {
    }
};

TEST_F(VIntFixture, numLeadingZeroes)
{
     uint8_t n = 1;

     ASSERT_EQ(8, VInt::numLeadingZeroes(0));
     ASSERT_EQ(7, VInt::numLeadingZeroes(1));
     ASSERT_EQ(6, VInt::numLeadingZeroes(1 << 1));
     ASSERT_EQ(5, VInt::numLeadingZeroes(1 << 2));
     ASSERT_EQ(4, VInt::numLeadingZeroes(1 << 3));
     ASSERT_EQ(3, VInt::numLeadingZeroes(1 << 4));
     ASSERT_EQ(2, VInt::numLeadingZeroes(1 << 5));
     ASSERT_EQ(1, VInt::numLeadingZeroes(1 << 6));
     ASSERT_EQ(0, VInt::numLeadingZeroes(1 << 7));
}

TEST_F(VIntFixture, uint64)
{
    ASSERT_EQ(1, VInt({0x81}).uint());

    // 2^56 - 1
    ASSERT_EQ(((uint64_t)1 << 56) - 1,
              VInt({0x01, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}).uint());
    // 2^14 - 1
    ASSERT_EQ((1<<14) - 1,
              VInt({0x7f, 0xff}).uint());
}

TEST_F(VIntFixture, constructor)
{
    ASSERT_THROW(VInt(matroska::byte_string({0xff})), std::runtime_error);
}

TEST_F(VIntFixture, idToName)
{
    ASSERT_STREQ("EBML", VInt({0x1A,0x45,0xDF,0xA3}).toName().c_str());
}
