// ------------------------------------------------
// File : cstream.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Channel streaming classes. These do the actual
//      streaming of media between clients.
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

#include "channel.h"
#include "cstream.h"
#include "pcp.h"
#include "servmgr.h"
#include "version2.h"
#include "critsec.h"

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
void ChanPacket::writePeercast(Stream &out)
{
    unsigned int tp = 0;
    switch (type)
    {
        case T_HEAD: tp = 'HEAD'; break;
        case T_META: tp = 'META'; break;
        case T_DATA: tp = 'DATA'; break;
    }

    if (type != T_UNKNOWN)
    {
        out.writeTag(tp);
        out.writeShort(len);
        out.writeShort(0);
        out.write(data, len);
    }
}

// -----------------------------------
ChanPacket& ChanPacket::operator=(const ChanPacket& other)
{
    this->type = other.type;
    this->len  = other.len;
    this->pos  = other.pos;
    this->sync = other.sync;
    this->cont = other.cont;
    memcpy(this->data, other.data, this->len);

    return *this;
}

// -----------------------------------
void ChanPacket::readPeercast(Stream &in)
{
    unsigned int tp = in.readTag();

    switch (tp)
    {
        case 'HEAD':    type = T_HEAD; break;
        case 'DATA':    type = T_DATA; break;
        case 'META':    type = T_META; break;
        default:        type = T_UNKNOWN;
    }
    len = in.readShort();
    in.readShort();
    if (len > MAX_DATALEN)
        throw StreamException("Bad ChanPacket");
    in.read(data, len);
}

// -----------------------------------
// (使われていないようだ。)
int ChanPacketBuffer::copyFrom(ChanPacketBuffer &buf, unsigned int reqPos)
{
    lock.on();
    buf.lock.on();

    firstPos = 0;
    lastPos = 0;
    safePos = 0;
    readPos = 0;

    for (unsigned int i = buf.firstPos; i <= buf.lastPos; i++)
    {
        ChanPacket *src = &buf.packets[i%MAX_PACKETS];
        if (src->type & accept)
        {
            if (src->pos >= reqPos)
            {
                lastPos = writePos;
                packets[writePos++] = *src;
            }
        }
    }

    buf.lock.off();
    lock.off();
    return lastPos - firstPos;
}

// ------------------------------------------------------------------
// ストリームポジションが spos か、それよりも新しいパケットが見付かれ
// ば pack に代入する。見付かった場合は true, そうでなければ false を
// 返す。
bool ChanPacketBuffer::findPacket(unsigned int spos, ChanPacket &pack)
{
    if (writePos == 0)
        return false;

    lock.on();

    unsigned int fpos = getStreamPos(firstPos);
    if (spos < fpos)
        spos = fpos;

    // このループ、lastPos == UINT_MAX の時終了しないのでは？ …4G パ
    // ケットも送らないか。
    for (unsigned int i = firstPos; i <= lastPos; i++)
    {
        ChanPacket &p = packets[i%MAX_PACKETS];
        if (p.pos >= spos)
        {
            pack = p;
            lock.off();
            return true;
        }
    }

    lock.off();
    return false;
}

// ------------------------------------------------------------------
// バッファー内の一番新しいパケットのストリームポジションを返す。まだ
// パケットがない場合は 0 を返す。
unsigned int    ChanPacketBuffer::getLatestPos()
{
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

    CriticalSection cs(lock);

    for (int64_t i = lastPos; i >= firstPos; i--)
    {
        ChanPacket &p = packets[i%MAX_PACKETS];
        if (!p.cont)
            return p.pos;
    }

    return 0;
}

// ------------------------------------------------------------------
unsigned int    ChanPacketBuffer::getOldestNonContinuationPos()
{
    if (writePos == 0)
        return 0;

    CriticalSection cs(lock);

    for (int64_t i = firstPos; i <= lastPos; i++)
    {
        ChanPacket &p = packets[i%MAX_PACKETS];
        if (!p.cont)
            return p.pos;
    }

    return 0;
}

// ------------------------------------------------------------------
// バッファー内の一番古いパケットのストリームポジションを返す。まだパ
// ケットが無い場合は 0 を返す。
unsigned int    ChanPacketBuffer::getOldestPos()
{
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
    return packets[index%MAX_PACKETS].pos;
}

