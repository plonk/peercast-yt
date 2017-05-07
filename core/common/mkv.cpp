#include <limits.h> // INT_MAX

#include "mkv.h"
#include "channel.h"
#include "stream.h"
#include "matroska.h"
#include "dmstream.h"

using namespace matroska;

// data を type パケットとして送信する
void MKVStream::sendPacket(ChanPacket::TYPE type, const byte_string& data, bool continuation, Channel* ch)
{
    //LOG_DEBUG("MKV Sending %zu byte %s packet", data.size(), type==ChanPacket::T_HEAD?"HEAD":"DATA");

    if (data.size() > ChanPacket::MAX_DATALEN)
        throw StreamException("MKV packet too big");

    ChanPacket pack;
    pack.type = type;
    pack.pos  = ch->streamPos;
    pack.len  = data.size();
    pack.cont = continuation;
    memcpy(pack.data, data.data(), data.size());

    if (type == ChanPacket::T_HEAD)
        ch->headPack = pack;

    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);
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

// 非継続パケットの頭出しができないクライアントのために、なるべく要素
// をパケットの先頭にして送信する
void MKVStream::sendCluster(const byte_string& cluster, Channel* ch)
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

    // 15KB 以下の場合はそのまま送信
    if (cluster.size() <= 15*1024)
    {
        sendPacket(ChanPacket::T_DATA, cluster, continuation, ch);
        return;
    }

    MemoryStream in((void*) cluster.data(), cluster.size());

    VInt id   = VInt::read(in);
    VInt size = VInt::read(in);

    //LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

    byte_string buffer = id.bytes + size.bytes;

    int64_t payloadRemaining = (int64_t) size.uint(); // 負数が取れるように signed にする
    if (payloadRemaining < 0)
        throw StreamException("MKV Parse error");
    while (payloadRemaining > 0) // for each element in Cluster
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        //LOG_DEBUG("Got %s size=%s", id.toName().c_str(), std::to_string(size.uint()).c_str());

        if (buffer.size() > 0 &&
            buffer.size() + id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            MemoryStream mem((void*)buffer.data(), buffer.size());
            VInt id = VInt::read(mem);
            //LOG_DEBUG("Sending %s", id.toName().c_str());
            sendPacket(ChanPacket::T_DATA, buffer, continuation, ch);
            continuation = true;
            buffer.clear();
        }

        if (id.bytes.size() + size.bytes.size() + size.uint() > 15*1024)
        {
            if (buffer.size() != 0) throw StreamException("Logic error");
            buffer = id.bytes + size.bytes;
            auto payload = static_cast<Stream*>(&in)->read((int) size.uint());
            buffer.append(payload.begin(), payload.end());
            int pos = 0;
            while (pos < buffer.size())
            {
                int next = std::min(pos + 15*1024, (int)buffer.size());
                sendPacket(ChanPacket::T_DATA, buffer.substr(pos, next-pos), continuation, ch);
                continuation = true;
                pos = next;
            }
            buffer.clear();
        } else {
            buffer += id.bytes + size.bytes;
            auto payload = static_cast<Stream*>(&in)->read((int) size.uint());
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
    DynamicMemoryStream mem;
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
        }
    }
}

void MKVStream::readHeader(Stream &in, Channel *ch)
{
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
                } else
                {
                    // ヘッダーパケットを送信
                    sendPacket(ChanPacket::T_HEAD, header, false, ch);

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
}

// ビットレートの計測、更新
void MKVStream::checkBitrate(Stream &in, Channel *ch)
{
    ChanInfo info = ch->info;
    int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
    if (newBitrate > info.bitrate) {
        info.bitrate = newBitrate;
        ch->updateInfo(info);
    }
}

// Cluster 要素を読む
int MKVStream::readPacket(Stream &in, Channel *ch)
{
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
}

void MKVStream::readEnd(Stream &, Channel *)
{
    // we will never reach the end
}
