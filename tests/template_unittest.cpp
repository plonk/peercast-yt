#include <gtest/gtest.h>

#include "template.h"
#include "sstream.h"

#include "str.h"
#include "servmgr.h"

class TemplateFixture : public ::testing::Test {
public:
    TemplateFixture()
        : temp("")
    {
        locals.vars["servMgr"] = { { "version", "v0.1218" } };
        temp.prependScope(locals);
    }

    GenericScope locals;
    Template temp;
};

TEST_F(TemplateFixture, readVariable)
{
    StringStream in, out;

    in.writeString("servMgr.version}");
    in.rewind();
    temp.readVariable(in, &out);
    ASSERT_TRUE(str::has_prefix(out.str(), "v0.1218"));
}

TEST_F(TemplateFixture, writeVariable)
{
    amf0::Value out;
    temp.writeVariable(out, "servMgr.version");
    ASSERT_TRUE(str::has_prefix(out.string(), "v0.1218"));
}

TEST_F(TemplateFixture, writeVariableUndefined)
{
    amf0::Value out;
    temp.writeVariable(out, "hoge.fuga.piyo");
    ASSERT_EQ(amf0::Value(nullptr), out);
}

TEST_F(TemplateFixture, getIntVariable)
{
    ASSERT_EQ(0, temp.getIntVariable("servMgr.version"));
}

TEST_F(TemplateFixture, getBoolVariable)
{
    ASSERT_EQ(false, temp.getIntVariable("servMgr.version"));
}


TEST_F(TemplateFixture, readTemplate)
{
    StringStream in, out;
    int res;

    in.writeString("hoge");
    in.rewind();
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("hoge", out.str().c_str());
}

