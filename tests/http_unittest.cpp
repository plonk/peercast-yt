#include <gtest/gtest.h>

#include "sstream.h"
#include "http.h"

class HTTPFixture : public ::testing::Test {
public:
    HTTPFixture()
        : http(mem)
    {
    }
    StringStream mem;
    HTTP http;
};

TEST_F(HTTPFixture, readResponse)
{
    mem.str("HTTP/1.0 200 OK\r\n");
    int statusCode = http.readResponse();

    ASSERT_EQ(200, statusCode);
    // 副作用として cmdLine がちょんぎれる。
    ASSERT_STREQ("HTTP/1.0 200", http.cmdLine);
    ASSERT_TRUE(mem.eof());
}

TEST_F(HTTPFixture, checkResponseReturnsTrue)
{
    mem.str("HTTP/1.0 200 OK\r\n");

    ASSERT_TRUE(http.checkResponse(200));
    ASSERT_TRUE(mem.eof());
}


TEST_F(HTTPFixture, checkResponseThrows)
{
    mem.str("HTTP/1.0 404 Not Found\r\n");

    ASSERT_THROW(http.checkResponse(200), StreamException);
    ASSERT_TRUE(mem.eof());
}

TEST_F(HTTPFixture, readRequest)
{
    mem.str("GET /index.html HTTP/1.0\r\n");
    http.readRequest();
    ASSERT_STREQ("GET /index.html HTTP/1.0", http.cmdLine);
    ASSERT_TRUE(mem.eof());
}

TEST_F(HTTPFixture, nextHeader)
{
    mem.str("GET /index.html HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n");

    http.readRequest();
    ASSERT_EQ(0, http.headers.size());
    ASSERT_EQ(NULL, http.arg);

    ASSERT_TRUE(http.nextHeader());
    ASSERT_EQ(1, http.headers.size());
    ASSERT_STREQ("localhost", http.arg);

    ASSERT_TRUE(http.nextHeader());
    ASSERT_EQ(2, http.headers.size());
    ASSERT_STREQ("close", http.arg);

    ASSERT_FALSE(http.nextHeader());
    ASSERT_EQ(2, http.headers.size());
    ASSERT_EQ(NULL, http.arg);

    ASSERT_STREQ("localhost", http.headers.get("Host").c_str());
    ASSERT_STREQ("close", http.headers.get("Connection").c_str());
}

TEST_F(HTTPFixture, isHeader)
{
    mem.str("GET /index.html HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n");

    http.readRequest();

    ASSERT_TRUE(http.nextHeader());
    ASSERT_TRUE(http.isHeader("Host"));
    ASSERT_TRUE(http.isHeader("host")); // case-insensitive
    ASSERT_TRUE(http.isHeader("localhost")); // 値の部分にもマッチしちゃう
    ASSERT_TRUE(http.isHeader("h")); // 実は前方一致
    ASSERT_FALSE(http.isHeader("")); // でも空文字列はダメ

    ASSERT_TRUE(http.nextHeader());
    ASSERT_TRUE(http.isHeader("Connection"));

    ASSERT_FALSE(http.nextHeader());
}

TEST_F(HTTPFixture, isRequest)
{
    mem.str("GET /index.html HTTP/1.0\r\n");

    http.readRequest();

    ASSERT_TRUE(http.isRequest("GET"));
    ASSERT_FALSE(http.isRequest("POST"));
    ASSERT_FALSE(http.isRequest("get")); // case-sensitive
    ASSERT_TRUE(http.isRequest("G"));
    ASSERT_FALSE(http.isRequest("ET"));
    ASSERT_TRUE(http.isRequest("GET "));
    ASSERT_TRUE(http.isRequest(""));
}

TEST_F(HTTPFixture, getArgStr)
{
    ASSERT_EQ(NULL, http.arg);
    ASSERT_EQ(NULL, http.getArgStr());

    http.arg = (char*)"hoge";
    ASSERT_EQ(http.arg, http.getArgStr());
}

TEST_F(HTTPFixture, getArgInt)
{
    http.arg = NULL;

    ASSERT_EQ(NULL, http.arg);
    ASSERT_EQ(0, http.getArgInt());

    http.arg = (char*)"123";
    ASSERT_EQ(123, http.getArgInt());

    http.arg = (char*)"";
    ASSERT_EQ(0, http.getArgInt());

    http.arg = (char*)"hoge";
    ASSERT_EQ(0, http.getArgInt());
}

TEST_F(HTTPFixture, getAuthUserPass)
{
    mem.str("Authorization: BASIC OlBhc3N3MHJk\r\n");

    ASSERT_TRUE(http.nextHeader());
    ASSERT_TRUE(http.isHeader("Authorization"));
    char user[100] = "dead", pass[100] = "beef";
    http.getAuthUserPass(user, pass, 100, 100);

    ASSERT_STREQ("", user);
    ASSERT_STREQ("Passw0rd", pass);
}

TEST_F(HTTPFixture, getAuthUserPass2)
{
    mem.str("HOGEHOGEHOGE: hogehogehoge\r\n");

    ASSERT_TRUE(http.nextHeader());
    char user[100] = "dead", pass[100] = "beef";
    http.getAuthUserPass(user, pass, 100, 100);

    ASSERT_STREQ("dead", user);
    ASSERT_STREQ("beef", pass);
}

TEST_F(HTTPFixture, initRequest)
{
    http.initRequest("GET /index.html HTTP/1.0\r\n");
    // readRequest と違って、改行コードは削除されない。
    ASSERT_STREQ("GET /index.html HTTP/1.0\r\n", http.cmdLine);
}

TEST_F(HTTPFixture, reset)
{
    mem.str("GET /index.html HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "\r\n");

    http.readRequest();
    http.nextHeader();

    ASSERT_STREQ("Host: localhost", http.cmdLine);
    ASSERT_STREQ("localhost", http.arg);
    ASSERT_EQ(1, http.headers.size());

    http.reset();

    ASSERT_STREQ("", http.cmdLine);
    ASSERT_EQ(NULL, http.arg);
    ASSERT_EQ(0, http.headers.size());
}

TEST_F(HTTPFixture, initialState)
{
    ASSERT_STREQ("", http.cmdLine);
    ASSERT_EQ(NULL, http.arg);
    ASSERT_EQ(0, http.headers.size());
}
