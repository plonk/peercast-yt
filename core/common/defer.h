#ifndef _DEFER_H
#define _DEFER_H

#include <functional>

class Defer
{
public:
    typedef std::function<void()> Action;

    Defer(Action cleanup)
        : m_cleanup(cleanup)
    {
    }

    ~Defer()
    {
        m_cleanup();
    }

    Action m_cleanup;
};

#endif