// -------------------------------------------------------------------
// パケットインデックス index のパケットの次のパケットのストリームポジ
// ションを計算する。
unsigned int    ChanPacketBuffer::getStreamPosEnd(unsigned int index)
{
    return packets[index%MAX_PACKETS].pos + packets[index%MAX_PACKETS].len;
}

// -----------------------------------
bool ChanPacketBuffer::writePacket(ChanPacket &pack, bool updateReadPos)
{
    if (pack.len)
    {
        if (willSkip()) // too far behind
            return false;

        lock.on();

        pack.sync = writePos;
        packets[writePos%MAX_PACKETS] = pack;
        lastPos = writePos;
        writePos++;

        if (writePos >= MAX_PACKETS)
            firstPos = writePos-MAX_PACKETS;
        else
            firstPos = 0;

        if (writePos >= NUM_SAFEPACKETS)
            safePos = writePos - NUM_SAFEPACKETS;
        else
            safePos = 0;

        if (updateReadPos)
            readPos = writePos;

        lastWriteTime = sys->getTime();

        lock.off();
        return true;
    }

    return false;
}

// -----------------------------------
void    ChanPacketBuffer::readPacket(ChanPacket &pack)
{
    unsigned int tim = sys->getTime();

    if (readPos < firstPos)
        throw StreamException("Read too far behind");

    while (readPos >= writePos)
    {
        sys->sleepIdle();
        if ((sys->getTime() - tim) > 30)
            throw TimeoutException();
    }
    lock.on();
    pack = packets[readPos%MAX_PACKETS];
    readPos++;
    lock.off();

    sys->sleepIdle();
}

// ------------------------------------------------------------
// バッファーがいっぱいなら true を返す。そうでなければ false。
bool    ChanPacketBuffer::willSkip()
{
    return ((writePos - readPos) >= MAX_PACKETS);
}

// ------------------------------------------
bool ChannelStream::getStatus(Channel *ch, ChanPacket &pack)
{
    unsigned int ctime = sys->getTime();

    ChanHitList *chl = chanMgr->findHitListByID(ch->info.id);

    if (!chl)
        return false;

    int newLocalListeners = ch->localListeners();
    int newLocalRelays = ch->localRelays();

    if (
        (
        (numListeners != newLocalListeners)
        || (numRelays != newLocalRelays)
        || (ch->isPlaying() != isPlaying)
        || (servMgr->getFirewall() != fwState)
        || ((ctime - lastUpdate) > 120)
        )
        && ((ctime - lastUpdate) > 10)
       )
    {
        numListeners = newLocalListeners;
        numRelays = newLocalRelays;
        isPlaying = ch->isPlaying();
        fwState = servMgr->getFirewall();
        lastUpdate = ctime;

        ChanHit hit;

        unsigned int oldp = ch->rawData.getOldestPos();
        unsigned int newp = ch->rawData.getLatestPos();

        hit.initLocal(numListeners, numRelays, ch->info.numSkips, ch->info.getUptime(), isPlaying, oldp, newp, ch->sourceHost.host);
        hit.tracker = ch->isBroadcasting();

        MemoryStream pmem(pack.data, sizeof(pack.data));
        AtomStream atom(pmem);

        GnuID noID;

        atom.writeParent(PCP_BCST, 10);
            atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_TRACKERS);
            atom.writeChar(PCP_BCST_HOPS, 0);
            atom.writeChar(PCP_BCST_TTL, 11);
            atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
            atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
            atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
            atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
            atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
            atom.writeBytes(PCP_BCST_CHANID, ch->info.id.id, 16);
            hit.writeAtoms(atom, noID);

        pack.len = pmem.pos;
        pack.type = ChanPacket::T_PCP;
        return true;
    }else
        return false;
}

// ------------------------------------------
void ChannelStream::updateStatus(Channel *ch)
{
    ChanPacket pack;
    if (getStatus(ch, pack))
    {
        if (!ch->isBroadcasting())
        {
            GnuID noID;
            int cnt = chanMgr->broadcastPacketUp(pack, ch->info.id, servMgr->sessionID, noID);
            LOG_CHANNEL("Sent channel status update to %d clients", cnt);
        }
    }
}

// -----------------------------------
void ChannelStream::readRaw(Stream &in, Channel *ch)
{
    ChanPacket pack;
    const int readLen = 8192;

    pack.init(ChanPacket::T_DATA, pack.data, readLen, ch->streamPos);
    in.read(pack.data, pack.len);
    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);

    ch->streamPos+=pack.len;
}
