#include "notif.h"
#include "critsec.h"

// global
NotificationBuffer g_notificationBuffer;

std::string Notification::getTypeStr(ServMgr::NOTIFY_TYPE type)
{
    switch (type)
    {
    case ServMgr::NT_UPGRADE:
        return "Upgrade Alert";
    case ServMgr::NT_PEERCAST:
        return "Peercast";
    case ServMgr::NT_BROADCASTERS:
        return "Broadcasters";
    case ServMgr::NT_TRACKINFO:
        return "Track Info";
    default:
        return "Unknown";
    }
}

NotificationBuffer::NotificationBuffer()
{
}

int NotificationBuffer::numUnread()
{
    CriticalSection cs(lock);

    int count = 0;
    for (auto& e : notifications)
    {
        count += !e.isRead;
    }
    return count;
}

void NotificationBuffer::markAsRead(unsigned int ctime)
{
    CriticalSection cs(lock);

    for (auto& e : notifications)
    {
        if (e.notif.time <= ctime)
            e.isRead = true;
    }
}

int NotificationBuffer::numNotifications()
{
    CriticalSection cs(lock);

    return notifications.size();
}

NotificationBuffer::Entry NotificationBuffer::getNotification(int index)
{
    CriticalSection cs(lock);

    try
    {
        return notifications.at(index);
    } catch (std::out_of_range&)
    {
        return Entry(Notification(), false);
    }
}

void NotificationBuffer::addNotification(const Notification& notif)
{
    CriticalSection cs(lock);

    while (notifications.size() >= MAX_NOTIFS)
        notifications.pop_front();

    notifications.push_back(Entry(notif, false));
}
