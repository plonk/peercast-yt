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
#include "chanmgr.h"

// ------------------------------------------
bool ChannelStream::getStatus(std::shared_ptr<Channel> ch, ChanPacket &pack)
{
    unsigned int ctime = sys->getTime();

    auto chl = chanMgr->findHitListByID(ch->info.id);

    if (!chl)
        return false;

    int newLocalListeners = ch->localListeners();
    int newLocalRelays = ch->localRelays();

    if (
        (
        (numListeners != newLocalListeners)
        || (numRelays != newLocalRelays)
        || (ch->isPlaying() != isPlaying)
        || (servMgr->getFirewall(ch->ipVersion) != fwState)
        || ((ctime - lastUpdate) > 120)
        )
        && ((ctime - lastUpdate) > 10)
       )
    {
        numListeners = newLocalListeners;
        numRelays = newLocalRelays;
        isPlaying = ch->isPlaying();
        fwState = servMgr->getFirewall(ch->ipVersion);
        lastUpdate = ctime;

        ChanHit hit;

        unsigned int oldp = ch->rawData.getOldestPos();
        unsigned int newp = ch->rawData.getLatestPos();

        hit.initLocal(numListeners, numRelays, ch->info.numSkips, ch->info.getUptime(), isPlaying, oldp, newp, ch->canAddRelay(), ch->sourceHost.host, (ch->ipVersion == Channel::IP_V6));
        hit.tracker = ch->isBroadcasting();

        MemoryStream pmem(pack.data, sizeof(pack.data));
        AtomStream atom(pmem);

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
            hit.writeAtoms(atom, GnuID());

        pack.len = pmem.pos;
        pack.type = ChanPacket::T_PCP;
        return true;
    }else
        return false;
}

// ------------------------------------------
void ChannelStream::updateStatus(std::shared_ptr<Channel> ch)
{
    ChanPacket pack;
    if (getStatus(ch, pack))
    {
        if (!ch->isBroadcasting())
        {
            int cnt = chanMgr->broadcastPacketUp(pack, ch->info.id, servMgr->sessionID, GnuID());
            LOG_INFO("Sent channel status update to %d clients", cnt);
        }
    }
}

// -----------------------------------
void ChannelStream::readRaw(Stream &in, std::shared_ptr<Channel> ch)
{
    ChanPacket pack;
    const int readLen = 8192;

    pack.init(ChanPacket::T_DATA, pack.data, readLen, ch->streamPos);
    in.read(pack.data, pack.len);
    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);

    ch->streamPos += pack.len;
}
