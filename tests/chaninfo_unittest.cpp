#include <gtest/gtest.h>

#include "channel.h"

class ChanInfoFixture : public ::testing::Test {
public:
    ChanInfoFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChanInfoFixture()
    {
    }

    ChanInfo info;
};

TEST_F(ChanInfoFixture, initialState)
{
    ASSERT_STREQ("", info.name.cstr());
    ASSERT_STREQ("00000000000000000000000000000000", static_cast<std::string>(info.id).c_str());
    ASSERT_STREQ("00000000000000000000000000000000", static_cast<std::string>(info.bcID).c_str());
    ASSERT_EQ(0, info.bitrate);
    ASSERT_EQ(ChanInfo::T_UNKNOWN, info.contentType);
    ASSERT_STREQ("", info.contentTypeStr.cstr());
    ASSERT_STREQ("", info.MIMEType.cstr());
    ASSERT_STREQ("", info.streamExt.cstr());
    ASSERT_EQ(ChanInfo::PROTOCOL::SP_UNKNOWN, info.srcProtocol);
    ASSERT_EQ(0, info.lastPlayStart);
    ASSERT_EQ(0, info.lastPlayEnd);
    ASSERT_EQ(0, info.numSkips);
    ASSERT_EQ(0, info.createdTime);
    ASSERT_EQ(ChanInfo::S_UNKNOWN, info.status);

    {
        ASSERT_STREQ("", info.track.contact.cstr());
        ASSERT_STREQ("", info.track.title.cstr());
        ASSERT_STREQ("", info.track.artist.cstr());
        ASSERT_STREQ("", info.track.album.cstr());
        ASSERT_STREQ("", info.track.genre.cstr());
    }

    ASSERT_STREQ("", info.desc);
    ASSERT_STREQ("", info.genre);
    ASSERT_STREQ("", info.url);
    ASSERT_STREQ("", info.comment);
}

TEST_F(ChanInfoFixture, writeInfoAtoms)
{
    MemoryStream mem(1024);
    AtomStream atom(mem);

    info.writeInfoAtoms(atom);

    ASSERT_EQ(81, mem.getPosition());
}

TEST_F(ChanInfoFixture, writeTrackAtoms)
{
    MemoryStream mem(1024);
    AtomStream atom(mem);

    info.writeTrackAtoms(atom);

    ASSERT_EQ(44, mem.getPosition());
}

TEST_F(ChanInfoFixture, getters)
{
    ASSERT_EQ(0, info.getUptime());
    ASSERT_EQ(0, info.getAge());
    ASSERT_EQ(false, info.isActive());
    ASSERT_EQ(false, info.isPrivate());
    ASSERT_STREQ("UNKNOWN", info.getTypeStr());
    ASSERT_STREQ("", info.getTypeExt());
    ASSERT_STREQ("application/octet-stream", info.getMIMEType());
}

TEST_F(ChanInfoFixture, static_getTypeStr)
{
    ASSERT_STREQ("RAW", ChanInfo::getTypeStr(ChanInfo::T_RAW));
    ASSERT_STREQ("MP3", ChanInfo::getTypeStr(ChanInfo::T_MP3));
    ASSERT_STREQ("OGG", ChanInfo::getTypeStr(ChanInfo::T_OGG));
    ASSERT_STREQ("OGM", ChanInfo::getTypeStr(ChanInfo::T_OGM));
    ASSERT_STREQ("WMA", ChanInfo::getTypeStr(ChanInfo::T_WMA));
    ASSERT_STREQ("MOV", ChanInfo::getTypeStr(ChanInfo::T_MOV));
    ASSERT_STREQ("MPG", ChanInfo::getTypeStr(ChanInfo::T_MPG));
    ASSERT_STREQ("NSV", ChanInfo::getTypeStr(ChanInfo::T_NSV));
    ASSERT_STREQ("WMV", ChanInfo::getTypeStr(ChanInfo::T_WMV));
    ASSERT_STREQ("FLV", ChanInfo::getTypeStr(ChanInfo::T_FLV));
    ASSERT_STREQ("PLS", ChanInfo::getTypeStr(ChanInfo::T_PLS));
    ASSERT_STREQ("ASX", ChanInfo::getTypeStr(ChanInfo::T_ASX));
    ASSERT_STREQ("UNKNOWN", ChanInfo::getTypeStr(ChanInfo::T_UNKNOWN));
}

TEST_F(ChanInfoFixture, static_getProtocolStr)
{
    ASSERT_STREQ("PEERCAST", ChanInfo::getProtocolStr(ChanInfo::SP_PEERCAST));
    ASSERT_STREQ("HTTP", ChanInfo::getProtocolStr(ChanInfo::SP_HTTP));
    ASSERT_STREQ("FILE", ChanInfo::getProtocolStr(ChanInfo::SP_FILE));
    ASSERT_STREQ("MMS", ChanInfo::getProtocolStr(ChanInfo::SP_MMS));
    ASSERT_STREQ("PCP", ChanInfo::getProtocolStr(ChanInfo::SP_PCP));
    ASSERT_STREQ("WMHTTP", ChanInfo::getProtocolStr(ChanInfo::SP_WMHTTP));
    ASSERT_STREQ("UNKNOWN", ChanInfo::getProtocolStr(ChanInfo::SP_UNKNOWN));
}

