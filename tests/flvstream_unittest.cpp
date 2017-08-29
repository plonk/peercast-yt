#include <gtest/gtest.h>

#include "flv.h"
#include "amf0.h"

class FLVStreamFixture : public ::testing::Test {
public:
    FLVStreamFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~FLVStreamFixture()
    {
    }
};

TEST_F(FLVStreamFixture, readMetaData_commonCase)
{
    std::string buf;

    buf += amf0::Value("onMetaData").serialize();
    buf += amf0::Value::object(
        {
            { "videodatarate", 100 },
            { "audiodatarate", 50 }
        }).serialize();

    bool success;
    int bitrate;
    std::tie(success, bitrate) = FLVStream::readMetaData(const_cast<char*>(buf.data()), buf.size());

    ASSERT_TRUE(success);
    ASSERT_EQ(150, bitrate);
}

TEST_F(FLVStreamFixture, readMetaData_notOnMetaData)
{
    std::string buf;

    buf += amf0::Value("hoge").serialize();
    buf += amf0::Value::object(
        {
            { "videodatarate", 100 },
            { "audiodatarate", 50 }
        }).serialize();

    bool success;
    int bitrate;
    std::tie(success, bitrate) = FLVStream::readMetaData(const_cast<char*>(buf.data()), buf.size());

    ASSERT_FALSE(success);
}

TEST_F(FLVStreamFixture, readMetaData_onlyVideo)
{
    std::string buf;

    buf += amf0::Value("onMetaData").serialize();
    buf += amf0::Value::object(
        {
            { "videodatarate", 100 },
        }).serialize();

    bool success;
    int bitrate;
    std::tie(success, bitrate) = FLVStream::readMetaData(const_cast<char*>(buf.data()), buf.size());

    ASSERT_TRUE(success);
    ASSERT_EQ(100, bitrate);
}

TEST_F(FLVStreamFixture, readMetaData_onlyAudio)
{
    std::string buf;

    buf += amf0::Value("onMetaData").serialize();
    buf += amf0::Value::object(
        {
            { "audiodatarate", 50 },
        }).serialize();

    bool success;
    int bitrate;
    std::tie(success, bitrate) = FLVStream::readMetaData(const_cast<char*>(buf.data()), buf.size());

    ASSERT_TRUE(success);
    ASSERT_EQ(50, bitrate);
}

TEST_F(FLVStreamFixture, readMetaData_neitherRateIsSupplied)
{
    std::string buf;

    buf += amf0::Value("onMetaData").serialize();
    buf += amf0::Value::object({}).serialize();

    bool success;
    int bitrate;
    std::tie(success, bitrate) = FLVStream::readMetaData(const_cast<char*>(buf.data()), buf.size());

    ASSERT_FALSE(success);
}
