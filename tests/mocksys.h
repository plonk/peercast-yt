#ifndef MOCKSYS_H
#define MOCKSYS_H

#include "sys.h"

class MockSys : public Sys
{
public:
    MockSys()
    {
    }

    std::shared_ptr<ClientSocket> createSocket() override
    {
        return NULL;
    }

    bool startThread(class ThreadInfo*) override
    {
        return false;
    }

    bool startWaitableThread(class ThreadInfo*) override
    {
        return false;
    }

    void sleep(int) override
    {
    }

    unsigned int getTime() override
    {
        return 0;
    }

    double getDTime() override
    {
        return 0;
    }

    unsigned int rnd() override
    {
        return 123456789;
    }

    // URLをブラウザやメーラで開く。
    void getURL(const char*) override
    {
    }

    void exit() override
    {
    }

    bool hasGUI() override
    {
        return false;
    }

    // ローカルサーバーのURLを開く。
    void callLocalURL(const char*, int) override
    {
    }

    // ファイルを開く。
    void executeFile(const char*) override
    {
    }

    void waitThread(ThreadInfo*) override
    {
    }

    std::string getHostname() override
    {
        return "localhost";
    }

    std::vector<std::string> getIPAddresses(const std::string& name) override
    {
        if (name == "localhost") {
            return { "127.0.0.1" };
        } else if (name == "ip6-localhost") {
            return { "::1" };
        } else {
            return {};
        }
    }

    bool getHostnameByAddress(const IP& ip, std::string& out) override
    {
        if (ip.str() == "127.0.0.1") {
            out = "localhost"; return true;
        } else if (ip.str() == "::1") {
            out = "ip6-localhost"; return true;
        } else if (ip.str() == "8.8.4.4") {
            out = "dns.google"; return true;
        } else if (ip.str() == "8.8.8.8") {
            out = "dns.google"; return true;
        } else {
            out = ""; return false;
        }
    }

    std::string getExecutablePath() override
    {
        return "";
    }

    std::string dirname(const std::string&) override
    {
        return "";
    }

    std::string joinPath(const std::vector<std::string>&) override
    {
        return "";
    }

    std::string realPath(const std::string& path) override
    {
        return "";
    }
};

#endif