TEST_F(ChanInfoFixture, static_getTypeExt)
{
    ASSERT_STREQ(".ogg", ChanInfo::getTypeExt(ChanInfo::T_OGM));
    ASSERT_STREQ(".ogg", ChanInfo::getTypeExt(ChanInfo::T_OGG));
    ASSERT_STREQ(".mp3", ChanInfo::getTypeExt(ChanInfo::T_MP3));
    ASSERT_STREQ(".mov", ChanInfo::getTypeExt(ChanInfo::T_MOV));
    ASSERT_STREQ(".nsv", ChanInfo::getTypeExt(ChanInfo::T_NSV));
    ASSERT_STREQ(".wmv", ChanInfo::getTypeExt(ChanInfo::T_WMV));
    ASSERT_STREQ(".wma", ChanInfo::getTypeExt(ChanInfo::T_WMA));
    ASSERT_STREQ(".flv", ChanInfo::getTypeExt(ChanInfo::T_FLV));
    ASSERT_STREQ("", ChanInfo::getTypeExt(ChanInfo::T_UNKNOWN));
}

TEST_F(ChanInfoFixture, static_getMIMEStr)
{
    ASSERT_STREQ("application/x-ogg", ChanInfo::getMIMEType(ChanInfo::T_OGG));
    ASSERT_STREQ("application/x-ogg", ChanInfo::getMIMEType(ChanInfo::T_OGM));
    ASSERT_STREQ("audio/mpeg", ChanInfo::getMIMEType(ChanInfo::T_MP3));
    ASSERT_STREQ("video/quicktime", ChanInfo::getMIMEType(ChanInfo::T_MOV));
    ASSERT_STREQ("video/mpeg", ChanInfo::getMIMEType(ChanInfo::T_MPG));
    ASSERT_STREQ("video/nsv", ChanInfo::getMIMEType(ChanInfo::T_NSV));
    ASSERT_STREQ("video/x-ms-asf", ChanInfo::getMIMEType(ChanInfo::T_ASX));
    ASSERT_STREQ("audio/x-ms-wma", ChanInfo::getMIMEType(ChanInfo::T_WMA));
    ASSERT_STREQ("video/x-ms-wmv", ChanInfo::getMIMEType(ChanInfo::T_WMV));
    ASSERT_STREQ("video/x-flv", ChanInfo::getMIMEType(ChanInfo::T_FLV));
}

TEST_F(ChanInfoFixture, static_getTypeFromStr)
{
    ASSERT_EQ(ChanInfo::T_MP3, ChanInfo::getTypeFromStr("MP3"));
    ASSERT_EQ(ChanInfo::T_OGG, ChanInfo::getTypeFromStr("OGG"));
    ASSERT_EQ(ChanInfo::T_OGM, ChanInfo::getTypeFromStr("OGM"));
    ASSERT_EQ(ChanInfo::T_RAW, ChanInfo::getTypeFromStr("RAW"));
    ASSERT_EQ(ChanInfo::T_NSV, ChanInfo::getTypeFromStr("NSV"));
    ASSERT_EQ(ChanInfo::T_WMA, ChanInfo::getTypeFromStr("WMA"));
    ASSERT_EQ(ChanInfo::T_WMV, ChanInfo::getTypeFromStr("WMV"));
    ASSERT_EQ(ChanInfo::T_FLV, ChanInfo::getTypeFromStr("FLV"));
    ASSERT_EQ(ChanInfo::T_PLS, ChanInfo::getTypeFromStr("PLS"));
    ASSERT_EQ(ChanInfo::T_PLS, ChanInfo::getTypeFromStr("M3U"));
    ASSERT_EQ(ChanInfo::T_ASX, ChanInfo::getTypeFromStr("ASX"));

    ASSERT_EQ(ChanInfo::T_MP3, ChanInfo::getTypeFromStr("mp3")); // type str. is case-insensitive
    ASSERT_EQ(ChanInfo::T_UNKNOWN, ChanInfo::getTypeFromStr("mp345"));
}

TEST_F(ChanInfoFixture, static_getProtocolFromStr)
{
    ASSERT_EQ(ChanInfo::SP_PEERCAST, ChanInfo::getProtocolFromStr("PEERCAST"));
    ASSERT_EQ(ChanInfo::SP_HTTP, ChanInfo::getProtocolFromStr("HTTP"));
    ASSERT_EQ(ChanInfo::SP_FILE, ChanInfo::getProtocolFromStr("FILE"));
    ASSERT_EQ(ChanInfo::SP_MMS, ChanInfo::getProtocolFromStr("MMS"));
    ASSERT_EQ(ChanInfo::SP_PCP, ChanInfo::getProtocolFromStr("PCP"));
    ASSERT_EQ(ChanInfo::SP_WMHTTP, ChanInfo::getProtocolFromStr("WMHTTP"));

    ASSERT_EQ(ChanInfo::SP_PEERCAST, ChanInfo::getProtocolFromStr("Peercast")); // type str. is case-insesitive
    ASSERT_EQ(ChanInfo::SP_UNKNOWN, ChanInfo::getProtocolFromStr("RTMP"));
}

TEST_F(ChanInfoFixture, setContentType)
{
    ASSERT_EQ(ChanInfo::T_UNKNOWN, info.contentType);

    info.setContentType(ChanInfo::T_MKV);
    ASSERT_EQ(ChanInfo::T_MKV, info.contentType);
    ASSERT_STREQ("MKV", info.contentTypeStr.cstr());
    ASSERT_STREQ("video/x-matroska", info.MIMEType.cstr());
    ASSERT_STREQ(".mkv", info.streamExt.cstr());
}
