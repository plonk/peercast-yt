#include <limits.h> // INT_MAX

#include "mkv.h"
#include "channel.h"
#include "stream.h"
#include "matroska.h"
#include "sstream.h"

using namespace matroska;

// data を type パケットとして送信する
void MKVStream::sendPacket(ChanPacket::TYPE type, const byte_string& data, bool continuation, std::shared_ptr<Channel> ch)
{
    if (data.size() > ChanPacket::MAX_DATALEN)
        throw StreamException("MKV packet too big");

    if (type == ChanPacket::T_HEAD)
    {
        ch->resetStream();
    }

    ChanPacket pack;
    pack.type = type;
    pack.pos  = ch->streamPos;
    pack.len  = data.size();
    pack.cont = continuation;
    memcpy(pack.data, data.data(), data.size());

    if (type == ChanPacket::T_HEAD)
        ch->headPack = pack;

    ch->newPacket(pack);
    // rateLimit で律速するので checkReadDelay は使わない。
    //ch->checkReadDelay(pack.len);
    ch->streamPos += pack.len;
}

bool MKVStream::hasKeyFrame(const byte_string& cluster)
{
    MemoryStream in(const_cast<unsigned char*>(cluster.data()), cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    int64_t payloadRemaining = (int64_t) size.uint(); // 負数が取れるように signed にする
    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");
    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id   = VInt::read(in);
        VInt size = VInt::read(in);
        std::string blockData = static_cast<Stream*>(&in)->read((int) size.uint());

        if (id.toName() == "SimpleBlock")
        {
            MemoryStream mem((void*)blockData.data(), blockData.size());

            VInt trackno = VInt::read(mem);
            if (trackno.uint() == m_videoTrackNumber)
            {
                if (((uint8_t)blockData[trackno.bytes.size() + 2] & 0x80) != 0)
                {
                    m_hasKeyFrame = true;
                    return true; // キーフレームがある
                }
            }
        }
        payloadRemaining -= id.bytes.size() + size.bytes.size() + size.uint();
    }
    if (payloadRemaining != 0)
        throw StreamException("MKV Parse error");
    return false;
}

uint64_t MKVStream::unpackUnsignedInt(const std::string& bytes)
{
    if (bytes.size() == 0)
        throw std::runtime_error("empty string");

    uint64_t res = 0;

    for (size_t i = 0; i < bytes.size(); i++)
    {
        res <<= 8;
        res |= (uint8_t) bytes[i];
    }

    return res;
}

void MKVStream::rateLimit(uint64_t timecode)
{
    // Timecode は単調増加ではないが、少しのジッターはバッファーが吸収
    // してくれるだろう。

    unsigned int secondsFromStart = timecode * m_timecodeScale / 1000000000;
    unsigned int ctime = sys->getTime();

    if (m_startTime + secondsFromStart > ctime) // if this is into the future
    {
        int diff = (m_startTime + secondsFromStart) - ctime;
        LOG_DEBUG("rateLimit: diff = %d sec", diff);
        sys->sleep(diff * 1000);
    }
}

// 非継続パケットの頭出しができないクライアントのために、なるべく要素
// をパケットの先頭にして送信する
void MKVStream::sendCluster(const byte_string& cluster, std::shared_ptr<Channel> ch)
{
    bool continuation;

    if (hasKeyFrame(cluster))
        continuation = false;
    else
    {
        if (m_hasKeyFrame)
            continuation = true;
        else
            continuation = false;
    }

    MemoryStream in((void*) cluster.data(), cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    byte_string buffer = id.bytes + size.bytes;

    int64_t payloadRemaining = (int64_t) size.uint(); // 負数が取れるように signed にする

    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");

    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (buffer.size() > 0 &&
            buffer.size() + id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
            continuation = true;
            buffer.clear();
        }

        std::string payload = in.Stream::read((int) size.uint());

        if (id.toName() == "Timecode")
        {
            if (ch->readDelay)
                rateLimit(unpackUnsignedInt(payload));
        }

        if (id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            if (buffer.size() != 0) throw StreamException("Logic error");
            buffer = id.bytes + size.bytes;
            buffer.append(payload.begin(), payload.end());
            size_t pos = 0;
            while (pos < buffer.size())
            {
                int next = std::min(pos + 15*1024, buffer.size());
                sendPacket(ChanPacket::T_DATA, buffer.substr(pos, next-pos), continuation, ch);
                continuation = true;
                pos = next;
            }
            buffer.clear();
        } else {
            buffer += id.bytes + size.bytes;
            buffer.append(payload.begin(), payload.end());
            MemoryStream mem((void*)buffer.c_str(), buffer.size());
            mem.rewind();
            VInt id = VInt::read(mem);
        }
        payloadRemaining -= id.bytes.size() + size.bytes.size() + size.uint();
    }

    if (buffer.size() > 0)
    {
        sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
    }
}

