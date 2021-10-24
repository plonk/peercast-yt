#include <gtest/gtest.h>

#include "sys.h"
#ifdef _UNIX
#include "usys.h"
#else
#include "wsys.h"
#endif

class SysFixture : public ::testing::Test {
public:
    SysFixture()
    {
#ifdef _UNIX
        m_sys = new USys();
#else
        m_sys = new WSys(0);
#endif
    }

    void SetUp()
    {
        m_done = false;
    }

    void TearDown()
    {
    }

    ~SysFixture()
    {
        delete m_sys;
    }

    static THREAD_PROC proc(ThreadInfo* p)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        m_done = true;
        return 0;
    }

    static bool m_done;
    Sys* m_sys;
};

bool SysFixture::m_done;

TEST_F(SysFixture, strdup)
{
    const char *orig = "abc";
    char *copy = Sys::strdup(orig);
    ASSERT_STREQ("abc", copy);
    ASSERT_NE(orig, copy);
    free(copy);
}

TEST_F(SysFixture, stricmp)
{
    ASSERT_EQ(0, Sys::stricmp("", ""));
    ASSERT_EQ(0, Sys::stricmp("a", "a"));
    ASSERT_EQ(0, Sys::stricmp("a", "A"));
    ASSERT_EQ(-1, Sys::stricmp("a", "b"));
    ASSERT_TRUE(Sys::stricmp("a", "b") < 0);
    ASSERT_TRUE(Sys::stricmp("a", "B") < 0);
    ASSERT_TRUE(Sys::stricmp("b", "a") > 0);
    ASSERT_TRUE(Sys::stricmp("b", "A") > 0);
}

// #include <strings.h>
TEST_F(SysFixture, strnicmp)
{
    // ASSERT_EQ(0, strncasecmp("", "", 0));
    // ASSERT_EQ(0, strncasecmp("", "", 1));
    // ASSERT_EQ(0, strncasecmp("", "", 100));

    // ASSERT_EQ(0, strncasecmp("a", "a", 0));
    // ASSERT_EQ(0, strncasecmp("a", "a", 1));
    // ASSERT_EQ(0, strncasecmp("a", "a", 100));

    // ASSERT_EQ(0, strncasecmp("a", "A", 0));
    // ASSERT_EQ(0, strncasecmp("a", "A", 1));
    // ASSERT_EQ(0, strncasecmp("a", "A", 100));

    // ASSERT_EQ(0, strncasecmp("a", "b", 0));
    // ASSERT_NE(0, strncasecmp("a", "b", 1));
    // ASSERT_NE(0, strncasecmp("a", "b", 100));

    ASSERT_EQ(0, Sys::strnicmp("", "", 0));
    ASSERT_EQ(0, Sys::strnicmp("", "", 1));
    ASSERT_EQ(0, Sys::strnicmp("", "", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "a", 0));
    ASSERT_EQ(0, Sys::strnicmp("a", "a", 1));
    ASSERT_EQ(0, Sys::strnicmp("a", "a", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "A", 0));
    ASSERT_EQ(0, Sys::strnicmp("a", "A", 1));
    ASSERT_EQ(0, Sys::strnicmp("a", "A", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "b", 0));
    ASSERT_NE(0, Sys::strnicmp("a", "b", 1));
    ASSERT_NE(0, Sys::strnicmp("a", "b", 100));
}

TEST_F(SysFixture, strcpy_truncate)
{
    char dest[10] = "fuga";

    ASSERT_STREQ("hoge", Sys::strcpy_truncate(dest, 10, "hoge"));
    ASSERT_STREQ("f", Sys::strcpy_truncate(dest, 2, "fuga"));
    ASSERT_STREQ("", Sys::strcpy_truncate(dest, 1, "piyo"));
}

TEST_F(SysFixture, sleep)
{
    auto t0 = m_sys->getDTime();
    m_sys->sleep(1);
    auto t1 = m_sys->getDTime();
    //ASSERT_TRUE(t1 - t0 < 1.000);
    ASSERT_TRUE(t1 - t0 >= 0.001);
}

TEST_F(SysFixture, getTime)
{
    ASSERT_LT(0, m_sys->getTime());
}

TEST_F(SysFixture, waitThread)
{
    ThreadInfo info;
    info.func = proc;
    m_sys->startWaitableThread(&info);
    m_sys->waitThread(&info);
    ASSERT_TRUE(m_done);
}


