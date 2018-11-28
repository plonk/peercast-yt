#include <gtest/gtest.h>

#include "atom.h"
#include "channel.h"
#include "sstream.h"

class ChanHitFixture : public ::testing::Test {
public:
    ChanHitFixture()
        : hit(new ChanHit())
    {
    }

    void SetUp()
    {
        hit->init();
    }

    ~ChanHitFixture()
    {
        delete hit;
    }

    ChanHit* hit;
};

// 初期状態を確かめるだけ。
TEST_F(ChanHitFixture, initialState)
{
    ASSERT_EQ(0, hit->host.ip);
    ASSERT_EQ(0, hit->host.port);

    ASSERT_EQ(0, hit->rhost[0].ip);
    ASSERT_EQ(0, hit->rhost[0].port);

    ASSERT_EQ(0, hit->rhost[1].ip);
    ASSERT_EQ(0, hit->rhost[1].port);

    ASSERT_EQ(0, hit->numListeners);
    ASSERT_EQ(0, hit->numRelays);
    ASSERT_EQ(0, hit->numHops);
    // ASSERT_EQ(0, hit->clap_pp);
    ASSERT_EQ(0, hit->time);
    ASSERT_EQ(0, hit->upTime);
    ASSERT_EQ(0, hit->lastContact);
    //ASSERT_EQ(0, hit->hitID);

    ASSERT_FALSE(hit->sessionID.isSet());
    ASSERT_FALSE(hit->chanID.isSet());

    ASSERT_EQ(0, hit->version);
    ASSERT_EQ(0, hit->versionVP);

    ASSERT_EQ(false, hit->firewalled);
    ASSERT_EQ(false, hit->stable);
    ASSERT_EQ(false, hit->tracker);
    ASSERT_EQ(true, hit->recv);
    ASSERT_EQ(false, hit->yp);
    ASSERT_EQ(false, hit->dead);
    ASSERT_EQ(false, hit->direct);
    ASSERT_EQ(true, hit->relay);
    ASSERT_EQ(true, hit->cin);
    // ASSERT_EQ(false, hit->relayfull);
    // ASSERT_EQ(false, hit->chfull);
    // ASSERT_EQ(false, hit->ratefull);

    ASSERT_EQ(NULL, hit->next);

    // ASSERT_EQ(0, hit->status);
    // ASSERT_EQ(0, hit->servent_id);
    ASSERT_EQ(0, hit->oldestPos);
    ASSERT_EQ(0, hit->newestPos);

    ASSERT_EQ(0, hit->uphost.ip);
    ASSERT_EQ(0, hit->uphost.port);

    ASSERT_EQ(0, hit->uphostHops);

    ASSERT_EQ(' ', hit->versionExPrefix[0]);
    ASSERT_EQ(' ', hit->versionExPrefix[1]);

    ASSERT_EQ(0, hit->versionExNumber);
    // ASSERT_EQ(0, hit->lastSendSeq);
}

TEST_F(ChanHitFixture, pickNearestIP)
{
    hit->rhost[0].fromStrIP("210.210.210.210", 8145); // global
    hit->rhost[1].fromStrIP("192.168.0.2", 8145); // local

    Host host;

    {
        // pick local
        host.fromStrIP("192.168.0.1", 7144);

        hit->pickNearestIP(host);

        char ip[32];
        hit->host.toStr(ip);

        ASSERT_STREQ("192.168.0.2:8145", ip);
    }

    {
        // pick global
        host.fromStrIP("209.209.209.209", 7144);

        hit->pickNearestIP(host);

        char ip[32];
        hit->host.toStr(ip);

        ASSERT_STREQ("210.210.210.210:8145", ip);
    }
}

// TEST_F(ChanHitFixture, initLocal_pp_Stealth)
// {
//     hit->numListeners = 100;

//     hit->initLocal_pp(true, 10);

//     ASSERT_EQ(0, hit->numListeners);
//     ASSERT_EQ(10, hit->clap_pp);
// }

