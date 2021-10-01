#include <gtest/gtest.h>

#include "http.h"

class HTTPHeadersFixture : public ::testing::Test {
public:
    HTTPHeadersFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~HTTPHeadersFixture()
    {
    }

    HTTPHeaders headers;
};

TEST_F(HTTPHeadersFixture, set)
{
    headers.set("Content-Length", "123");
    ASSERT_EQ("123", headers.m_headers["CONTENT-LENGTH"]);
}

TEST_F(HTTPHeadersFixture, get)
{
    headers.set("Content-Length", "123");
    ASSERT_EQ("123", headers.get("Content-Length"));
    ASSERT_EQ("123", headers.get("CONTENT-LENGTH"));
    ASSERT_EQ("123", headers.get("content-length"));
}

TEST_F(HTTPHeadersFixture, copyConstruct)
{
    headers.set("a", "b");
    HTTPHeaders copy(headers);
    ASSERT_EQ("b", copy.get("A"));
}

TEST_F(HTTPHeadersFixture, assignment)
{
    headers.set("a", "b");
    HTTPHeaders copy;
    ASSERT_EQ("", copy.get("A"));
    copy = headers;
    ASSERT_EQ("b", copy.get("A"));
}

TEST_F(HTTPHeadersFixture, hasKeyWithValue_basic)
{
    headers.set("connection", "keep-alive");
    ASSERT_TRUE(headers.hasKeyWithValue("connection", "keep-alive"));
}

TEST_F(HTTPHeadersFixture, hasKeyWithValue_caseSensitivity)
{
    headers.set("Connection", "Keep-Alive");
    ASSERT_TRUE(headers.hasKeyWithValue("CONNECTION", "KEEP-ALIVE"));
}

TEST_F(HTTPHeadersFixture, hasKeyWithValue_commaValues)
{
    headers.set("connection", "keep-alive, Upgrade");
    ASSERT_TRUE(headers.hasKeyWithValue("connection", "Upgrade"));
}
