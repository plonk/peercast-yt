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
#if defined(__MINGW32__) || defined(__MINGW64__)
#include "mingw.mutex.h"
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include "mingw.thread.h"
#endif
#include <thread>

// ------------------------------------
// class WEvent

#ifdef WIN32
#include <windows.h>
#include "common.h" // TimeoutException

class WEvent
{
public:
    WEvent()
    {
        event = ::CreateEvent(NULL,  // no security attributes
                              TRUE,  // manual-reset
                              FALSE, // initially non-signaled
                              NULL); // anonymous
    }

    ~WEvent()
    {
        ::CloseHandle(event);
    }

    void    signal()
    {
        ::SetEvent(event);
    }

    void    wait(int timeout = 30000)
    {
        switch(::WaitForSingleObject(event, timeout))
        {
        case WAIT_TIMEOUT:
            throw TimeoutException();
            break;
        //case WAIT_OBJECT_0:
            //break;
        }
    }

    void    reset()
    {
        ::ResetEvent(event);
    }

    HANDLE event;
};
#else
// class WEvent Unix implementation

class WEvent
{
public:

    WEvent()
    {
    }

    void    signal()
    {
    }

    void    wait(int timeout = 30000)
    {
    }

    void    reset()
    {
    }
};
#endif

// ------------------------------------
class WLock
{
private:
    std::recursive_mutex m_mutex;

public:
    void    on()
    {
        m_mutex.lock();
    }

    void    off()
    {
        m_mutex.unlock();
    }
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

    THREAD_HANDLE   handle;
};

#endif
