#include <gtest/gtest.h>

#include "template.h"
#include "sstream.h"

class TemplateFixture : public ::testing::Test {
public:
    TemplateFixture()
        : temp("")
    {
    }

    Template temp;
};

TEST_F(TemplateFixture, readVariable)
{
    StringStream in, out;

    in.writeString("servMgr.version}");
    in.rewind();
    temp.readVariable(in, &out, 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariable)
{
    StringStream out;

    temp.writeVariable(out, "servMgr.version", 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariableUndefined)
{
    StringStream out;

    temp.writeVariable(out, "hoge.fuga.piyo", 0);
    ASSERT_STREQ("hoge.fuga.piyo", out.str().c_str());
}

TEST_F(TemplateFixture, getIntVariable)
{
    ASSERT_EQ(0, temp.getIntVariable("servMgr.version", 0));
}

TEST_F(TemplateFixture, getBoolVariable)
{
    ASSERT_EQ(false, temp.getIntVariable("servMgr.version", 0));
}


TEST_F(TemplateFixture, readTemplate)
{
    StringStream in, out;
    bool res;

    in.writeString("hoge");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("hoge", out.str().c_str());
}

TEST_F(TemplateFixture, getStringVariable)
{
    ASSERT_STREQ("1", temp.getStringVariable("TRUE", 0).c_str());
    ASSERT_STREQ("0", temp.getStringVariable("FALSE", 0).c_str());
}

TEST_F(TemplateFixture, evalStringLiteral)
{
    ASSERT_STREQ("abc", temp.evalStringLiteral("\"abc\"").c_str());
    ASSERT_STREQ("", temp.evalStringLiteral("\"\"").c_str());
}

TEST_F(TemplateFixture, readStringLiteral)
{
    std::string lit, rest;
    std::tie(lit, rest) = temp.readStringLiteral("\"abc\"def");

    ASSERT_STREQ("\"abc\"", lit.c_str());
    ASSERT_STREQ("def", rest.c_str());
}

TEST_F(TemplateFixture, tokenizeBinaryExpression)
{
    auto tok = temp.tokenize("a==b");

    ASSERT_EQ(3, tok.size());
    ASSERT_STREQ("a", tok[0].c_str());
    ASSERT_STREQ("==", tok[1].c_str());
    ASSERT_STREQ("b", tok[2].c_str());
}

TEST_F(TemplateFixture, tokenizeVariableExpression)
{
    auto tok = temp.tokenize("a");

    ASSERT_EQ(1, tok.size());
    ASSERT_STREQ("a", tok[0].c_str());

    tok = temp.tokenize("!a");
    ASSERT_EQ(1, tok.size());
    ASSERT_STREQ("!a", tok[0].c_str());
}

TEST_F(TemplateFixture, evalCondition)
{
    ASSERT_TRUE(temp.evalCondition("TRUE", 0));
    ASSERT_FALSE(temp.evalCondition("FALSE", 0));
}

TEST_F(TemplateFixture, evalCondition2)
{
    ASSERT_TRUE(temp.evalCondition("TRUE==TRUE", 0));
    ASSERT_FALSE(temp.evalCondition("TRUE==FALSE", 0));
}

TEST_F(TemplateFixture, evalCondition3)
{
    ASSERT_FALSE(temp.evalCondition("TRUE!=TRUE", 0));
    ASSERT_TRUE(temp.evalCondition("TRUE!=FALSE", 0));
}

TEST_F(TemplateFixture, readIfTrue)
{
    StringStream in, out;

    in.writeString("TRUE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalse)
{
    StringStream in, out;

    in.writeString("FALSE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("F", out.str().c_str());
}

TEST_F(TemplateFixture, readIfTrueWithoutElse)
{
    StringStream in, out;

    in.writeString("TRUE}T{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalseWithoutElse)
{
    StringStream in, out;

    in.writeString("FALSE}T{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("", out.str().c_str());
}

TEST_F(TemplateFixture, fragment)
{
    StringStream in, out;
    bool res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("hogefugapiyo", out.str().c_str());
}

TEST_F(TemplateFixture, fragment2)
{
    StringStream in, out;
    bool res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("fuga", out.str().c_str());
}

TEST_F(TemplateFixture, fragment3)
{
    StringStream in, out;
    bool res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    temp.selectedFragment = "b";
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment)
{
    StringStream in, out;
    bool res;

    in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("10", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment2)
{
    StringStream in, out;
    bool res;

    in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("1", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment3)
{
    StringStream in, out;
    bool res;

    in.writeString("{@fragment a}{$TRUE}{@end}{$FALSE}");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("1", out.str().c_str());
}
