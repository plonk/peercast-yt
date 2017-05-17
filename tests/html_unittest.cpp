#include <gtest/gtest.h>

#include "html.h"
#include "sstream.h"
#include "version2.h"

class HTMLFixture : public ::testing::Test {
public:
    HTMLFixture()
        : html("Untitled", mem)
    {
    }

    StringStream mem;
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

TEST_F(HTMLFixture, writeOKwithHeaders)
{
    // html.writeOK("application/octet-stream", { {"Date", "hoge"} });
}

TEST_F(HTMLFixture, locateTo)
{
    html.locateTo("/index.html");
    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /index.html\r\n\r\n",
                 mem.str().c_str());
}

TEST_F(HTMLFixture, addHead)
{
    html.addHead();
    ASSERT_STREQ("<head><title>Untitled</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"></meta></head>", mem.str().c_str());
}


// タイトルはフォーマット文字列として解釈されてはいoけない。
TEST_F(HTMLFixture, addHead2)
{
    StringStream mem2;
    HTML html2("%s%s%s%s%s%s", mem2);

    html2.addHead();
    ASSERT_STREQ("<head><title>%s%s%s%s%s%s</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"></meta></head>", mem2.str().c_str());
}

