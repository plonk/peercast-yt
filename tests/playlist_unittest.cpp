#include <gtest/gtest.h>

#include "playlist.h"
#include "sstream.h"

class PlayListFixture : public ::testing::Test {
public:
    PlayListFixture()
        : pls(PlayList::T_PLS, 1)
        , asx(PlayList::T_ASX, 1)
    {
    }

    PlayList pls;
    PlayList asx;
};

TEST_F(PlayListFixture, initialState)
{
    ASSERT_EQ(PlayList::T_PLS, pls.type);
    ASSERT_EQ(0, pls.numURLs);
    ASSERT_EQ(1, pls.maxURLs);
    ASSERT_NE(nullptr, pls.urls);
    ASSERT_NE(nullptr, pls.titles);

    ASSERT_EQ(PlayList::T_ASX, asx.type);
    ASSERT_EQ(0, asx.numURLs);
    ASSERT_EQ(1, asx.maxURLs);
    ASSERT_NE(nullptr, asx.urls);
    ASSERT_NE(nullptr, asx.titles);
}

TEST_F(PlayListFixture, addURL)
{
    ASSERT_EQ(0, pls.numURLs);

    pls.addURL("http://127.0.0.1:7144/stream/00000000000000000000000000000000.flv", "A ch");

    ASSERT_EQ(1, pls.numURLs);
    ASSERT_STREQ("http://127.0.0.1:7144/stream/00000000000000000000000000000000.flv", pls.urls[0].cstr());
    ASSERT_STREQ("A ch", pls.titles[0].cstr());

    // cannot add beyond maxURLs
    pls.addURL("http://127.0.0.1:7144/stream/FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF.flv", "B ch");

    ASSERT_EQ(1, pls.numURLs);
}

TEST_F(PlayListFixture, addChannel)
{
    ChanInfo info;
    info.name = "1ch";
    info.id.fromStr("01234567890123456789012345678901");
    info.contentType = ChanInfo::T_FLV;
    pls.addChannel("http://127.0.0.1:7144", info);
    ASSERT_EQ(1, pls.numURLs);
    ASSERT_STREQ("http://127.0.0.1:7144/stream/01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd", pls.urls[0].cstr());
    ASSERT_STREQ("1ch", pls.titles[0].cstr());

    info.contentType = ChanInfo::T_WMV;
    asx.addChannel("http://127.0.0.1:7144", info);
    ASSERT_STREQ("http://127.0.0.1:7144/stream/01234567890123456789012345678901.wmv?auth=44d5299e57ad9274fee7960a9fa60bfd", asx.urls[0].cstr());
}

TEST_F(PlayListFixture, write_pls)
{
    StringStream mem;

    ChanInfo info;
    info.name = "1ch";
    info.id.fromStr("01234567890123456789012345678901");
    info.contentType = ChanInfo::T_FLV;
    pls.addChannel("http://127.0.0.1:7144", info);

    pls.write(mem);
    ASSERT_STREQ("http://127.0.0.1:7144/stream/01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd\r\n",
                 mem.str().c_str());
}

TEST_F(PlayListFixture, write_asx)
{
    StringStream mem;

    ChanInfo info;
    info.name = "1ch";
    info.id.fromStr("01234567890123456789012345678901");
    info.contentType = ChanInfo::T_WMV;
    asx.addChannel("http://127.0.0.1:7144", info);

    asx.write(mem);
    ASSERT_STREQ("<ASX Version=\"3.0\">\r\n"
                 "<ENTRY>\r\n"
                 "<REF href=\"http://127.0.0.1:7144/stream/01234567890123456789012345678901.wmv?auth=44d5299e57ad9274fee7960a9fa60bfd\" />\r\n"
                 "</ENTRY>\r\n"
                 "</ASX>\r\n",
                 mem.str().c_str());
}

TEST_F(PlayListFixture, getPlayListType)
{
    ASSERT_EQ(PlayList::T_ASX, PlayList::getPlayListType(ChanInfo::T_WMA));
    ASSERT_EQ(PlayList::T_ASX, PlayList::getPlayListType(ChanInfo::T_WMV));
    ASSERT_EQ(PlayList::T_RAM, PlayList::getPlayListType(ChanInfo::T_OGM));
    ASSERT_EQ(PlayList::T_PLS, PlayList::getPlayListType(ChanInfo::T_OGG));
    ASSERT_EQ(PlayList::T_PLS, PlayList::getPlayListType(ChanInfo::T_MP3));
    ASSERT_EQ(PlayList::T_PLS, PlayList::getPlayListType(ChanInfo::T_FLV));
    ASSERT_EQ(PlayList::T_PLS, PlayList::getPlayListType(ChanInfo::T_MKV));
}
