#include <gtest/gtest.h>

#include "template.h"
#include "dmstream.h"

class TemplateFixture : public ::testing::Test {
public:
    TemplateFixture()
        : temp("")
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~TemplateFixture()
    {
    }

    Template temp;
};

TEST_F(TemplateFixture, readVariable)
{
    DynamicMemoryStream in, out;

    in.writeString("servMgr.version}");
    in.rewind();
    temp.readVariable(in, &out, 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariable)
{
    DynamicMemoryStream out;

    temp.writeVariable(out, "servMgr.version", 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariableNonExistentVariable)
{
    DynamicMemoryStream out;

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
    DynamicMemoryStream in, out;
    bool res;

    in.writeString("hoge");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("hoge", out.str().c_str());
}

TEST_F(TemplateFixture, readIfTrue)
{
    DynamicMemoryStream in, out;

    in.writeString("TRUE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalse)
{
    DynamicMemoryStream in, out;

    in.writeString("FALSE}T{@else}F{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("F", out.str().c_str());
}

TEST_F(TemplateFixture, readIfTrueWithoutElse)
{
    DynamicMemoryStream in, out;

    in.writeString("TRUE}T{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("T", out.str().c_str());
}

TEST_F(TemplateFixture, readIfFalseWithoutElse)
{
    DynamicMemoryStream in, out;

    in.writeString("FALSE}T{@end}");
    in.rewind();
    temp.readIf(in, &out, 0);
    ASSERT_STREQ("", out.str().c_str());
}

TEST_F(TemplateFixture, fragment)
{
    DynamicMemoryStream in, out;
    bool res;

    in.writeString("hoge{@fragment a}fuga{@end}piyo");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("hogefugapiyo", out.str().c_str());
}

TEST_F(TemplateFixture, fragment2)
{
    DynamicMemoryStream in, out;
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
    DynamicMemoryStream in, out;
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
    DynamicMemoryStream in, out;
    bool res;

    in.writeString("{@fragment a}{$TRUE}{@end}{@fragment b}{$FALSE}{@end}");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("10", out.str().c_str());
}

TEST_F(TemplateFixture, variableInFragment2)
{
    DynamicMemoryStream in, out;
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
    DynamicMemoryStream in, out;
    bool res;

    in.writeString("{@fragment a}{$TRUE}{@end}{$FALSE}");
    in.rewind();
    temp.selectedFragment = "a";
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("1", out.str().c_str());
}