// TEST_F(ChanHitFixture, initLocal_pp_NonStealth)
// {
//     hit->numListeners = 100;

//     hit->initLocal_pp(false, 10);

//     ASSERT_EQ(1, hit->numListeners);
//     ASSERT_EQ(10, hit->clap_pp);
// }

TEST_F(ChanHitFixture, createXML)
{
    XML::Node* node = hit->createXML();

    MemoryStream stream(1024);
    node->write(stream, 0);
    int length = stream.pos;

    char buf[1024];
    stream.rewind();
    stream.read(buf, length);
    buf[length] = '\0';

    ASSERT_STREQ("<host ip=\"0.0.0.0:0\" hops=\"0\" listeners=\"0\" relays=\"0\" uptime=\"0\" push=\"0\" relay=\"1\" direct=\"0\" cin=\"1\" stable=\"0\" version=\"0\" update=\"0\" tracker=\"0\"/>\n", buf);

    delete node;
}

TEST_F(ChanHitFixture, writeVariableNonExistent)
{
    MemoryStream mem(1024);
    bool res;

    res = hit->writeVariable(mem, "foo");
    ASSERT_EQ(false, res);
}

TEST_F(ChanHitFixture, writeVariable)
{
#define TEST_VARIABLE(name, value)                          \
    do {                                                    \
        StringStream mem;                                   \
        ASSERT_EQ(true, hit->writeVariable(mem, name));     \
        EXPECT_EQ(value, mem.str());                        \
    } while (0)

    TEST_VARIABLE("rhost0", "0.0.0.0:0");
    TEST_VARIABLE("rhost1", "0.0.0.0:0");
    TEST_VARIABLE("numHops", "0");
    TEST_VARIABLE("numListeners", "0");
    TEST_VARIABLE("numRelays", "0");
    TEST_VARIABLE("uptime", "-");
    TEST_VARIABLE("update", "-");
    TEST_VARIABLE("isFirewalled", "0");
    //TEST_VARIABLE("version", "0"); // original
    TEST_VARIABLE("version", "-");
    // TEST_VARIABLE("agent", "0");
    // TEST_VARIABLE("check", "<a href=\"#\" onclick=\"checkip('0.0.0.0')\">_</a>");
    // TEST_VARIABLE("uphost", "0.0.0.0:0");
    // TEST_VARIABLE("uphostHops", "0");
    // TEST_VARIABLE("canRelay", "1");
}

// 条件変数: chanID.isSet()     +0 +24
//           versionExNumber==0 +0 +20
//           uphost.ip==0       +0 +36
// 2^3 = 8 通りのパスがある。
// 常に送信されるサイズは 8 + 24 + 12+10 + 12+10 + 12*5 + 9 + 12*2 = 169 バイト
TEST_F(ChanHitFixture, writeAtom000)
{
    MemoryStream mem(1024);
    AtomStream writer(mem);

    GnuID chid;
    chid.clear();
    hit->versionExNumber = 0;
    hit->uphost.ip = 0;
    ASSERT_FALSE(chid.isSet());
    hit->writeAtoms(writer, chid);
    ASSERT_EQ(169, mem.pos);
    // ASSERT_EQ(157, mem.pos); // オリジナルの0.1218では157バイトになる。
}

TEST_F(ChanHitFixture, writeAtom001)
{
    MemoryStream mem(1024);
    AtomStream writer(mem);
    GnuID chid;
    chid.clear();
    hit->versionExNumber = 0;
    hit->uphost.ip = 1;
    hit->writeAtoms(writer, chid);
    ASSERT_EQ(169 + 36, mem.pos);
}

TEST_F(ChanHitFixture, writeAtom010)
{
    MemoryStream mem(1024);
    AtomStream writer(mem);
    GnuID chid;
    chid.clear();
    hit->versionExNumber = 1;
    hit->uphost.ip = 0;
    hit->writeAtoms(writer, chid);
    ASSERT_EQ(169 + 20, mem.pos);
}

