#include <gtest/gtest.h>
#include "common.h"
#include "mocksys.h"

#include <gtest/gtest.h>

// GnuID::generate() が sys->rnd() に依存している。

class GnuIDFixture : public ::testing::Test {
public:
    GnuIDFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~GnuIDFixture()
    {
    }

    GnuID id;
    char buf[80]; // 最低 33 バイト必要。
};

TEST_F(GnuIDFixture, initialState)
{
    // GnuID はコンストラクタで初期化されないので、不定値が入っている。
    // オール0のIDが欲しければ明示的に clear メソッドでクリアする。
    // id.toStr(buf);
    // EXPECT_TRUE(id.isSet());
    // EXPECT_STRNE("00000000000000000000000000000000", buf);

    ASSERT_FALSE(id.isSet());
    id.toStr(buf);
    ASSERT_STREQ("00000000000000000000000000000000", buf);
}

TEST_F(GnuIDFixture, generate)
{
    id.toStr(buf);
    ASSERT_FALSE(id.isSet());
    ASSERT_STREQ("00000000000000000000000000000000", buf);

    GnuID id2;
    id2.clear();
    ASSERT_TRUE(id.isSame(id2));

    id.generate();
    EXPECT_FALSE(id.isSame(id2));
}

TEST_F(GnuIDFixture, getFlags)
{
    ASSERT_EQ(0, id.getFlags());

    id.generate(0xff);
    ASSERT_EQ(0xff, id.getFlags());
}

TEST_F(GnuIDFixture, clear)
{
    id.generate();
    ASSERT_TRUE(id.isSet());

    id.clear();
    ASSERT_FALSE(id.isSet());
}

TEST_F(GnuIDFixture, encode)
{
    ASSERT_FALSE(id.isSet());
    id.encode(NULL, "A", NULL, 0);
    ASSERT_TRUE(id.isSet());

    GnuID id2;
    id2.encode(NULL, "A", NULL, 0);
    ASSERT_TRUE(id.isSame(id2));

    ASSERT_STREQ("41004100410041004100410041004100", id.str().c_str());
}

TEST_F(GnuIDFixture, encode2)
{
    id.encode(NULL, "A", "B", 0);
    ASSERT_STREQ("03000300030003000300030003000300", id.str().c_str());
}

TEST_F(GnuIDFixture, encode3)
{
    id.encode(NULL, "A", "B", 1);
    ASSERT_STREQ("02010201020102010201020102010201", id.str().c_str());
}

TEST_F(GnuIDFixture, encode4)
{
    id.encode(NULL, "AB", NULL, 0);
    ASSERT_STREQ("41420041420041420041420041420041", id.str().c_str());
}

TEST_F(GnuIDFixture, encode_differentSeeds)
{
    ASSERT_FALSE(id.isSet());
    id.encode(NULL, "A", NULL, 0);

    GnuID id2;
    id2.fromStr("00000000000000000000000000000001");
    ASSERT_TRUE(id.isSet());
    id2.encode(NULL, "A", NULL, 0);
    ASSERT_FALSE(id.isSame(id2));

    ASSERT_STREQ("41004100410041004100410041004100", id.str().c_str());
    ASSERT_STREQ("41004100410041004100410041004101", id2.str().c_str());
}

TEST_F(GnuIDFixture, encode_differentSalts)
{
    id.encode(NULL, "A", NULL, 0);

    GnuID id2;
    id2.encode(NULL, "B", NULL, 0);
    ASSERT_FALSE(id.isSame(id2));

    ASSERT_STREQ("41004100410041004100410041004100", id.str().c_str());
    ASSERT_STREQ("42004200420042004200420042004200", id2.str().c_str());
}

TEST_F(GnuIDFixture, encode_prefixSharingSalts)
{
    id.encode(NULL, "ナガイナマエ", NULL, 0);

    GnuID id2;
    id2.encode(NULL, "ナガイナマエ(立て直し)", NULL, 0);
    ASSERT_FALSE(id.isSame(id2));
}
