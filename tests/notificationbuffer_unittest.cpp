#include <gtest/gtest.h>

#include "notif.h"

class NotificationBufferFixture : public ::testing::Test {
public:
    NotificationBufferFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~NotificationBufferFixture()
    {
    }
};

TEST_F(NotificationBufferFixture, initialState)
{
    NotificationBuffer buf;

    ASSERT_EQ(0, buf.numNotifications());
    ASSERT_EQ(0, buf.numUnread());

    ASSERT_NO_THROW(buf.getNotification(123));

    NotificationBuffer::Entry e = buf.getNotification(123);
    ASSERT_FALSE(e.isRead);
}

TEST_F(NotificationBufferFixture, addNotification)
{
    NotificationBuffer buf;
    Notification notif;

    ASSERT_EQ(0, buf.numNotifications());
    buf.addNotification(notif);
    ASSERT_EQ(1, buf.numNotifications());
}

TEST_F(NotificationBufferFixture, maxNotifs)
{
    NotificationBuffer buf;
    Notification notif;

    for (int i = 0; i < 100; i++)
        buf.addNotification(notif);

    ASSERT_EQ(20, buf.numNotifications());
}
