#include <gtest/gtest.h>

#include "url.h"

class URLSourceFixture : public ::testing::Test {
public:
    URLSourceFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~URLSourceFixture()
    {
    }
};

TEST_F(URLSourceFixture, getSourceProtocol)
{
    char* p;

    p = (char*)"http://example.com/source";
    ASSERT_EQ(ChanInfo::SP_HTTP, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("example.com/source", p);

    p = (char*)"mms://example.com/source";
    ASSERT_EQ(ChanInfo::SP_MMS, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("example.com/source", p);

    p = (char*)"pcp://example.com/source";
    ASSERT_EQ(ChanInfo::SP_PCP, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("example.com/source", p);

    p = (char*)"file:///home/user/source";
    ASSERT_EQ(ChanInfo::SP_FILE, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("/home/user/source", p);

    p = (char*)"/home/user/source";
    ASSERT_EQ(ChanInfo::SP_FILE, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("/home/user/source", p);

    p = (char*)"rtmp://example.com/source";
    ASSERT_EQ(ChanInfo::SP_RTMP, URLSource::getSourceProtocol(p));
    ASSERT_STREQ("example.com/source", p);
}
