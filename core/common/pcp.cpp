// ------------------------------------------------
// File : pcp.cpp
// Date: 1-mar-2004
// Author: giles
//
// (c) 2002-4 peercast.org
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

#include "atom.h"
#include "pcp.h"
#include "peercast.h"
#include "version2.h"

// ------------------------------------------
void PCPStream::init(const GnuID &rid)
{
    remoteID = rid;
    routeList.clear();

    lastPacketTime = 0;
    nextRootPacket = 0;  // 0 seconds (never)

    inData.init();
    inData.accept = ChanPacket::T_PCP;

    outData.init();
    outData.accept = ChanPacket::T_PCP;
}

// ------------------------------------------
void PCPStream::readVersion(Stream &in)
{
    int len = in.readInt();

    if (len != 4)
        throw StreamException("Invalid PCP");

    int ver = in.readInt();

    LOG_DEBUG("PCP ver: %d", ver);
}

// ------------------------------------------
void PCPStream::readHeader(Stream &in, std::shared_ptr<Channel>)
{
//  AtomStream atom(in);

//  if (in.readInt() != PCP_CONNECT)
//      throw StreamException("Not PCP");

//  readVersion(in);
}

// ------------------------------------------
bool PCPStream::sendPacket(ChanPacket &pack, const GnuID &destID)
{
    if (destID.isSet())
        if (!destID.isSame(remoteID))
            if (!routeList.contains(destID))
                return false;

    return outData.writePacket(pack);
}

// ------------------------------------------
void PCPStream::flush(Stream &in)
{
    ChanPacket pack;
    // send outward packets
    while (outData.numPending())
    {
        outData.readPacket(pack);
        pack.writeRaw(in);
    }
}

// ------------------------------------------
int PCPStream::readPacket(Stream &in, std::shared_ptr<Channel>)
{
    BroadcastState bcs;
    return readPacket(in, bcs);
}

// ------------------------------------------
int PCPStream::readPacket(Stream &in, BroadcastState &bcs)
{
    int error = PCP_ERROR_GENERAL;
    try
    {
        AtomStream atom(in);

        ChanPacket pack;
        MemoryStream mem(pack.data, sizeof(pack.data));
        AtomStream patom(mem);

        // send outward packets
        error = PCP_ERROR_WRITE;
        if (outData.numPending())
        {
            outData.readPacket(pack);
            pack.writeRaw(in);
        }
        error = PCP_ERROR_GENERAL;

        if (outData.willSkip())
        {
            error = PCP_ERROR_WRITE+PCP_ERROR_SKIP;
            throw StreamException("Send too slow");
        }

        error = PCP_ERROR_READ;
        // poll for new downward packet
        if (in.readReady())
        {
            int numc, numd;
            ID4 id;

            id = atom.read(numc, numd);

            mem.rewind();
            pack.len = patom.writeAtoms(id, in, numc, numd);
            pack.type = ChanPacket::T_PCP;

            inData.writePacket(pack);
        }
        error = PCP_ERROR_GENERAL;

        // process downward packets
        if (inData.numPending())
        {
            inData.readPacket(pack);

            mem.rewind();

            int numc, numd;
            ID4 id = patom.read(numc, numd);

            error = PCPStream::procAtom(patom, id, numc, numd, bcs);

            if (error)
                throw StreamException("PCP exception");
        }

        error = 0;
    }catch (StreamException &e)
    {
        LOG_ERROR("PCP readPacket: %s (%d)", e.msg, error);
    }

    return error;
}

// ------------------------------------------
void PCPStream::readEnd(Stream &, std::shared_ptr<Channel>)
{
}

// ------------------------------------------
void PCPStream::readPushAtoms(AtomStream &atom, int numc, BroadcastState &bcs)
{
    Host host;
    GnuID   chanID;

    chanID.clear();

    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if (id == PCP_PUSH_IP)
            host.ip = atom.readAddress();
        else if (id == PCP_PUSH_PORT)
            host.port = atom.readShort();
        else if (id == PCP_PUSH_CHANID)
            atom.readBytes(chanID.id, 16);
        else
        {
            LOG_DEBUG("PCP skip: %s, %d, %d", id.getString().str(), c, d);
            atom.skip(c, d);
        }
    }

    if (bcs.forMe)
    {
        Servent *s = NULL;

        if (chanID.isSet())
        {
            auto ch = chanMgr->findChannelByID(chanID);
            if (ch)
                if (ch->isBroadcasting() || (!ch->isFull() && !servMgr->relaysFull() && ch->info.id.isSame(chanID)))
                    s = servMgr->allocServent();
        }else{
            s = servMgr->allocServent();
        }

        if (s)
        {
            LOG_DEBUG("GIVing to %s", host.str().c_str());
            s->initGIV(host, chanID);
        }
    }
}

