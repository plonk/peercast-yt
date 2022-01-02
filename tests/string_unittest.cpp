#include <gtest/gtest.h>
#include "sys.h"

TEST(StringTest, isEmptyWorks) {
  String str;
  ASSERT_TRUE( str.isEmpty() );
}

TEST(StringTest, setFromStopwatchWorks) {
    String str;
    str.setFromStopwatch(60 * 60 * 24);
    ASSERT_STREQ("1 day, 0 hour", str);

    str.setFromStopwatch(0);
    ASSERT_STREQ("-", str);
}

#include "regexp.h"
#include <time.h>
TEST(StringTest, setFromTime) {
    ASSERT_TRUE(Regexp("^(\\w+) (\\w+) +(\\d+) (\\d+):(\\d+):(\\d+) (\\d+)\n$").matches(String().setFromTime(0)));
    time_t now = ::time(nullptr);
    ASSERT_TRUE(Regexp("^(\\w+) (\\w+) +(\\d+) (\\d+):(\\d+):(\\d+) (\\d+)\n$").matches(String().setFromTime(now)));
}

TEST(StringTest, eqWorks) {
    String s1("abc");
    String s2("abc");
    String s3("ABC");
    String s4("xyz");

    ASSERT_EQ(s1, s2);
    ASSERT_NE(s1, s3);
    ASSERT_NE(s1, s4);
}

TEST(StringTest, containsWorks) {
    String abc("abc");
    String ABC("ABC");
    String xyz("xyz");
    String a("a");

    ASSERT_TRUE(abc.contains(ABC));
    ASSERT_FALSE(abc.contains(xyz));
    ASSERT_FALSE(ABC.contains(xyz));

    ASSERT_TRUE(abc.contains(a));
    ASSERT_FALSE(a.contains(abc));
}

TEST(StringTest, appendChar) {
    String buf;
    for (int i = 0; i < 500; i++) {
        buf.append('A');
    }

    // バッファーは 256 バイトあるので、NUL の分を考慮しても 255 文字
    // まで入りそうなものだが、入るのは 254 文字まで。
    ASSERT_EQ(254, strlen(buf.data));
}

TEST(StringTest, appendString) {
    String s = "a";

    s.append("bc");

    ASSERT_STREQ("abc", s.cstr());
}

TEST(StringTest, prependWorks) {
    String buf = " world!";
    buf.prepend("Hello");
    ASSERT_STREQ("Hello world!", buf);

    String buf2 = " world!AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    buf2.prepend("Hello");
    ASSERT_STREQ("Hello", buf2);
}

TEST(StringTest, isWhitespaceWorks) {
    ASSERT_TRUE(String::isWhitespace(' '));
    ASSERT_TRUE(String::isWhitespace('\t'));
    ASSERT_FALSE(String::isWhitespace('A'));
    ASSERT_FALSE(String::isWhitespace('-'));
    ASSERT_FALSE(String::isWhitespace('\r'));
    ASSERT_FALSE(String::isWhitespace('\n'));
}

TEST(StringTest, ASCII2HTMLWorks) {
    String str;

    str.ASCII2HTML("AAA");
    ASSERT_STREQ("AAA", str);

    // 英数字以外は16進の文字実体参照としてエスケープされる。
    str.ASCII2HTML("   ");
    ASSERT_STREQ("&#x20;&#x20;&#x20;", str);

    str.ASCII2HTML("&\"<>");
    ASSERT_STREQ("&#x26;&#x22;&#x3C;&#x3E;", str);
}

// TEST(StringTest, ASCII2HTMLWorks) {
//     String str;

//     str.ASCII2HTML("AAA");
//     ASSERT_STREQ("AAA", str);

//     str.ASCII2HTML("   ");
//     ASSERT_STREQ("&#x20;&#x20;&#x20;", str);

//     String str2("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"); // 255
//     str.ASCII2HTML(str2);
//     ASSERT_EQ(246, strlen(str.data));

//     str.ASCII2HTML("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA ");
//     ASSERT_EQ(251, strlen(str.data));
// }

TEST(StringTest, HTML2ASCIIWorks) {
    String str;

    str.HTML2ASCII("&#x21;");
    ASSERT_STREQ("!", str);

    str.HTML2ASCII("A");
    ASSERT_STREQ("A", str);

    str.HTML2ASCII("&amp;");
    ASSERT_STREQ("&amp;", str);
}

TEST(StringTest, setFromStopwatch)
{
    String s;

    ASSERT_STREQ("1 day, 0 hour", (s.setFromStopwatch(86400), s).cstr());
    ASSERT_STREQ("1 hour, 0 min", (s.setFromStopwatch(3600), s).cstr());
    ASSERT_STREQ("1 min, 0 sec", (s.setFromStopwatch(60), s).cstr());
    ASSERT_STREQ("1 sec", (s.setFromStopwatch(1), s).cstr());
    ASSERT_STREQ("-", (s.setFromStopwatch(0), s).cstr());
}

TEST(StringTest, assignment)
{
    String s, t;

    s = "hoge";
    t = s;
    ASSERT_STREQ("hoge", t.cstr());
}

TEST(StringTest, sjisToUtf8)
{
    String tmp = "4\x93\xFA\x96\xDA"; // "4日目" in Shit_JIS
    tmp.convertTo(String::T_UNICODESAFE);
    ASSERT_STREQ("4日目", tmp.cstr());
}

TEST(StringTest, setUnquote)
{
    String s = "xyz";

    s.setUnquote("\"abc\"");
    ASSERT_STREQ("abc", s.cstr());

    // 二文字に満たない場合は空になる。
    s.setUnquote("a");
    ASSERT_STREQ("", s.cstr());
}

TEST(StringTest, clear)
{
    String s = "abc";

    ASSERT_STREQ("abc", s.cstr());
    s.clear();

    ASSERT_STREQ("", s.cstr());
}

TEST(StringTest, equalOperatorCString)
{
    String s;
    const char* xs = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // 300 x's

    s = xs;
    ASSERT_STRNE(xs, s.cstr());
    ASSERT_EQ(255, strlen(s.cstr()));
}

TEST(StringTest, equalOperatorStdString)
{
    String s;
    const char* xs = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // 300 x's

    s = std::string(xs);
    ASSERT_STRNE(xs, s.cstr());
    ASSERT_EQ(255, strlen(s.cstr()));
}

TEST(StringTest, cstr)
{
    String s = "abc";

    ASSERT_STREQ("abc", s.cstr());
    ASSERT_EQ(s.data, s.cstr());
}

TEST(StringTest, c_str)
{
    String s = "abc";

    ASSERT_STREQ("abc", s.c_str());
    ASSERT_EQ(s.data, s.c_str());
}

TEST(StringTest, str)
{
    ASSERT_STREQ("", String("").str().c_str());
    ASSERT_STREQ("A", String("A").str().c_str());

    String s = "abc";

    ASSERT_STREQ("abc", s.str().c_str());
    ASSERT_NE(s.data, s.str().c_str());
}

TEST(StringTest, size)
{
    ASSERT_EQ(0, String("").size());
    ASSERT_EQ(3, String("abc").size());
}

TEST(StringTest, setFromString)
{
    String s;

    s.setFromString("abc def");
    ASSERT_STREQ("abc", s.data);

    s.setFromString("\"abc def\"");
    ASSERT_STREQ("abc def", s.data);
}
