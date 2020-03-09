#include <gtest/gtest.h>

#include "channel.h"

class ChanHitListFixture : public ::testing::Test {
public:
    void SetUp()
    {
        hitlist = std::make_shared<ChanHitList>();

        host.fromStrIP("209.209.209.209", 7144);
        global.fromStrIP("210.210.210.210", 7144);
        local.fromStrIP("192.168.0.2", 7144);

        hit.init();
        hit.host = host;
        hit.rhost[0] = global;
        hit.rhost[1] = local;
    }

    void TearDown()
    {
    }

    ChanHit hit;
    std::shared_ptr<ChanHitList> hitlist;
    Host host;
    Host global;
    Host local;
};

TEST_F(ChanHitListFixture, initialState)
{
    ASSERT_EQ(false, hitlist->used);

    ASSERT_EQ(NULL, hitlist->hit);

    ASSERT_EQ(0, hitlist->lastHitTime);
}

TEST_F(ChanHitListFixture, contactTrackers)
{
    // 使われてない。
    ASSERT_EQ(0, hitlist->contactTrackers(false, 0, 0, 0));
}

template <typename T>
static int listCount(T list)
{
    int count = 0;

    while (list != NULL)
    {
        count++;
        list = list->next;
    }
    return count;
}

TEST_F(ChanHitListFixture, addHit)
{
    hitlist->addHit(hit);

    ASSERT_EQ(1, listCount(hitlist));

    // 同じホストを追加しても増えない。
    hitlist->addHit(hit);

    ASSERT_EQ(1, listCount(hitlist));
}

TEST_F(ChanHitListFixture, delHit)
{
}

TEST_F(ChanHitListFixture, deadHit)
{
}

TEST_F(ChanHitListFixture, clearHits)
{
}

TEST_F(ChanHitListFixture, numHits)
{
}

TEST_F(ChanHitListFixture, numListeners)
{
}

TEST_F(ChanHitListFixture, numClaps)
{
}

TEST_F(ChanHitListFixture, numRelays)
{
}

TEST_F(ChanHitListFixture, numFirewalled)
{
}

TEST_F(ChanHitListFixture, numTrackers)
{
}

TEST_F(ChanHitListFixture, closestHit)
{
}

TEST_F(ChanHitListFixture, furthestHit)
{
}

TEST_F(ChanHitListFixture, newestHit)
{
}

TEST_F(ChanHitListFixture, pickHits)
{
}

TEST_F(ChanHitListFixture, pickSourceHits)
{
}

TEST_F(ChanHitListFixture, isUsed)
{
    ASSERT_EQ(false, hitlist->isUsed());
}

TEST_F(ChanHitListFixture, clearDeadHits)
{
}

TEST_F(ChanHitListFixture, createXML)
{
    hitlist->addHit(hit);

    {
        XML::Node* root = hitlist->createXML();
        MemoryStream mem(1024);

        root->write(mem, 0);
        mem.buf[mem.pos] = '\0';

        ASSERT_STREQ("<hits hosts=\"1\" listeners=\"0\" relays=\"0\" firewalled=\"0\" closest=\"0\" furthest=\"0\" newest=\"0\">\n<host ip=\"209.209.209.209:7144\" hops=\"0\" listeners=\"0\" relays=\"0\" uptime=\"0\" push=\"0\" relay=\"1\" direct=\"0\" cin=\"1\" stable=\"0\" version=\"0\" update=\"0\" tracker=\"0\"/>\n</hits>\n", mem.buf);
        delete root;
    }

    {
        XML::Node* root = hitlist->createXML(false);
        MemoryStream mem(1024);

        root->write(mem, 0);
        mem.buf[mem.pos] = '\0';

        ASSERT_STREQ("<hits hosts=\"1\" listeners=\"0\" relays=\"0\" firewalled=\"0\" closest=\"0\" furthest=\"0\" newest=\"0\"/>\n", mem.buf);
        delete root;
    }

}

TEST_F(ChanHitListFixture, deleteHit)
{
    ASSERT_EQ(NULL, hitlist->hit);

    hitlist->addHit(hit);

    ASSERT_EQ(1, listCount(hitlist->hit));
    // ASSERT_NE(NULL, hitlist->hit); // なんでコンパイルできない？

    ASSERT_EQ(NULL, hitlist->deleteHit(hitlist->hit));
    ASSERT_EQ(0, listCount(hitlist->hit));
}

TEST_F(ChanHitListFixture, getTotalListeners)
{
    ASSERT_EQ(0, hitlist->getTotalListeners());

    hit.numListeners = 10;
    hitlist->addHit(hit);

    ASSERT_EQ(10, hitlist->getTotalListeners());
}

TEST_F(ChanHitListFixture, getTotalRelays)
{
    ASSERT_EQ(0, hitlist->getTotalListeners());

    hit.numRelays = 10;
    hitlist->addHit(hit);

    ASSERT_EQ(10, hitlist->getTotalRelays());
}

TEST_F(ChanHitListFixture, getTotalFirewalled)
{
    ASSERT_EQ(0, hitlist->getTotalFirewalled());

    hit.firewalled = true;
    hitlist->addHit(hit);

    ASSERT_EQ(1, hitlist->getTotalFirewalled());
}

// TEST_F(ChanHitListFixture, getSeq)
// {
//     unsigned int seq = hitlist->getSeq();

//     ASSERT_EQ(seq + 1, hitlist->getSeq());
// }
#include "servmgr.h"

TEST_F(ChanHitListFixture, forEachHit)
{
    ChanHit h1, h2, h3;

    hitlist->used = true;

    h1.rhost[0].fromStrIP("0.0.0.1", 7144);
    h2.rhost[0].fromStrIP("0.0.0.2", 7144);
    h3.rhost[0].fromStrIP("0.0.0.3", 7144);

    hitlist->addHit(h1);
    hitlist->addHit(h2);
    hitlist->addHit(h3);

    int count = 0;
    hitlist->forEachHit([&](ChanHit*) { count++; });
    ASSERT_EQ(3, count);
}
