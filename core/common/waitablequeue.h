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
#ifndef _WAITABLEQUEUE_H
#define _WAITABLEQUEUE_H

#include <queue>
#include <condition_variable>

template <typename T>
class WaitableQueue
{
public:
    WaitableQueue() {}

    void enqueue(T t)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(t);
        m_cv.notify_one();
    }

    T dequeue()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });
        auto t = m_queue.front();
        m_queue.pop();
        return t;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<T> m_queue;
};

#endif