// ------------------------------------------
void PCPStream::readRootAtoms(AtomStream &atom, int numc, BroadcastState &bcs)
{
    String url;

    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if (id == PCP_ROOT_UPDINT)
        {
            int si = atom.readInt();

            chanMgr->setUpdateInterval(si);
            LOG_DEBUG("PCP got new host update interval: %ds", si);
        }else if (id == PCP_ROOT_URL)
        {
            url = "http://www.peercast.org/";
            String loc;
            atom.readString(loc.data, sizeof(loc.data), d);
            url.append(loc);
        }else if (id == PCP_ROOT_CHECKVER)
        {
            unsigned int newVer = atom.readInt();
            if (newVer > PCP_CLIENT_VERSION)
            {
                Sys::strcpy_truncate(servMgr->downloadURL, sizeof(servMgr->downloadURL), url.cstr());
                peercast::notifyMessage(ServMgr::NT_UPGRADE, "There is a new version of PeerCast available, please click here to upgrade your client.");
            }
            LOG_DEBUG("PCP got version check: %d / %d", newVer, PCP_CLIENT_VERSION);
        }else if (id == PCP_ROOT_NEXT)
        {
            unsigned int time = atom.readInt();

            if (time)
            {
                unsigned int ctime = sys->getTime();
                nextRootPacket = ctime+time;
                LOG_DEBUG("PCP expecting next root packet in %us", time);
            }else
            {
                nextRootPacket = 0;
            }
        }else if (id == PCP_ROOT_UPDATE)
        {
            atom.skip(c, d);

            chanMgr->broadcastTrackerUpdate(remoteID, true);
        }else if ((id == PCP_MESG_ASCII) || (id == PCP_MESG))           // PCP_MESG_ASCII to be depreciated
        {
            String newMsg;

            atom.readString(newMsg.data, sizeof(newMsg.data), d);
            if (!newMsg.isSame(servMgr->rootMsg.cstr()))
            {
                servMgr->rootMsg = newMsg;
                LOG_DEBUG("PCP got new root mesg: %s", servMgr->rootMsg.cstr());
                if (servMgr->rootMsg != "")
                    peercast::notifyMessage(ServMgr::NT_PEERCAST,
                                            (std::string(servMgr->rootHost.str()) + "「" + servMgr->rootMsg.cstr() + "」").c_str());
            }
        }else
        {
            LOG_DEBUG("PCP skip: %s, %d, %d", id.getString().str(), c, d);
            atom.skip(c, d);
        }
    }
}

// ------------------------------------------
void PCPStream::readPktAtoms(std::shared_ptr<Channel> ch, AtomStream &atom, int numc, BroadcastState &bcs)
{
    ChanPacket pack;
    ID4 type;

    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if (id == PCP_CHAN_PKT_TYPE)
        {
            type = atom.readID4();

            if (type == PCP_CHAN_PKT_HEAD)
                pack.type = ChanPacket::T_HEAD;
            else if (type == PCP_CHAN_PKT_DATA)
                pack.type = ChanPacket::T_DATA;
            else
                pack.type = ChanPacket::T_UNKNOWN;
        }else if (id == PCP_CHAN_PKT_POS)
        {
            pack.pos = atom.readInt();
        }else if (id == PCP_CHAN_PKT_CONTINUATION)
        {
            pack.cont = atom.readChar();
        }else if (id == PCP_CHAN_PKT_DATA)
        {
            if (d > ChanPacket::MAX_DATALEN)
            {
                throw StreamException("Data size too large");
            }
            pack.len = d;
            atom.readBytes(pack.data, pack.len);
        }
        else
        {
            LOG_DEBUG("PCP skip: %s, %d, %d", id.getString().str(), c, d);
            atom.skip(c, d);
        }
    }

    if (ch)
    {
        std::lock_guard<std::recursive_mutex> cs(ch->lock);

        int diff = pack.pos - ch->streamPos;
        if (diff)
            LOG_DEBUG("PCP skipping %s%d (%u -> %u)", (diff>0)?"+":"", diff, ch->streamPos, pack.pos);

        if (pack.type == ChanPacket::T_HEAD)
        {
            LOG_DEBUG("New head packet at %u", pack.pos);

            // check for stream restart
            if (pack.pos == 0)
            {
                LOG_INFO("PCP resetting stream");
                ch->streamIndex++;
                ch->rawData.init();
            }

            ch->headPack = pack;

            ch->rawData.writePacket(pack, true);
            ch->streamPos = pack.pos+pack.len;
        }else if (pack.type == ChanPacket::T_DATA)
        {
            ch->rawData.writePacket(pack, true);
            ch->streamPos = pack.pos+pack.len;
        }
    }

    // update this parent packet stream position
    if ((pack.pos) && (!bcs.streamPos || (pack.pos < bcs.streamPos)))
        bcs.streamPos = pack.pos;
}