TEST_F(TemplateFixture, getStringVariable)
{
    ASSERT_STREQ("1", temp.getStringVariable("TRUE").c_str());
    ASSERT_STREQ("0", temp.getStringVariable("FALSE").c_str());
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

static std::vector<std::string> toVector(const std::list<std::string>& ls)
{
    return { ls.begin(), ls.end() };
}

TEST_F(TemplateFixture, tokenizeBinaryExpression)
{
    std::vector<std::string> tok = toVector(temp.tokenize("a==b"));

    ASSERT_EQ(3, tok.size());
    ASSERT_STREQ("a", tok[0].c_str());
    ASSERT_STREQ("==", tok[1].c_str());
    ASSERT_STREQ("b", tok[2].c_str());
}

TEST_F(TemplateFixture, tokenizeVariableExpression)
{
    std::vector<std::string> tok = toVector(temp.tokenize("a"));

    ASSERT_EQ(1, tok.size());
    ASSERT_STREQ("a", tok[0].c_str());

    tok = toVector(temp.tokenize("!a"));
    ASSERT_EQ(2, tok.size());
    ASSERT_STREQ("!", tok[0].c_str());
    ASSERT_STREQ("a", tok[1].c_str());
}

TEST_F(TemplateFixture, tokenizeCallExpression)
{
    std::vector<std::string> tok = toVector(temp.tokenize("inspect(x)"));

    ASSERT_EQ(4, tok.size());
    ASSERT_EQ(tok[0], "inspect");
    ASSERT_EQ(tok[1], "(");
    ASSERT_EQ(tok[2], "x");
    ASSERT_EQ(tok[3], ")");
}

TEST_F(TemplateFixture, evalCondition)
{
    ASSERT_TRUE(temp.evalCondition("TRUE"));
    ASSERT_FALSE(temp.evalCondition("FALSE"));
}

TEST_F(TemplateFixture, evalCondition2)
{
    ASSERT_TRUE(temp.evalCondition("TRUE==TRUE"));
    ASSERT_FALSE(temp.evalCondition("TRUE==FALSE"));
}

TEST_F(TemplateFixture, evalCondition3)
{
    ASSERT_FALSE(temp.evalCondition("TRUE!=TRUE"));
    ASSERT_TRUE(temp.evalCondition("TRUE!=FALSE"));
}

TEST_F(TemplateFixture, stringBinaryCondition)
{
    ASSERT_TRUE(temp.evalCondition("\"A\"==\"A\""));
    ASSERT_FALSE(temp.evalCondition("\"A\"==\"B\""));
}

TEST_F(TemplateFixture, regexpBinaryCondition)
{
    ASSERT_TRUE(temp.evalCondition("\"A\"=~\"A\""));
    ASSERT_FALSE(temp.evalCondition("\"A\"=~\"B\""));

    ASSERT_FALSE(temp.evalCondition("\"A\"!~\"A\""));
    ASSERT_TRUE(temp.evalCondition("\"A\"!~\"B\""));

    ASSERT_TRUE(temp.evalCondition("\"ABC\"=~\"^A\""));
    ASSERT_TRUE(temp.evalCondition("\"ABC\"=~\"C$\""));
}

TEST_F(TemplateFixture, readIfTrue)
{
    StringStream in, out;

    in.writeString("TRUE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalse)
{
    StringStream in, out;

    in.writeString("FALSE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out);
    ASSERT_STREQ("F", out.str().c_str());
}

TEST_F(TemplateFixture, readIfTrueWithoutElse)
{
    StringStream in, out;

    in.writeString("TRUE}T{@end}");
    in.rewind();
    temp.readIf(in, &out);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalseWithoutElse)
{
    StringStream in, out;

    in.writeString("FALSE}T{@end}");
    in.rewind();
    temp.readIf(in, &out);
    ASSERT_STREQ("", out.str().c_str());
}

TEST_F(TemplateFixture, fragment)
{
    StringStream in, out;
    int res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("hogefugapiyo", out.str().c_str());
}

TEST_F(TemplateFixture, fragment2)
{
    StringStream in, out;
    int res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("fuga", out.str().c_str());
}

TEST_F(TemplateFixture, fragment3)
{
    StringStream in, out;
    int res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    temp.selectedFragment = "b";
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment)
{
    StringStream in, out;
    int res;

    in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
    in.rewind();
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("10", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment2)
{
    StringStream in, out;
    int res;

    in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("1", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment3)
{
    StringStream in, out;
    int res;

    in.writeString("{@fragment a}{$TRUE}{@end}{$FALSE}");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_STREQ("1", out.str().c_str());
}

TEST_F(TemplateFixture, elsif)
{
    StringStream in, out;
    int res;

    in.str("{@if TRUE}A{@elsif TRUE}B{@else}C{@end}");
    out.str("");
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_EQ("A", out.str());

    in.str("{@if TRUE}A{@elsif FALSE}B{@else}C{@end}");
    out.str("");
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_EQ("A", out.str());

    in.str("{@if FALSE}A{@elsif TRUE}B{@else}C{@end}");
    out.str("");
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_EQ("B", out.str());

    in.str("{@if FALSE}A{@elsif FALSE}B{@else}C{@end}");
    out.str("");
    res = temp.readTemplate(in, &out);
    ASSERT_EQ(Template::TMPL_END, res);
    ASSERT_EQ("C", out.str());
}

TEST_F(TemplateFixture, parse)
{
    auto tok = temp.tokenize("inspect(x)");
    auto val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value::strictArray({"inspect", "x"}).inspect());

    tok = temp.tokenize("!x.y");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value::strictArray({"!", "x.y"}).inspect());

    tok = temp.tokenize("x.y");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value("x.y").inspect());

    tok = temp.tokenize("a == b");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value::strictArray({"==", "a", "b"}).inspect());

    tok = temp.tokenize("f(a) != g(b)");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value::strictArray({"!=", amf0::Value::strictArray({ "f", "a" }), amf0::Value::strictArray({ "g", "b" }) }).inspect());

    tok = temp.tokenize("\"a\"");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value::strictArray({"quote", "a"}).inspect());

    tok = temp.tokenize("1");
    val = Template::parse(tok);
    ASSERT_EQ(val.inspect(), amf0::Value("1").inspect());
}
