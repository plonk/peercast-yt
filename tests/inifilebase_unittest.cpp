#include <gtest/gtest.h>

#include "inifile.h"
#include "sstream.h"

class IniFileBaseFixture : public ::testing::Test {
public:
    IniFileBaseFixture()
       : ini(mem)
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~IniFileBaseFixture()
    {
    }

    StringStream mem;
    IniFileBase ini;
};

TEST_F(IniFileBaseFixture, initialState)
{
    ASSERT_STREQ("", ini.currLine);
    ASSERT_EQ(nullptr, ini.nameStr);
    ASSERT_EQ(nullptr, ini.valueStr);
}

TEST_F(IniFileBaseFixture, readNext_eof)
{
    mem.str("");
    ASSERT_FALSE(ini.readNext());
}

TEST_F(IniFileBaseFixture, emptyLine)
{
    mem.str("\n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_STREQ("", ini.getName());
    ASSERT_STREQ("", ini.getStrValue());
}

TEST_F(IniFileBaseFixture, readNext_assignment)
{
    mem.str("x = 1\n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_STREQ("x", ini.getName());
    ASSERT_STREQ("1", ini.getStrValue());
    ASSERT_EQ(1, ini.getIntValue());
    ASSERT_EQ(true, ini.getBoolValue());
}

TEST_F(IniFileBaseFixture, namesAndValuesAreTrimmed)
{
    mem.str("\ty   =    2   \n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_STREQ("y", ini.getName());
    ASSERT_STREQ("2", ini.getStrValue());
}

TEST_F(IniFileBaseFixture, getBoolValue)
{
    mem.str("x = 0\n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_EQ(false, ini.getBoolValue());
}

TEST_F(IniFileBaseFixture, readNext_assignmentWithoutValue)
{
    mem.str("x = \n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_STREQ("x", ini.getName());
    ASSERT_STREQ("", ini.getStrValue());
    ASSERT_EQ(0, ini.getIntValue());
    ASSERT_EQ(false, ini.getBoolValue());
}

TEST_F(IniFileBaseFixture, readNext_sectionHeader)
{
    mem.str("[Section]\n");
    ASSERT_TRUE(ini.readNext());
    ASSERT_STREQ("[Section]", ini.getName());
    ASSERT_STREQ("", ini.getStrValue());
}

// writing


TEST_F(IniFileBaseFixture, writeSection)
{
    ini.writeSection("Section");
    ASSERT_EQ("\r\n[Section]\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, writeIntValue)
{
    ini.writeIntValue("x", 1);
    ASSERT_EQ("x = 1\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, writeStrValue)
{
    ini.writeStrValue("y", "foo bar baz");
    ASSERT_EQ("y = foo bar baz\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, letsTryToSmashStack)
{
    ini.writeStrValue("veryLongString", "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    ASSERT_EQ("veryLongString = AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, writeBoolValue_true)
{
    ini.writeBoolValue("z", 1);
    ASSERT_EQ("z = Yes\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, writeBoolValue_false)
{
    ini.writeBoolValue("z", 0);
    ASSERT_EQ("z = No\r\n", mem.str());
}

TEST_F(IniFileBaseFixture, writeLine)
{
    ini.writeLine("[End]");
    ASSERT_EQ("[End]\r\n", mem.str());
}