// -----------------------------------
void PCPStream::readHostAtoms(AtomStream &atom, int numc, BroadcastState &bcs)
{
    ChanHit hit;
    hit.init();
    GnuID chanID = bcs.chanID;  //use default

    //bool busy = false;

    unsigned int ipNum=0;

    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if (id == PCP_HOST_IP)
        {
            hit.rhost[ipNum].ip = atom.readAddress();
        }else if (id == PCP_HOST_PORT)
        {
            int port = atom.readShort();
            hit.rhost[ipNum++].port = port;

            if (ipNum > 1)
                ipNum = 1;
        }
        else if (id == PCP_HOST_NUML)
            hit.numListeners = atom.readInt();
        else if (id == PCP_HOST_NUMR)
            hit.numRelays = atom.readInt();
        else if (id == PCP_HOST_UPTIME)
            hit.upTime = atom.readInt();
        else if (id == PCP_HOST_OLDPOS)
            hit.oldestPos = atom.readInt();
        else if (id == PCP_HOST_NEWPOS)
            hit.newestPos = atom.readInt();
        else if (id == PCP_HOST_VERSION)
            hit.version = atom.readInt();
        else if (id == PCP_HOST_VERSION_VP)
            hit.versionVP = atom.readInt();
        else if (id == PCP_HOST_VERSION_EX_PREFIX)
            atom.readBytes(hit.versionExPrefix, 2);
        else if (id == PCP_HOST_VERSION_EX_NUMBER)
            hit.versionExNumber = atom.readShort();
        else if (id == PCP_HOST_FLAGS1)
        {
            int fl1 = atom.readChar();

            hit.recv = (fl1 & PCP_HOST_FLAGS1_RECV) !=0;
            hit.relay = (fl1 & PCP_HOST_FLAGS1_RELAY) !=0;
            hit.direct = (fl1 & PCP_HOST_FLAGS1_DIRECT) !=0;
            hit.cin = (fl1 & PCP_HOST_FLAGS1_CIN) !=0;
            hit.tracker = (fl1 & PCP_HOST_FLAGS1_TRACKER) !=0;
            hit.firewalled = (fl1 & PCP_HOST_FLAGS1_PUSH) !=0;
        }else if (id == PCP_HOST_ID)
            atom.readBytes(hit.sessionID.id, 16);
        else if (id == PCP_HOST_CHANID)
            atom.readBytes(chanID.id, 16);
        else if (id == PCP_HOST_UPHOST_IP)
            hit.uphost.ip = atom.readAddress();
        else if (id == PCP_HOST_UPHOST_PORT)
            hit.uphost.port = atom.readInt();
        else if (id == PCP_HOST_UPHOST_HOPS)
            hit.uphostHops = atom.readInt();
        else
        {
            LOG_DEBUG("PCP skip: %s, %d, %d", id.getString().str(), c, d);
            atom.skip(c, d);
        }
    }

    hit.host = hit.rhost[0];
    hit.chanID = chanID;

    hit.numHops = bcs.numHops;

    if (hit.recv)
        chanMgr->addHit(hit);
    else
        chanMgr->delHit(hit);
}

