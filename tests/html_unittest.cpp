#include <gtest/gtest.h>

#include "html.h"
#include "dmstream.h"
#include "version2.h"

class HTMLFixture : public ::testing::Test {
public:
    HTMLFixture()
        : html("Untitled", mem)
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~HTMLFixture()
    {
    }

    DynamicMemoryStream mem;
    HTML html;
};

TEST_F(HTMLFixture, initialState)
{
    ASSERT_STREQ("Untitled", html.title.cstr());
    ASSERT_EQ(0, html.tagLevel);
    ASSERT_EQ(0, html.refresh);
}

TEST_F(HTMLFixture, writeOK)
{
    html.writeOK("application/octet-stream");
    // ASSERT_STREQ("HTTP/1.0 200 OK\r\nServer: "
    //              PCX_AGENT "\r\n"
    //              "Connection: close\r\n"
    //              "Content-Type: application/octet-stream\r\n"
    //              "\r\n", mem.str().c_str());
    ASSERT_STREQ("HTTP/1.0 200 OK\r\nServer: "
                 PCX_AGENT "\r\n"
                 "Connection: close\r\n"
                 "Content-Type: application/octet-stream\r\n"
                 "Date: Thu, 01 Jan 1970 00:00:00 GMT\r\n"
                 "\r\n", mem.str().c_str());
}

TEST_F(HTMLFixture, writeOKwithAdditionalHeaders)
{
    // html.writeOK("application/octet-stream", { {"Date", "hoge"} });
}
