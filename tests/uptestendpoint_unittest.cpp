#include <gtest/gtest.h>
#include "uptest.h"

class UptestEndpointFixture : public ::testing::Test {
public:
    UptestEndpointFixture()
        : e("http://example.com")
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~UptestEndpointFixture()
    {
    }
    UptestEndpoint e;
};

TEST_F(UptestEndpointFixture, initialState)
{
    ASSERT_EQ("http://example.com", e.url);
    ASSERT_EQ(UptestEndpoint::kUntried, e.status);
    ASSERT_EQ(UptestInfo(), e.m_info);
    ASSERT_EQ(0, e.lastTriedAt);
    ASSERT_EQ("", e.m_xml);
}

TEST_F(UptestEndpointFixture, readInfo)
{
    std::string xml = "<yp4g>"
        "<yp name=\"YP\"/>"
        "<host ip=\"192.168.0.1\" port_open=\"1\" speed=\"999\" over=\"0\"/>"
        "<uptest checkable=\"1\" remain=\"0\"/>"
        "<uptest_srv addr=\"example.com\" port=\"443\" object=\"/yp/uptest.cgi\" post_size=\"250\" limit=\"3000\" interval=\"15\" enabled=\"1\"/>"
        "</yp4g>";

    auto info = UptestEndpoint::readInfo(xml);
    ASSERT_EQ("YP", info.name);
    ASSERT_EQ("192.168.0.1", info.ip);
    ASSERT_EQ("1", info.port_open);
    ASSERT_EQ("999", info.speed);
    ASSERT_EQ("0", info.over);
    ASSERT_EQ("1", info.checkable);
    ASSERT_EQ("0", info.remain);
    ASSERT_EQ("example.com", info.addr);
    ASSERT_EQ("443", info.port);
    ASSERT_EQ("/yp/uptest.cgi", info.object);
    ASSERT_EQ("250", info.post_size);
    ASSERT_EQ("3000", info.limit);
    ASSERT_EQ("15", info.interval);
    ASSERT_EQ("1", info.enabled);
}

TEST_F(UptestEndpointFixture, isReady)
{
    ASSERT_EQ(UptestEndpoint::kUntried, e.status);
    ASSERT_TRUE(e.isReady());
}

// Returns false because sys->getTime() == 0 and e.lastTriedAt == 0.
TEST_F(UptestEndpointFixture, isReady_Error)
{
    e.status = UptestEndpoint::kError;
    ASSERT_FALSE(e.isReady());
}

// ditto
TEST_F(UptestEndpointFixture, isReady_Success)
{
    e.status = UptestEndpoint::kSuccess;
    ASSERT_FALSE(e.isReady());
}

TEST_F(UptestEndpointFixture, generateRandomBytes)
{
    ASSERT_EQ("", UptestEndpoint::generateRandomBytes(0));
    ASSERT_EQ(1024, UptestEndpoint::generateRandomBytes(1024).size());
}
