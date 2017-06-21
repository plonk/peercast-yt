#include <gtest/gtest.h>
#include "id.h"

class ID4Fixture : public ::testing::Test {
public:
    ID4Fixture()
        : pid(new ID4()),
          abcd("abcd"),
          abcd2('abcd')
    {
    }

    ~ID4Fixture( )
    {
        delete pid;
    }

    ID4 id;
    ID4* pid;
    ID4 abcd;
    ID4 abcd2;
};

TEST_F(ID4Fixture, initializedToZero)
{
    ASSERT_EQ(0, id.getValue());
    ASSERT_EQ(0, pid->getValue());
}

TEST_F(ID4Fixture, equalsAndNotEqual)
{
    ASSERT_EQ(id, *pid);
    ASSERT_NE(id, abcd);
}

// 複数文字の文字定数はリトル・エンディアンの環境で文字の順番が反転す
// ることが期待される。
TEST_F(ID4Fixture, getValue)
{
    uint16_t n = 0xabcd;
    uint8_t *p = (uint8_t*) &n;

    if (*p == 0xab) {
        // ビッグ
        ASSERT_EQ(ID4('abcd'), ID4("abcd"));
    } else if (*p == 0xcd) {
        // リトル
        ASSERT_NE(ID4('abcd'), ID4("abcd"));
    } else {
        // 何も信じられない。
        ASSERT_TRUE(false);
    }
}
