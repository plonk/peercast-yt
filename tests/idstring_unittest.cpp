#include <gtest/gtest.h>
#include "id.h"

// ID4の文字列表現を保待するためのクラス。Stringよりもメモリー・フット
// プリントが小さい。

class IDStringFixture : public ::testing::Test {
public:
    IDStringFixture()
        : idstr("abcde", 4),
          long_idstr("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 40)
    {
    }

    IDString idstr;
    IDString long_idstr;
};

TEST_F(IDStringFixture, TruncateToSpecifiedLength)
{
    ASSERT_STREQ("abcd", idstr.str());
}

TEST_F(IDStringFixture, CanBeCastToConstCharPointer)
{
    ASSERT_STREQ("abcd", (const char*) idstr);
}

TEST_F(IDStringFixture, TruncateTooLongString)
{
    ASSERT_EQ(31, strlen(long_idstr.str()));
}
