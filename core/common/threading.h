// ------------------------------------------------
// File : threading.h
// Date: 16-aug-2017
// Desc:
//      Extracted from sys.h. Threading and locking.
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#ifndef _THREADING_H
#define _THREADING_H

#include <atomic>
#include <mutex>
#include <thread>
#include <memory>
#include "waitablequeue.h"
#include "chanpacket.h"

// ------------------------------------
class Message
{
public:
    std::string type;
    virtual ~Message() = default;
};

class PacketMessage : public Message
{
public:
    std::shared_ptr<ChanPacket> packet;
};

// ------------------------------------
class ThreadInfo;
typedef int (*THREAD_FUNC)(ThreadInfo *);
#define THREAD_PROC int
typedef std::thread THREAD_HANDLE;

class ThreadInfo
{
public:
    ThreadInfo()
        : m_active(false)
        , channel(NULL)
    {
        func         = NULL;
        data         = NULL;
    }

    void    shutdown() noexcept;

    bool active() noexcept { return m_active.load(); }

    std::atomic<bool>   m_active;
    THREAD_FUNC     func;
    void            *data;
    std::shared_ptr<class Channel> channel;

    WaitableQueue<std::shared_ptr<Message>> mbox;

    THREAD_HANDLE   handle;
};

#endif