// Tracks 要素からビデオトラックのトラック番号を調べる。
void MKVStream::readTracks(const std::string& data)
{
    StringStream mem;
    mem.str(data);

    while (!mem.eof())
    {
        VInt id   = VInt::read(mem);
        VInt size = VInt::read(mem);
        LOG_DEBUG("Got LEVEL2 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (id.toName() == "TrackEntry")
        {
            int end = mem.getPosition() + size.uint();
            int trackno = -1;
            int tracktype = -1;

            while (mem.getPosition() < end)
            {
                VInt id   = VInt::read(mem);
                VInt size = VInt::read(mem);

                if (id.toName() == "TrackNumber")
                    trackno = (uint8_t) mem.readChar();
                else if (id.toName() == "TrackType")
                    tracktype = (uint8_t) mem.readChar();
                else
                    mem.skip(size.uint());
            }

            if (tracktype == 1)
            {
                m_videoTrackNumber = trackno;
                LOG_DEBUG("MKV video track number is %d", trackno);
            }
        }else
        {
            mem.skip(size.uint());
        }
    }
}

// TimecodeScale の値を調べる。
void MKVStream::readInfo(const std::string& data)
{
    StringStream in;
    in.str(data);

    LOG_TRACE("Info length = %d", (int) in.getLength());

    try {
        while (in.getPosition() < in.getLength())
        {
            int pos = in.getPosition();
            VInt id   = VInt::read(in);
            VInt size = VInt::read(in);

            LOG_TRACE("readInfo: Got %s 0x%lX with size=%d at pos %d",
                      id.toName().c_str(),
                      (unsigned long int) id.uint(),
                      (int) size.uint(),
                      pos);

            auto data = in.Stream::read((int) size.uint());

            if (id.toName() == "TimecodeScale")
            {
                auto scale = unpackUnsignedInt(data);
                LOG_DEBUG("TimecodeScale = %d nanoseconds", (int) scale);
                m_timecodeScale = scale;
            }
        }
    } catch (std::exception& e) {
        LOG_ERROR("Failed to parse Info: %s", e.what());
    }
}

void MKVStream::readHeader(Stream &in, std::shared_ptr<Channel> ch)
{
    try{
        byte_string header;

        while (true)
        {
            VInt id   = VInt::read(in);
            VInt size = VInt::read(in);
            LOG_DEBUG("Got LEVEL0 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

            header += id.bytes;
            header += size.bytes;

            if (id.toName() != "Segment")
            {
                // Segment 以外のレベル 0 要素は単にヘッドパケットに追加す
                // る
                auto data = in.read((int) size.uint());
                header.append(data.begin(), data.end());
            }else
            {
                // Segment 内のレベル 1 要素を読む
                while (true)
                {
                    VInt id = VInt::read(in);
                    VInt size = VInt::read(in);
                    LOG_DEBUG("Got LEVEL1 %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

                    if (id.toName() != "Cluster")
                    {
                        // Cluster 以外の要素はヘッドパケットに追加する
                        header += id.bytes;
                        header += size.bytes;

                        auto data = in.read((int) size.uint());

                        if (id.toName() == "Tracks")
                            readTracks(data);

                        header.append(data.begin(), data.end());

                        if (id.toName() == "Info")
                            readInfo(data);
                    } else
                    {
                        // ヘッダーパケットを送信
                        sendPacket(ChanPacket::T_HEAD, header, false, ch);

                        m_startTime = sys->getTime();

                        // もうIDとサイズを読んでしまったので、最初のクラ
                        // スターを送信

                        byte_string cluster = id.bytes + size.bytes;
                        auto data = in.read((int) size.uint());
                        cluster.append(data.begin(), data.end());
                        sendCluster(cluster, ch);
                        return;
                    }
                }
            }
        }
    }catch (std::runtime_error& e)
    {
        throw StreamException(e.what());
    }
}

// ビットレートの計測、更新
void MKVStream::checkBitrate(Stream &in, std::shared_ptr<Channel> ch)
{
    ChanInfo info = ch->info;
    int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
    if (newBitrate > info.bitrate) {
        info.bitrate = newBitrate;
        ch->updateInfo(info);
    }
}

// Cluster 要素を読む
int MKVStream::readPacket(Stream &in, std::shared_ptr<Channel> ch)
{
    try {
        checkBitrate(in, ch);

        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        if (id.toName() != "Cluster")
        {
            LOG_ERROR("Cluster expected, but got %s", id.toName().c_str());
            throw StreamException("Logic error");
        }

        byte_string cluster = id.bytes + size.bytes;
        auto data = in.read((int) size.uint());
        cluster.append(data.begin(), data.end());
        sendCluster(cluster, ch);

        return 0; // no error
    }catch (std::runtime_error& e)
    {
        throw StreamException(e.what());
    }
}

void MKVStream::readEnd(Stream &, std::shared_ptr<Channel>)
{
    // we will never reach the end
}
