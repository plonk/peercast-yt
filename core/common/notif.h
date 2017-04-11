#ifndef _NOTIF_H
#define _NOTIF_H

#include "common.h"
#include "servmgr.h"
#include <deque>

class NotificationBuffer;

// global
extern NotificationBuffer g_notificationBuffer;

// 通知と通知バッファー。

class Notification
{
public:
    Notification()
        : time(0)
        , type(ServMgr::NT_PEERCAST)
        , message("")
    {
    }

    Notification(unsigned int aTime, ServMgr::NOTIFY_TYPE aType, const std::string& aMessage)
        : time(aTime)
        , type(aType)
        , message(aMessage)
    {
    }

    static std::string getTypeStr(ServMgr::NOTIFY_TYPE type);

    std::string getTypeStr()
    {
        return Notification::getTypeStr(this->type);
    }

    unsigned int time;
    ServMgr::NOTIFY_TYPE type;
    std::string message;
};

class NotificationBuffer
{
public:
    class Entry
    {
    public:
        Entry(const Notification& aNotif, bool aIsRead)
            : notif(aNotif)
            , isRead(aIsRead)
        {
        }

        Notification notif;
        bool isRead;
    };
    static const int MAX_NOTIFS = 100;

    NotificationBuffer();

    int numUnread();
    void markAsRead(unsigned int ctime);
    int numNotifications();
    Entry getNotification(int index);
    void addNotification(const Notification& notif);

    bool writeVariable(Stream& out, const String& varName);
    bool writeVariable(Stream& out, const String& varName, int loopCount);

    std::deque<Entry> notifications;
    WLock lock;
};

#endif
