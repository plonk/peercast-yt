#include <gtest/gtest.h>

#include "cgi.h"

using namespace cgi;

class cgiFixture : public ::testing::Test {
public:
    void SetUp()
    {
        for (auto i = 0; i < 256; i++) {
            all_bytes.push_back(i);
        }
    }
    std::string all_bytes;
};

TEST_F(cgiFixture, escape)
{
    ASSERT_STREQ("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F+%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7E%7F%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF", cgi::escape(all_bytes).c_str());
}

TEST_F(cgiFixture, unescape)
{
    // Ruby CGI.escape でエスケープしたものを復元。
    EXPECT_EQ(all_bytes, cgi::unescape("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F+%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7E%7F%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF"));
    // A-F に小文字を使う。
    ASSERT_EQ(all_bytes, cgi::unescape("%00%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%1f+%21%22%23%24%25%26%27%28%29%2a%2b%2c-.%2f0123456789%3a%3b%3c%3d%3e%3f%40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5b%5c%5d%5e_%60abcdefghijklmnopqrstuvwxyz%7b%7c%7d%7e%7f%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f%90%91%92%93%94%95%96%97%98%99%9a%9b%9c%9d%9e%9f%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%ba%bb%bc%bd%be%bf%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf%d0%d1%d2%d3%d4%d5%d6%d7%d8%d9%da%db%dc%dd%de%df%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef%f0%f1%f2%f3%f4%f5%f6%f7%f8%f9%fa%fb%fc%fd%fe%ff"));
    // すべての文字を16進エスケープ。
    ASSERT_EQ(all_bytes, cgi::unescape("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F%20%21%22%23%24%25%26%27%28%29%2A%2B%2C%2D%2E%2F%30%31%32%33%34%35%36%37%38%39%3A%3B%3C%3D%3E%3F%40%41%42%43%44%45%46%47%48%49%4A%4B%4C%4D%4E%4F%50%51%52%53%54%55%56%57%58%59%5A%5B%5C%5D%5E%5F%60%61%62%63%64%65%66%67%68%69%6A%6B%6C%6D%6E%6F%70%71%72%73%74%75%76%77%78%79%7A%7B%7C%7D%7E%7F%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF"));
}

TEST_F(cgiFixture, rfc1123Time)
{
    ASSERT_STREQ("Thu, 01 Jan 1970 00:00:00 GMT", cgi::rfc1123Time(0).c_str());
}

TEST_F(cgiFixture, escape_html)
{
    ASSERT_STREQ("", escape_html("").c_str());
    ASSERT_STREQ("a", escape_html("a").c_str());
    ASSERT_STREQ("aa", escape_html("aa").c_str());
    ASSERT_STREQ("&lt;&amp;&gt;&#39;&quot;", escape_html("<&>\'\"").c_str());
}

TEST_F(cgiFixture, unescape_html)
{
    ASSERT_STREQ("", unescape_html("").c_str());
    ASSERT_STREQ("a", unescape_html("a").c_str());
    ASSERT_STREQ("aa", unescape_html("aa").c_str());
    ASSERT_STREQ("<&>", unescape_html("&lt;&amp;&gt;").c_str());
    ASSERT_STREQ("<&>", unescape_html("&lt;&&gt;").c_str());
    ASSERT_STREQ("<a&b>", unescape_html("&lt;a&b&gt;").c_str());
    ASSERT_STREQ("&gt", unescape_html("&gt").c_str());
    ASSERT_STREQ("&YY;", unescape_html("&YY;").c_str());
    ASSERT_STREQ("あ", unescape_html("&#12354;").c_str());
    ASSERT_STREQ("\xf0\x9f\x92\xa9", unescape_html("&#x1f4a9;").c_str());
    ASSERT_STREQ("\xf0\x9f\x92\xa9", unescape_html("&#X1F4A9;").c_str());
}

TEST_F(cgiFixture, unescape_javascript)
{
    ASSERT_STREQ("", escape_javascript("").c_str());
    ASSERT_STREQ("a", escape_javascript("a").c_str());
    ASSERT_STREQ("aa", escape_javascript("aa").c_str());
    ASSERT_STREQ("\\\'\\\"\\\\", escape_javascript("\'\"\\").c_str());
    ASSERT_STREQ("あ", escape_javascript("あ").c_str());
    ASSERT_STREQ("\\x0D\\x0A", escape_javascript("\r\n").c_str());
}
