#include <gtest/gtest.h>

#include "notif.h"

class NotificationFixture : public ::testing::Test {
public:
};

TEST_F(NotificationFixture, initialState1)
{
    Notification notif;
    ASSERT_EQ(0, notif.time);
    ASSERT_EQ(ServMgr::NT_PEERCAST, notif.type);
    ASSERT_STREQ("", notif.message.c_str());
}

TEST_F(NotificationFixture, initialState2)
{
    Notification notif(1, ServMgr::NT_BROADCASTERS, "A");
    ASSERT_EQ(1, notif.time);
    ASSERT_EQ(ServMgr::NT_BROADCASTERS, notif.type);
    ASSERT_STREQ("A", notif.message.c_str());
}

TEST_F(NotificationFixture, getTypeStr)
{
    Notification notif;

    notif.type = ServMgr::NT_UPGRADE;
    ASSERT_STREQ("Upgrade Alert", notif.getTypeStr().c_str());

    notif.type = ServMgr::NT_PEERCAST;
    ASSERT_STREQ("Peercast", notif.getTypeStr().c_str());

    notif.type = ServMgr::NT_BROADCASTERS;
    ASSERT_STREQ("Broadcasters", notif.getTypeStr().c_str());

    notif.type = ServMgr::NT_TRACKINFO;
    ASSERT_STREQ("Track Info", notif.getTypeStr().c_str());

    notif.type = (ServMgr::NOTIFY_TYPE)7144;
    ASSERT_STREQ("Unknown", notif.getTypeStr().c_str());
}
