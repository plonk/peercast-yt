#ifndef MOCKSYS_H
#define MOCKSYS_H

#include "sys.h"

class MockSys : public Sys
{
public:
    MockSys()
    {
    }

    virtual class ClientSocket* createSocket()
    {
        return NULL;
    }

    virtual bool startThread(class ThreadInfo*) {
        return false;
    }

    virtual void sleep(int)
    {
    }

    virtual void appMsg(long, long = 0)
    {
    }

    virtual unsigned int getTime()
    {
        return 0;
    }

    virtual double getDTime()
    {
        return 0;
    }

    virtual unsigned int rnd()
    {
        return 123456789;
    }

    // URLをブラウザやメーラで開く。
    virtual void getURL(const char*)
    {
    }

    virtual void exit()
    {
    }

    virtual bool hasGUI()
    {
        return false;
    }

    // ローカルサーバーのURLを開く。
    virtual void callLocalURL(const char*, int)
    {
    }

    // ファイルを開く。
    virtual void executeFile(const char*)
    {
    }

    virtual void endThread(ThreadInfo*)
    {
    }

    virtual void waitThread(ThreadInfo*, int timeout = 30000)
    {
    }
};

#endif
