#ifndef _CRITSEC_H
#define _CRITSEC_H

// クリティカルセクションをマークする。コンストラクターでロックを取得
// し、デストラクタで開放する。
class CriticalSection
{
public:
    CriticalSection(WLock& lock)
        : m_lock(lock)
    {
        m_lock.on();
    }

    ~CriticalSection()
    {
        m_lock.off();
    }

    WLock& m_lock;
};

#endif
