#include <gtest/gtest.h>

#include "ini.h"

class IniFixture : public ::testing::Test {
public:
    IniFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~IniFixture()
    {
    }
};

TEST_F(IniFixture, createDocument)
{
    ini::Document doc
    {
        { "Server",
                {
                    { "serverPort", 7144 }
                }
        },
            // [Filter]
            // ip = 255.255.255.255
            // private = No
            // ban = No
            // network = Yes
            // direct = Yes
            // [End]
        { "Filter",
                {
                    { "ip", "255.255.255.255" },
                    { "private", false },
                    { "ban", false },
                    { "network", true },
                    { "direct", true },
                },
          "End"
        }
    };
    EXPECT_EQ(2, doc.size());
    ASSERT_EQ("\n[Server]\nserverPort = 7144\n\n[Filter]\nip = 255.255.255.255\nprivate = No\nban = No\nnetwork = Yes\ndirect = Yes\n[End]\n", ini::dump(doc));
}
