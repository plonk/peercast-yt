#include <limits.h> // INT_MAX

#include "mkv.h"
#include "channel.h"
#include "stream.h"
#include "matroska.h"

using namespace matroska;

void MKVStream::sendPacket(ChanPacket::TYPE type, const matroska::byte_string& data, Channel* ch)
{
    LOG_DEBUG("MKV Sending %zu byte %s packet", data.size(), type==ChanPacket::T_HEAD?"HEAD":"DATA");

    ChanPacket pack;
    pack.type = type;
    pack.pos = ch->streamPos;
    pack.len = data.size();
    memcpy(pack.data, data.data(), data.size());

    if (type == ChanPacket::T_HEAD)
        ch->headPack = pack;

    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);
    ch->streamPos += pack.len;
}

void MKVStream::readHeader(Stream &in, Channel *ch)
{
    byte_string header;

    while (true)
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);
        LOG_DEBUG("Got LEVEL0 %s size=%s",
                  id.toName().c_str(),
                  std::to_string(size.uint()).c_str());

        header += id.bytes;
        header += size.bytes;

        if (id.toName() != "Segment")
        {
            uint8_t buffer[4096];
            uint64_t remaining = size.uint();

            while (remaining > 0)
            {
                int readSize = std::min(remaining, (uint64_t)4096);
                int r;
                r = in.read(buffer, readSize);
                header += byte_string(buffer, buffer + r);
                remaining -= r;
            }
        }else
        {
            while (true)
            {
                VInt id = VInt::read(in);
                VInt size = VInt::read(in);
                LOG_DEBUG("Got LEVEL1 %s size=%s",
                          id.toName().c_str(),
                          std::to_string(size.uint()).c_str());

                if (id.toName() != "Cluster")
                {
                    header += id.bytes;
                    header += size.bytes;

                    uint8_t buffer[4096];
                    uint64_t remaining = size.uint();

                    while (remaining > 0)
                    {
                        int readSize = std::min(remaining, (uint64_t)4096);
                        int r;
                        r = in.read(buffer, readSize);
                        header += byte_string(buffer, buffer + r);
                        remaining -= r;
                    }
                } else
                {
                    // ヘッダーパケットを送信
                    if (header.size() > ChanPacket::MAX_DATALEN)
                    {
                        throw StreamException("MKV header too big");
                    }
                    sendPacket(ChanPacket::T_HEAD, header, ch);

                    // 最初のクラスターを送信

                    // クラスターヘッダー
                    sendPacket(ChanPacket::T_DATA, id.bytes + size.bytes, ch);

                    uint64_t sizeRemaining = size.uint();
                    char buf[15 * 1024];
                    while (sizeRemaining > 15 * 1024)
                    {
                        in.read(buf, 15 * 1024);
                        sendPacket(ChanPacket::T_DATA,
                                   byte_string(buf, buf + 15 * 1024),
                                   ch);
                        sizeRemaining -= 15 * 1024;
                    }
                    if (sizeRemaining != 0)
                    {
                        in.read(buf, sizeRemaining);
                        sendPacket(ChanPacket::T_DATA,
                                   byte_string(buf, buf + sizeRemaining),
                                   ch);
                        sizeRemaining = 0;
                    }
                    return;
                }
            }
        }
    }
}

// Cluster 要素を読む
int MKVStream::readPacket(Stream &in, Channel *ch)
{
    { // ビットレートの計測、更新
        ChanInfo info = ch->info;
        int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
        if (newBitrate > info.bitrate) {
            info.bitrate = newBitrate;
            ch->updateInfo(info);
        }
    }

    matroska::byte_string buffer;

    VInt id = VInt::read(in);
    VInt size = VInt::read(in);

    LOG_DEBUG("Got %s size=%s", id.toName().c_str(),
              std::to_string(size.uint()).c_str());
    if (id.toName() != "Cluster")
    {
        LOG_ERROR("Cluster expected, but got %s", id.toName().c_str());
        return 1; // 正しいエラーコードは？
    }

    buffer += id.bytes;
    buffer += size.bytes;

    // クラスター要素のペイロードの残りバイト数
    uint64_t clusterRemaining = size.uint();

    // クラスターを分解する
    while (clusterRemaining > 0)
    {
        VInt id = VInt::read(in);
        VInt size = VInt::read(in);

        LOG_DEBUG("Got %s size=%s", id.toName().c_str(),
                  std::to_string(size.uint()).c_str());

        if (buffer.size() + size.uint() > 15 * 1024)
        {
            sendPacket(ChanPacket::T_DATA, buffer, ch);
            buffer.clear();
        }

        buffer += id.bytes;
        buffer += size.bytes;
        clusterRemaining -= id.bytes.size() + size.bytes.size();

        int sizeRemaining = size.uint();
        uint8_t buf[15 * 1024];
        if (sizeRemaining > 15 * 1024)
        {
            // クラスターが 15KB を超える時は分割して最後まで送り切る

            if (buffer.size() > 0)
            {
                sendPacket(ChanPacket::T_DATA, buffer, ch);
                buffer.clear();
            }

            while (sizeRemaining > 15 * 1024)
            {
                in.read(buf, 15 * 1024);
                sendPacket(ChanPacket::T_DATA,
                           byte_string(buf, buf + 15 * 1024),
                           ch);
                sizeRemaining -= 15 * 1024;
            }
            if (sizeRemaining != 0)
            {
                in.read(buf, sizeRemaining);
                sendPacket(ChanPacket::T_DATA,
                           byte_string(buf, buf + sizeRemaining),
                           ch);
                sizeRemaining = 0;
            }
        }
        else
        {
            // バッファーに入れるだけ
            in.read(buf, sizeRemaining);
            buffer += byte_string(buf, buf + sizeRemaining);
        }
        clusterRemaining -= size.uint();
    }

    if (buffer.size() > 0)
    {
        sendPacket(ChanPacket::T_DATA, buffer, ch);
        buffer.clear();
    }

    return 0; // no error
}

void MKVStream::readEnd(Stream &, Channel *)
{
    // we will never reach the end
}
