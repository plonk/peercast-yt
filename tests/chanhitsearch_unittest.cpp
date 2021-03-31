#include <gtest/gtest.h>

#include "channel.h"

class ChanHitSearchFixture : public ::testing::Test {
public:
    ChanHitSearch chs;
};

TEST_F(ChanHitSearchFixture, initialState)
{
    char buf[33];
    GnuID zero_id;
    zero_id.clear();

    ASSERT_EQ("0.0.0.0:0", chs.matchHost.str());

    ASSERT_EQ(0, chs.waitDelay);
    ASSERT_EQ(false, chs.useFirewalled);
    ASSERT_EQ(false, chs.trackersOnly);
    ASSERT_EQ(true, chs.useBusyRelays);
    ASSERT_EQ(true, chs.useBusyControls);
    ASSERT_TRUE(chs.excludeID.isSame(zero_id));
    ASSERT_EQ(0, chs.numResults);
}