// ------------------------------------------
void PCPStream::readChanAtoms(AtomStream &atom, int numc, BroadcastState &bcs)
{
    std::shared_ptr<Channel> ch = NULL;
    ChanInfo newInfo;

    ch = chanMgr->findChannelByID(bcs.chanID);
    auto chl = chanMgr->findHitListByID(bcs.chanID);

    if (ch)
        newInfo = ch->info;
    else if (chl)
        newInfo = chl->info;

    for (int i = 0; i < numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if ((id == PCP_CHAN_PKT) && (ch))
        {
            readPktAtoms(ch, atom, c, bcs);
        }else if (id == PCP_CHAN_INFO)
        {
            newInfo.readInfoAtoms(atom, c);
        }else if (id == PCP_CHAN_TRACK)
        {
            newInfo.readTrackAtoms(atom, c);
        }else if (id == PCP_CHAN_BCID)
        {
            atom.readBytes(newInfo.bcID.id, 16);
        }else if (id == PCP_CHAN_KEY)           // depreciated
        {
            atom.readBytes(newInfo.bcID.id, 16);
            newInfo.bcID.id[0] = 0;             // clear flags
        }else if (id == PCP_CHAN_ID)
        {
            atom.readBytes(newInfo.id.id, 16);

            ch = chanMgr->findChannelByID(newInfo.id);
            chl = chanMgr->findHitListByID(newInfo.id);
        }else
        {
            // IM50,51 対策。
            LOG_ERROR("PCP unknown or misplaced atom: %s, %d, %d", id.getString().str(), c, d);
            throw StreamException("Protocol error");

            //LOG_DEBUG("PCP skip: %s, %d, %d", id.getString().str(), c, d);
            //atom.skip(c, d);
        }
    }

    if (!chl)
        chl = chanMgr->addHitList(newInfo);

    if (chl)
    {
        chl->info.update(newInfo);

        if (!servMgr->chanLog.isEmpty())
        {
            try
            {
                FileStream file;
                file.openWriteAppend(servMgr->chanLog.cstr());
                XML::Node *rn = new XML::Node("update time=\"%u\"", sys->getTime());
                XML::Node *n = chl->info.createChannelXML();
                n->add(chl->createXML(false));
                n->add(chl->info.createTrackXML());
                rn->add(n);
                rn->write(file, 0);
                delete rn;
                file.close();
            }catch (StreamException &e)
            {
                LOG_ERROR("Unable to update channel log: %s", e.msg);
            }
        }
    }

    if (ch && !ch->isBroadcasting())
        ch->updateInfo(newInfo);
}

// ------------------------------------------
int PCPStream::readBroadcastAtoms(AtomStream &atom, int numc, BroadcastState &bcs)
{
    ChanPacket pack;
    int ttl=1;
    int ver=0;
    char ver_ex_prefix[3] = { '*', '*', '\0' };
    GnuID fromID, destID;

    fromID.clear();
    destID.clear();

    bcs.initPacketSettings();

    MemoryStream pmem(pack.data, sizeof(pack.data));
    AtomStream patom(pmem);

    patom.writeParent(PCP_BCST, numc);

    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);

        if (id == PCP_BCST_TTL)
        {
            ttl = atom.readChar()-1;
            patom.writeChar(id, ttl);
        }else if (id == PCP_BCST_HOPS)
        {
            bcs.numHops = atom.readChar()+1;
            patom.writeChar(id, bcs.numHops);
        }else if (id == PCP_BCST_FROM)
        {
            atom.readBytes(fromID.id, 16);
            patom.writeBytes(id, fromID.id, 16);

            routeList.add(fromID);
        }else if (id == PCP_BCST_GROUP)
        {
            bcs.group = atom.readChar();
            patom.writeChar(id, bcs.group);
        }else if (id == PCP_BCST_DEST)
        {
            atom.readBytes(destID.id, 16);
            patom.writeBytes(id, destID.id, 16);
            bcs.forMe = destID.isSame(servMgr->sessionID);

            char idstr1[64];
            char idstr2[64];

            destID.toStr(idstr1);
            servMgr->sessionID.toStr(idstr2);
        }else if (id == PCP_BCST_CHANID)
        {
            atom.readBytes(bcs.chanID.id, 16);
            patom.writeBytes(id, bcs.chanID.id, 16);
        }else if (id == PCP_BCST_VERSION)
        {
            ver = atom.readInt();
            patom.writeInt(id, ver);
            LOG_DEBUG("PCP bcst version %d", ver);
        } else if (id == PCP_BCST_VERSION_VP) {
            int ver_vp = atom.readInt();
            patom.writeInt(id, ver_vp);
            LOG_DEBUG("PCP bcst VP version %d", ver_vp);
        } else if (id == PCP_BCST_VERSION_EX_PREFIX) {
            atom.readBytes(ver_ex_prefix, 2);
            patom.writeBytes(id, ver_ex_prefix, 2);
        } else if (id == PCP_BCST_VERSION_EX_NUMBER) {
            short ver_ex_number = atom.readShort();
            patom.writeShort(id, ver_ex_number);
            LOG_DEBUG("PCP bcst ex version %s%hd", ver_ex_prefix, ver_ex_number);
        }else
        {
            // copy and process atoms
            int oldPos = pmem.pos;
            patom.writeAtoms(id, atom.io, c, d);
            pmem.pos = oldPos;
            readAtom(patom, bcs);
        }
    }

    LOG_DEBUG("PCP bcst: group=%d, hops=%d, ver=%d, from=%s, dest=%s", bcs.group, bcs.numHops, ver, fromID.str().c_str(), destID.str().c_str());
    if (fromID.isSet())
        if (fromID.isSame(servMgr->sessionID))
        {
            LOG_ERROR("BCST loopback");
            return PCP_ERROR_BCST+PCP_ERROR_LOOPBACK;
        }

    // broadcast back out if ttl > 0
    if ((ttl>0) && (!bcs.forMe))
    {
        pack.len = pmem.pos;
        pack.type = ChanPacket::T_PCP;

        if (bcs.group & (PCP_BCST_GROUP_ROOT|PCP_BCST_GROUP_TRACKERS|PCP_BCST_GROUP_RELAYS))
        {
            chanMgr->broadcastPacketUp(pack, bcs.chanID, remoteID, destID);
        }

        if (bcs.group & (PCP_BCST_GROUP_ROOT|PCP_BCST_GROUP_TRACKERS|PCP_BCST_GROUP_RELAYS))
        {
            servMgr->broadcastPacket(pack, bcs.chanID, remoteID, destID, Servent::T_COUT);
        }

        if (bcs.group & (PCP_BCST_GROUP_RELAYS|PCP_BCST_GROUP_TRACKERS))
        {
            servMgr->broadcastPacket(pack, bcs.chanID, remoteID, destID, Servent::T_CIN);
        }

        if (bcs.group & (PCP_BCST_GROUP_RELAYS))
        {
            servMgr->broadcastPacket(pack, bcs.chanID, remoteID, destID, Servent::T_RELAY);
        }
    }
    return 0;
}

