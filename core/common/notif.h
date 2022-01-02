#ifndef _NOTIF_H
#define _NOTIF_H

#include "common.h"
#include "servmgr.h"
#include "varwriter.h"
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

class NotificationBuffer : public VariableWriter
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
    static const int MAX_NOTIFS = 20;

    NotificationBuffer();

    int numUnread();
    void markAsRead(unsigned int ctime);
    int numNotifications();
    Entry getNotification(int index);
    void addNotification(const Notification& notif);

    amf0::Value getState() override;

    std::deque<Entry> notifications;
    std::recursive_mutex lock;
};

#endif
