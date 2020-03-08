// ------------------------------------------------
// File : channel.h
// Date: 4-apr-2002
// Author: giles
//
// (c) 2002 peercast.org
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

#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "sys.h"
#include "stream.h"
#include "xml.h"
#include "asf.h"
#include "cstream.h"
#include "varwriter.h"

// --------------------------------------------------
struct MP3Header
{
    int lay;
    int version;
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int stereo;
};

// ----------------------------------
#include "chaninfo.h"
#include "chanhit.h"

// ----------------------------------
class ChanMeta
{
public:
    enum
    {
        MAX_DATALEN = 65536
    };

    void    init()
    {
        len = 0;
        cnt = 0;
        startPos = 0;
    }

    void    fromXML(XML &);
    void    fromMem(void *, int);
    void    addMem(void *, int);

    unsigned int    len, cnt, startPos;
    char            data[MAX_DATALEN];
};

// ------------------------------------------
class RawStream : public ChannelStream
{
public:
    void readHeader(Stream &, std::shared_ptr<Channel>) override;
    int  readPacket(Stream &, std::shared_ptr<Channel>) override;
    void readEnd(Stream &, std::shared_ptr<Channel>) override;
};

// ------------------------------------------
class PeercastStream : public ChannelStream
{
public:
    void readHeader(Stream &, std::shared_ptr<Channel>) override;
    int  readPacket(Stream &, std::shared_ptr<Channel>) override;
    void readEnd(Stream &, std::shared_ptr<Channel>) override;
};

// ------------------------------------------
class ChannelSource
{
public:
    virtual ~ChannelSource() {}

    virtual void stream(std::shared_ptr<Channel>) = 0;
    virtual int getSourceRate() { return 0; }
    virtual int getSourceRateAvg() { return this->getSourceRate(); }
};

// ------------------------------------------
class PeercastSource : public ChannelSource
{
public:

    PeercastSource() : m_channel(NULL) {}
    void    stream(std::shared_ptr<Channel>) override;
    int     getSourceRate() override;
    int     getSourceRateAvg() override;
    ChanHit pickFromHitList(std::shared_ptr<Channel> ch, ChanHit &oldHit);

    std::shared_ptr<Channel> m_channel;
};

// ----------------------------------
class Channel : public VariableWriter,
                public std::enable_shared_from_this<Channel>
{
public:

    enum STATUS
    {
        S_NONE,
        S_WAIT,
        S_CONNECTING,
        S_REQUESTING,
        S_CLOSING,
        S_RECEIVING,
        S_BROADCASTING,
        S_ABORT,
        S_SEARCHING,
        S_NOHOSTS,
        S_IDLE,
        S_ERROR,
        S_NOTFOUND
    };

    enum TYPE
    {
        T_NONE,
        T_ALLOCATED,
        T_BROADCAST,
        T_RELAY
    };

    enum SRC_TYPE
    {
        SRC_NONE,
        SRC_PEERCAST,
        SRC_SHOUTCAST,
        SRC_ICECAST,
        SRC_URL,
        SRC_HTTPPUSH
    };

    Channel();
    void    reset();
    void    endThread();

    void    startMP3File(char *);
    void    startGet();
    void    startICY(ClientSocket *, SRC_TYPE);
    void    startURL(const char *);
    void    startHTTPPush(ClientSocket *, bool isChunked);
    void    startWMHTTPPush(ClientSocket *cs);

    ChannelStream   *createSource();

    void    resetPlayTime();

    bool    notFound()
    {
        return (status == S_NOTFOUND);
    }

    bool    isPlaying()
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        return (status == S_RECEIVING) || (status == S_BROADCASTING);
    }

    bool    isReceiving()
    {
        return (status == S_RECEIVING);
    }

    bool    isBroadcasting()
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        return (status == S_BROADCASTING);
    }

    virtual bool isFull();
    bool    canAddRelay();

    bool    checkBump();

    bool    checkIdle();
    void    sleepUntil(double);

    bool    isActive()
    {
        return type != T_NONE;
    }

    void    connectFetch();
    int     handshakeFetch();

    bool    isIdle() { return isActive() && (status==S_IDLE); }

    static THREAD_PROC stream(ThreadInfo *);

    void         setStatus(STATUS s);
    const char   *getSrcTypeStr() { return srcTypes[srcType]; }
    const char   *getStatusStr() { return statusMsgs[status]; }
    const char   *getName() { return info.name.cstr(); }
    GnuID        getID() { return info.id; }
    int          getBitrate() { return info.bitrate; }
    std::string  getSourceString();
    std::string  getBufferString();

    void         broadcastTrackerUpdate(const GnuID &, bool = false);
    bool         sendPacketUp(ChanPacket &, const GnuID &, const GnuID &, const GnuID &);

    bool         writeVariable(Stream &, const String &) override;
    bool         acceptGIV(ClientSocket *);
    void         updateInfo(const ChanInfo &);
    int          readStream(Stream &, ChannelStream *);
    void         checkReadDelay(unsigned int);
    void         processMp3Metadata(char *);
    void         readHeader();
    void         startStream();

    XML::Node    *createRelayXML(bool);

    void         newPacket(ChanPacket &);

    int          localListeners();
    int          localRelays();

    int          totalListeners();
    int          totalRelays();

    std::string  authSecret();
    static std::string renderHexDump(const std::string& in);

    ::String            mount;
    ChanMeta            insertMeta;
    ChanPacket          headPack;

    ChanPacketBuffer    rawData;

    ChannelStream       *sourceStream;
    unsigned int        streamIndex;

    ChanInfo            info;
    ChanHit             sourceHost;
    ChanHit             designatedHost;

    GnuID               remoteID;

    ::String            sourceURL;

    bool                bump, stayConnected;
    int                 icyMetaInterval;
    unsigned int        streamPos;
    bool                readDelay;

    TYPE                type;
    ChannelSource       *sourceData;

    SRC_TYPE            srcType;

    MP3Header           mp3Head;
    ThreadInfo          thread;

    unsigned int        lastIdleTime;
    STATUS              status;
    static const char   *statusMsgs[], *srcTypes[];

    ClientSocket        *sock;
    ClientSocket        *pushSock;

    unsigned int        lastTrackerUpdate;
    unsigned int        lastMetaUpdate;

    double              startTime, syncTime;

    WEvent              syncEvent;

    mutable std::recursive_mutex lock;

    std::shared_ptr<Channel> next;
};

#endif