TEST_F(SysFixture, getHostnameByAddressIPv4)
{
    std::string str;
    EXPECT_TRUE(m_sys->getHostnameByAddress(IP::parse("8.8.4.4"), str));
    EXPECT_STREQ("dns.google", str.c_str());
}

TEST_F(SysFixture, getHostnameByAddressIPv6)
{
    std::string str;
    EXPECT_TRUE(m_sys->getHostnameByAddress(IP::parse("2001:4860:4860::8844"), str));
    EXPECT_STREQ("dns.google", str.c_str());
}

// Travis CI でコケるので外す。
#if 0
#include <thread>

static ClientSocket* serv;
static void serverProc(Sys* sys)
{
    serv = sys->createSocket();
    Host servhost(0, 7144);
    serv->bind(servhost);
    serv->accept();
}

TEST_F(SysFixture, createSocket)
{
    ClientSocket* sock;

    ASSERT_NO_THROW(sock = m_sys->createSocket());
    ASSERT_NE(nullptr, sock);

    Host h;
    ASSERT_THROW(h = sock->getLocalHost(), SockException);

    delete sock;

    {
        std::thread serverThread(serverProc, m_sys);
        sock = m_sys->createSocket();
        ASSERT_NO_THROW(sock->open(Host(127 << 24 | 1, 7144)));
        m_sys->sleep(500); // let the server bind
        ASSERT_NO_THROW(sock->connect());
        ASSERT_NO_THROW(h = sock->getLocalHost());
        ASSERT_EQ("127.0.0.1:0", h.str());
        serverThread.join();
        delete sock;
        delete serv;
    }

    {
        std::thread serverThread(serverProc, m_sys);
        sock = m_sys->createSocket();
        ASSERT_NO_THROW(sock->open(Host(IP::parse("::1"), 7144)));
        m_sys->sleep(500); // let the server bind
        ASSERT_NO_THROW(sock->connect());
        ASSERT_NO_THROW(h = sock->getLocalHost());
        ASSERT_EQ("[::1]:0", h.str());
        serverThread.join();
        delete sock;
        delete serv;
    }
}
#endif

TEST_F(SysFixture, getExecutablePath)
{
    std::string str;
    ASSERT_NO_THROW(str = m_sys->getExecutablePath());
    ASSERT_TRUE(str.size() > 1);
#if _UNIX
    ASSERT_EQ('/', str[0]);
#endif
}

TEST_F(SysFixture, dirname)
{
#if _UNIX
    ASSERT_EQ("/usr", m_sys->dirname("/usr/lib"));
    ASSERT_EQ("/", m_sys->dirname("/usr/"));
    ASSERT_EQ(".", m_sys->dirname("usr"));
    ASSERT_EQ("/", m_sys->dirname("/"));
#else
    /* 今のところWindows版で実装されていなくても構わない。 */
    ASSERT_THROW(m_sys->dirname(""),
                 NotImplementedException);
#endif
}

TEST_F(SysFixture, joinPath)
{
#if _UNIX
    ASSERT_EQ("/etc/passwd", m_sys->joinPath( {"/etc", "passwd"} ));
    ASSERT_EQ("/etc/passwd", m_sys->joinPath( {"/etc", "", "passwd"} ));
    ASSERT_EQ("/etc/passwd", m_sys->joinPath( {"/etc/", "passwd"} ));
    ASSERT_EQ("/etc/passwd", m_sys->joinPath( {"/etc/", "/passwd"} ));
    ASSERT_EQ("/etc/passwd", m_sys->joinPath( {"/etc/", "/", "/passwd"} ));
    ASSERT_EQ("./passwd", m_sys->joinPath( {".", "passwd"} ));
#else
    /* 今のところWindows版で実装されていなくても構わない。 */
    ASSERT_THROW(m_sys->joinPath({}),
                 NotImplementedException);
#endif
}

// 実際に引いてみる。dns.google は現在 IPv4 アドレスが２つ、IPv6 アド
// レスが 2 つ設定されている。このテストは、テストを走らせるマシンに
// IPv4 か IPv6 のアドレスが設定されていることを仮定している。
TEST_F(SysFixture, getIPAddresses)
{
    auto vec = m_sys->getIPAddresses("dns.google");

    // 少なくとも２つのアドレスが返る。
    ASSERT_GE(vec.size(), 2);
}