// ------------------------------------------
int PCPStream::procAtom(AtomStream &atom, ID4 id, int numc, int dlen, BroadcastState &bcs)
{
    int r = 0;

    if (id == PCP_CHAN)
    {
        readChanAtoms(atom, numc, bcs);
    }else if (id == PCP_ROOT)
    {
        if (servMgr->isRoot)
            throw StreamException("Unauthorized root message");
        else
            readRootAtoms(atom, numc, bcs);
    }else if (id == PCP_HOST)
    {
        readHostAtoms(atom, numc, bcs);
    }else if ((id == PCP_MESG_ASCII) || (id == PCP_MESG))       // PCP_MESG_ASCII to be depreciated
    {
        String msg;
        atom.readString(msg.data, sizeof(msg.data), dlen);
        LOG_DEBUG("PCP got text: %s", msg.cstr());
    }else if (id == PCP_BCST)
    {
        r = readBroadcastAtoms(atom, numc, bcs);
    }else if (id == PCP_HELO)
    {
        atom.skip(numc, dlen);
        atom.writeParent(PCP_OLEH, 1);
            atom.writeBytes(PCP_HELO_SESSIONID, servMgr->sessionID.id, 16);
    }else if (id == PCP_PUSH)
    {
        readPushAtoms(atom, numc, bcs);
    }else if (id == PCP_OK)
    {
        atom.readInt();
    }else if (id == PCP_QUIT)
    {
        r = atom.readInt();
        if (!r)
            r = PCP_ERROR_QUIT;
    }else if (id == PCP_ATOM)
    {
        for (int i=0; i<numc; i++)
        {
            int nc, nd;
            ID4 aid = atom.read(nc, nd);
            int ar = procAtom(atom, aid, nc, nd, bcs);
            if (ar)
                r = ar;
        }
    }else
    {
        LOG_ERROR("PCP unknown or misplaced atom: %s", id.getString().str());
        throw StreamException("Protocol error");
        //LOG_INFO("PCP skip: %s", id.getString().str());
        //atom.skip(numc, dlen);
    }

    return r;
}

// ------------------------------------------
int PCPStream::readAtom(AtomStream &atom, BroadcastState &bcs)
{
    int numc, dlen;
    ID4 id = atom.read(numc, dlen);

    return  procAtom(atom, id, numc, dlen, bcs);
}
