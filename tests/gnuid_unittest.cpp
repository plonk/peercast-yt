#include <gtest/gtest.h>
#include "common.h"
#include "mocksys.h"

TEST(GnuIDTest, someThingsWork) {
    // GnuID::generate() が sys->rnd() に依存している。

    GnuID id;
    char buf[80]; // 最低 33 バイト必要。

    // GnuID はコンストラクタで初期化されないので、不定値が入っている。
    // オール0のIDが欲しければ明示的に clear メソッドでクリアする。
    // id.toStr(buf);
    // EXPECT_TRUE(id.isSet());
    // EXPECT_STRNE("00000000000000000000000000000000", buf);

    id.clear();
    id.toStr(buf);
    ASSERT_FALSE(id.isSet());
    ASSERT_STREQ("00000000000000000000000000000000", buf);

    GnuID id2;
    id2.clear();
    ASSERT_TRUE(id.isSame(id2));

    id.generate();
    EXPECT_FALSE(id.isSame(id2));
    ASSERT_EQ(0, id.getFlags());

    id.generate(0xff);
    ASSERT_EQ(0xff, id.getFlags());
}