TEST_F(ChanHitFixture, writeAtom100)
{
    MemoryStream mem(1024);
    AtomStream writer(mem);
    GnuID chid;
    chid.fromStr("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

    hit->versionExNumber = 0;
    hit->uphost.ip = 0;
    hit->writeAtoms(writer, chid);
    ASSERT_EQ(169 + 24, mem.pos);
}

TEST_F(ChanHitFixture, initLocal)
{
    Channel channel;

    int numl = 1;
    int numr = 2;
    int nums = 3; // 使われていない。
    int uptm = 4;
    bool connected = true;
    bool isFull = true;
    unsigned int bitrate = 5; // 使われていない。
    Channel* ch = &channel;
    unsigned int oldp = 6;
    unsigned int newp = 7;

    hit->initLocal(numl, numr, nums, uptm, connected,
                   /* isFull, bitrate, ch, */
                   oldp, newp);

    ASSERT_EQ(1, hit->numListeners);
    ASSERT_EQ(2, hit->numRelays);

    ASSERT_EQ(4, hit->upTime);
    ASSERT_EQ(true, hit->recv);
    // ASSERT_EQ(true, hit->chfull);

    ASSERT_EQ(6, hit->oldestPos);
    ASSERT_EQ(7, hit->newestPos);
}

TEST_F(ChanHitFixture, str)
{
    ASSERT_STREQ("0.0.0.0", hit->str().c_str());
}

TEST_F(ChanHitFixture, strOriginal)
{
    hit->version = 1218;
    ASSERT_STREQ("0.0.0.0 (1218)", hit->str().c_str());
}

TEST_F(ChanHitFixture, strVP)
{
    hit->version = 1218;
    hit->versionVP = 26;
    ASSERT_STREQ("0.0.0.0 (VP26)", hit->str().c_str());
}

TEST_F(ChanHitFixture, strIM)
{
    hit->version = 1218;
    hit->versionVP = 27;
    hit->versionExPrefix[0] = 'I';
    hit->versionExPrefix[1] = 'M';
    hit->versionExNumber = 51;
    ASSERT_STREQ("0.0.0.0 (IM51)", hit->str().c_str());
}

TEST_F(ChanHitFixture, versionString)
{
    ASSERT_STREQ("", hit->versionString().c_str());
}

TEST_F(ChanHitFixture, versionStringOriginal)
{
    hit->version = 1218;
    ASSERT_STREQ("1218", hit->versionString().c_str());
}

TEST_F(ChanHitFixture, versionStringVP)
{
    hit->version = 1218;
    hit->versionVP = 26;
    ASSERT_STREQ("VP26", hit->versionString().c_str());
}

TEST_F(ChanHitFixture, versionStringIM)
{
    hit->version = 1218;
    hit->versionVP = 27;
    hit->versionExPrefix[0] = 'I';
    hit->versionExPrefix[1] = 'M';
    hit->versionExNumber = 51;
    ASSERT_STREQ("IM51", hit->versionString().c_str());
}

TEST_F(ChanHitFixture, colorRed)
{
    ASSERT_EQ(0, hit->host.port);
    ASSERT_EQ(ChanHit::Color::red, hit->getColor());
}

TEST_F(ChanHitFixture, colorPurple)
{
    hit->host.port = 7144;
    hit->relay = false;
    hit->numRelays = 0;
    ASSERT_EQ(ChanHit::Color::purple, hit->getColor());
}

TEST_F(ChanHitFixture, colorBlue)
{
    hit->host.port = 7144;
    hit->relay = false;
    hit->numRelays = 1;
    ASSERT_EQ(ChanHit::Color::blue, hit->getColor());
}

TEST_F(ChanHitFixture, colorGreen)
{
    hit->host.port = 7144;
    hit->relay = true;
    ASSERT_EQ(ChanHit::Color::green, hit->getColor());
}

