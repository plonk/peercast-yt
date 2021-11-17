#include <gtest/gtest.h>

#include "chanpacket.h"
#include "sys.h"
#include "mocksys.h"

class ChanPacketBufferFixture : public ::testing::Test {
public:
    ChanPacketBufferFixture()
    {
        time_ = dynamic_cast<MockSys*>(sys)->time;
        dynamic_cast<MockSys*>(sys)->time = 12345;
    }

    ~ChanPacketBufferFixture()
    {
        dynamic_cast<MockSys*>(sys)->time = time_;
    }
    ChanPacketBuffer data;
    unsigned int time_;
};

TEST_F(ChanPacketBufferFixture, init)
{
    data.init();

    ASSERT_EQ(0, data.lastPos);
    ASSERT_EQ(0, data.firstPos);
    ASSERT_EQ(0, data.safePos);
    ASSERT_EQ(0, data.readPos);
    ASSERT_EQ(0, data.writePos);
    ASSERT_EQ(0, data.accept);
    ASSERT_EQ(0, data.lastWriteTime);

    ASSERT_EQ(0, data.numPending());
}

TEST_F(ChanPacketBufferFixture, addPacket)
{

    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    data.writePacket(packet);

    EXPECT_EQ(0, data.lastPos);
    EXPECT_EQ(0, data.firstPos);
    EXPECT_EQ(0, data.safePos);
    EXPECT_EQ(0, data.readPos);
    EXPECT_EQ(1, data.writePos);
    EXPECT_EQ(0, data.accept);
    EXPECT_EQ(12345, data.lastWriteTime);
    EXPECT_EQ(1, data.numPending());
}

TEST_F(ChanPacketBufferFixture, addPacket_init)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    data.writePacket(packet);

    EXPECT_EQ(1, data.writePos);
    EXPECT_EQ(1, data.numPending());

    // -------------------------------

    data.init();

    ASSERT_EQ(0, data.lastPos);
    ASSERT_EQ(0, data.firstPos);
    ASSERT_EQ(0, data.safePos);
    ASSERT_EQ(0, data.readPos);
    ASSERT_EQ(0, data.writePos);
    ASSERT_EQ(0, data.accept);
    ASSERT_EQ(0, data.lastWriteTime);

    ASSERT_EQ(0, data.numPending());
}

TEST_F(ChanPacketBufferFixture, readPacket)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    for (int i = 0; i < 8192; i++)
        packet.data[i] = i%256;

    data.writePacket(packet);

    // -------------------------------

    ChanPacket opacket;
    data.readPacket(opacket);

    ASSERT_EQ(opacket.type, ChanPacket::T_DATA);
    ASSERT_EQ(opacket.len, 8192);
    ASSERT_EQ(opacket.pos, 0);

    for (int i = 0; i < 8192; i++)
        ASSERT_EQ(opacket.data[i], static_cast<char>(i % 256));

    ASSERT_EQ(0, data.lastPos);
    ASSERT_EQ(0, data.firstPos);
    ASSERT_EQ(0, data.safePos);
    ASSERT_EQ(1, data.readPos);
    ASSERT_EQ(1, data.writePos);
    ASSERT_EQ(0, data.accept);
    ASSERT_EQ(12345, data.lastWriteTime);

    ASSERT_EQ(0, data.numPending());
}

TEST_F(ChanPacketBufferFixture, writePos)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    int i;
    for (i = 0; i < data.m_maxPackets; i++) {
        ASSERT_EQ(data.writePos, i);
        data.writePacket(packet);
    }

    ASSERT_EQ(data.writePos, i);
}

TEST_F(ChanPacketBufferFixture, lastPos)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    int i;
    for (i = 0; i < data.m_maxPackets; i++) {
        // lastPos goes 0, 0, 1, 2, 3, ...
        if (i == 0)
            ASSERT_EQ(data.lastPos, i);
        else
            ASSERT_EQ(data.lastPos, i - 1);
        data.writePacket(packet);
    }

    ASSERT_EQ(data.lastPos, i - 1);
}

TEST_F(ChanPacketBufferFixture, writePacket_fails_after_taking_in_MAX_PACKETS)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    for (int i = 0; i < data.m_maxPackets; i++) {
        ASSERT_TRUE( data.writePacket(packet) );
    }

    ASSERT_FALSE( data.writePacket(packet) ); // 65th packet
}

// バッファーの最初のアイテムのインデックス。バッファーが空の場合は 0。
TEST_F(ChanPacketBufferFixture, firstPos)
{
    ChanPacket packet;
    ChanPacket out;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    // (MAX_PACKETS 0's), 1, 2, ...
    for (int i = 0; i < data.m_maxPackets; i++) {
        ASSERT_EQ(data.writePos, i);
        ASSERT_EQ(data.firstPos, 0);

        data.writePacket(packet);
        data.readPacket(out); // advance readPos so that the buffer will not be full
    }
    ASSERT_EQ(data.writePos, 64);
    ASSERT_EQ(data.firstPos, 0);

#if 0 // 今ではバッファーは拡張されるので firstPos は 0 のままになる。

    ASSERT_TRUE( data.writePacket(packet) ); // 65th packet
    ASSERT_EQ(data.writePos, 65);
    ASSERT_EQ(data.firstPos, 1);

    ASSERT_TRUE( data.writePacket(packet) ); // 66th packet
    ASSERT_EQ(data.writePos, 66);
    ASSERT_EQ(data.firstPos, 2);
#endif
}

#include <thread>
#include <functional>

//#include <stdio.h>

// 本当の実装を呼び出す。
class MySys : public MockSys {
    void sleep(int ms) override
    {
        return this->Sys::sleep(ms);
    }

    unsigned int getTime() override
    {
        return this->Sys::getTime();
    }
};

TEST_F(ChanPacketBufferFixture, readPacket_mutithreaded)
{
    Sys *sys_ = sys;
    sys = new MySys();

    ChanPacket packet;
    ChanPacket out;

    packet.type = ChanPacket::T_DATA;
    packet.len = 4;
    packet.pos = 0;
    memcpy(packet.data, "HELLO", 4);

    std::function<void(void)> consumerProc = [&] {
        //sys->sleep(100);
        //fprintf(stderr, "before read\n");
        data.readPacket(out);
        //fprintf(stderr, "after read\n");
        ASSERT_EQ(out.len, 4);
        ASSERT_EQ(memcmp(out.data, "HELLO", 4), 0);
    };
    std::thread consumer(consumerProc);
    
    //sys->sleep(100);
    //fprintf(stderr, "before write\n");
    ASSERT_TRUE( data.writePacket(packet) );
    //fprintf(stderr, "after write\n");

    consumer.join();
    ASSERT_EQ(memcmp(out.data, "HELLO", 4), 0);

    sys = sys_;
}

TEST_F(ChanPacketBufferFixture, updateReadPos_true)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 4;
    packet.pos = 0;
    memcpy(packet.data, "HELLO", 4);

    ASSERT_TRUE( data.writePacket(packet, /*updateReadPos*/true) );

    ASSERT_EQ(data.writePos, 1);
    ASSERT_EQ(data.readPos, 1);
    ASSERT_EQ(data.numPending(), 0);
}

TEST_F(ChanPacketBufferFixture, updateReadPos_false)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 4;
    packet.pos = 0;
    memcpy(packet.data, "HELLO", 4);

    ASSERT_TRUE( data.writePacket(packet, /*updateReadPos*/false) );

    ASSERT_EQ(data.writePos, 1);
    ASSERT_EQ(data.readPos, 0);
    ASSERT_EQ(data.numPending(), 1);
}

