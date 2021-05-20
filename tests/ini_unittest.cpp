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

TEST_F(IniFixture, valueEq)
{
    ASSERT_EQ(ini::Value(true), ini::Value(true));
    ASSERT_NE(ini::Value(true), ini::Value(false));
}

TEST_F(IniFixture, sectionEq)
{
    ASSERT_EQ(ini::Section("Heading", { { "key", true } }),
              ini::Section("Heading", { { "key", true } }));
    ASSERT_NE(ini::Section("Heading", { { "key", true } }),
              ini::Section("Heading", { { "key", false } }));
    ASSERT_NE(ini::Section("Heading", { { "key", true } }),
              ini::Section("Heading", { { "hoge", true } }));
    ASSERT_NE(ini::Section("Heading", { { "key", true } }),
              ini::Section("Hoge",    { { "key", true } }));
}

TEST_F(IniFixture, documentEq)
{
    ASSERT_EQ(ini::Document({ { "Heading", { { "key",  true } } } }),
              ini::Document({ { "Heading", { { "key",  true } } } }));
    ASSERT_NE(ini::Document({ { "Heading", { { "key",  true } } } }),
              ini::Document({}));
    ASSERT_NE(ini::Document({ { "Heading", { { "key",  true } } } }),
              ini::Document({ { "Heading", { { "key",  false } } } }));
}

TEST_F(IniFixture, parse)
{
    auto doc = ini::parse("\n[Server]\nserverPort = 7144\n\n[Filter]\nip = 255.255.255.255\nprivate = No\nban = No\nnetwork = Yes\ndirect = Yes\n[End]\n");

    ASSERT_EQ(2, doc.size());
    ASSERT_EQ("Server", doc[0].name);
    ASSERT_EQ("Filter", doc[1].name);

    ASSERT_EQ(doc,
              ini::Document(
              {
                  { "Server",
                      {
                          { "serverPort", 7144 }
                      }
                  },
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
              }));
}
