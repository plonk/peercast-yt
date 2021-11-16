// ------------------------------------------------
// File : cstream.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//
// (c) 2002 peercast.org
//
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include "chanpacket.h"
#include "sys.h"
#include "stream.h"

// -----------------------------------
void ChanPacket::init(TYPE t, const void *p, unsigned int l, unsigned int _pos)
{
    type = t;
    if (l > MAX_DATALEN)
        throw StreamException("Packet data too large");
    len = l;
    memcpy(data, p, len);
    pos = _pos;
}

// -----------------------------------
void ChanPacket::writeRaw(Stream &out)
{
    out.write(data, len);
}

// -----------------------------------
// ChanPacket& ChanPacket::operator=(const ChanPacket& other)
// {
//     this->type = other.type;
//     this->len  = other.len;
//     this->pos  = other.pos;
//     this->sync = other.sync;
//     this->cont = other.cont;
//     memcpy(this->data, other.data, this->len);

//     return *this;
// }

// ------------------------------------------------------------------
// ストリームポジションが spos か、それよりも新しいパケットが見付かれ
// ば pack に代入する。見付かった場合は true, そうでなければ false を
// 返す。
bool ChanPacketBuffer::findPacket(unsigned int spos, ChanPacket &pack)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    if (writePos == 0)
        return false;

    unsigned int fpos = getStreamPos(firstPos);
    if (spos < fpos)
        spos = fpos;

    auto result = std::find_if(packets.begin(), packets.end(),
                               [=](const std::pair<unsigned int,std::shared_ptr<ChanPacket>>& pair) { return pair.second->pos >= spos; });
    if (result != packets.end())
    {
        pack = *result->second;
        return true;
    }

    return false;
}

// ------------------------------------------------------------------
// バッファー内の一番新しいパケットのストリームポジションを返す。まだ
// パケットがない場合は 0 を返す。
unsigned int    ChanPacketBuffer::getLatestPos()
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    if (!writePos)
        return 0;
    else
        return getStreamPos(lastPos);
}

// ------------------------------------------------------------------
unsigned int    ChanPacketBuffer::getLatestNonContinuationPos()
{
    if (writePos == 0)
        return 0;

    std::lock_guard<std::recursive_mutex> cs(lock);

    for (int64_t i = lastPos; i >= firstPos; i--)
    {
        auto p = packets.at((unsigned int) i);
        if (!p->cont)
            return p->pos;
    }

    return 0;
}

// ------------------------------------------------------------------
unsigned int    ChanPacketBuffer::getOldestNonContinuationPos()
{
    if (writePos == 0)
        return 0;

    std::lock_guard<std::recursive_mutex> cs(lock);

    for (int64_t i = firstPos; i <= lastPos; i++)
    {
        auto p = packets.at((unsigned int) i);
        if (!p->cont)
            return p->pos;
    }

    return 0;
}

// ------------------------------------------------------------------
// バッファー内の一番古いパケットのストリームポジションを返す。まだパ
// ケットが無い場合は 0 を返す。
unsigned int    ChanPacketBuffer::getOldestPos()
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    if (!writePos)
        return 0;
    else
        return getStreamPos(firstPos);
}

// -----------------------------------
unsigned int    ChanPacketBuffer::findOldestPos(unsigned int spos)
{
    unsigned int min = getStreamPos(safePos);
    unsigned int max = getStreamPos(lastPos);

    if (min > spos)
        return min;

    if (max < spos)
        return max;

    return spos;
}

// -------------------------------------------------------------------
// パケットインデックス index のパケットのストリームポジションを返す。
unsigned int    ChanPacketBuffer::getStreamPos(unsigned int index)
{
    return packets.at((unsigned int) index)->pos;
}

// -----------------------------------
bool ChanPacketBuffer::writePacket(ChanPacket &pack, bool updateReadPos)
{
    if (pack.len == 0)
        return false;

    if (willSkip()) // too far behind [読み出しが遅すぎる]
        return false;

    std::lock_guard<std::recursive_mutex> cs(lock);
    unsigned int now = sys->getTime();

    pack.sync = writePos;
    pack.time = now;
    packets[(unsigned int) writePos] = std::make_shared<ChanPacket>(pack);
    lastPos = writePos;
    writePos++;

    if (writePos >= MAX_PACKETS)
    {
        auto begin = packets.find((unsigned int ) firstPos);
        auto end = packets.find((unsigned int) lastPos);
        auto result = std::find_if(begin, end, [=](const std::pair<unsigned int,std::shared_ptr<ChanPacket>>& pair) { return pair.second->time >= now - 10; });
        if (result != end)
        {
            firstPos = result->first;
            packets.erase(begin, result);
        }
    }else
        firstPos = 0;

    if (writePos >= NUM_SAFEPACKETS)
        safePos = writePos - NUM_SAFEPACKETS;
    else
        safePos = 0;

    if (updateReadPos)
        readPos = writePos;

    lastWriteTime = now;

    return true;
}

// -----------------------------------
void    ChanPacketBuffer::readPacket(ChanPacket &pack)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int tim = sys->getTime();

    if (readPos < firstPos)
        throw StreamException("Read too far behind");

    while (readPos >= writePos)
    {
        lock.unlock();
        sys->sleepIdle();
        lock.lock();
        if ((sys->getTime() - tim) > 30)
            throw TimeoutException();
    }
    pack = *packets.at((unsigned int) readPos);
    readPos++;

    sys->sleepIdle();
}

// ------------------------------------------------------------
// バッファーがいっぱいなら true を返す。そうでなければ false。
bool    ChanPacketBuffer::willSkip()
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    return ((writePos - readPos) >= MAX_PACKETS);
}
