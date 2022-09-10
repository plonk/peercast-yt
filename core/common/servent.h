// ------------------------------------------------
// File : servent.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _SERVENT_H
#define _SERVENT_H

// ----------------------------------
#include "socket.h"
#include "sys.h"
#include "channel.h"
#include "http.h"
#include "cookie.h"
#include "pcp.h"
#include "cgi.h" // Query
#include "playlist.h"
#include "varwriter.h"

class HTML;
class AtomStream;

// ----------------------------------
// Servent handles the actual connection between clients
class Servent : public VariableWriter
{
public:

    enum
    {
        MAX_HASH = 500,     // max. amount of packet hashes Servents can store
        MAX_OUTPACKETS = 32 // max. output packets per queue (normal/priority)
    };

    enum TYPE
    {
        T_NONE,             // Not allocated
        T_INCOMING,         // Unknown incoming
        T_SERVER,           // The main server
        T_RELAY,            // Outgoing relay
        T_DIRECT,           // Outgoing direct connection
        T_COUT,             // PCP out connection
        T_CIN,              // PCP in connection
    };

    enum STATUS
    {
        S_NONE,
        S_CONNECTING,
        S_PROTOCOL,
        S_HANDSHAKE,
        S_CONNECTED,
        S_CLOSING,
        S_LISTENING,
        S_TIMEOUT,
        S_REFUSED,
        S_VERIFIED,
        S_ERROR,
        S_WAIT,
        S_FREE
    };

    enum SORT
    {
        SORT_NAME = 0,
        SORT_BITRATE,
        SORT_LISTENERS,
        SORT_HOSTS,
        SORT_TYPE,
        SORT_GENRE
    };

    enum ALLOW
    {
        ALLOW_HTML      = 0x01,
        ALLOW_BROADCAST = 0x02,
        ALLOW_NETWORK   = 0x04,
        ALLOW_DIRECT    = 0x08,
        ALLOW_ALL       = 0xff
    };

    enum class StreamRequestDenialReason
    {
        None,
        InsufficientBandwidth,
        PerChannelRelayLimit,
        RelayLimit,
        DirectLimit,
        NotPlaying,
        Other,
    };

    static const char* denialReasonToName(StreamRequestDenialReason r)
        {
            switch (r)
            {
            case StreamRequestDenialReason::None:
                return "None";
            case StreamRequestDenialReason::InsufficientBandwidth:
                return "InsufficientBandwidth";
            case StreamRequestDenialReason::PerChannelRelayLimit:
                return "PerChannelRelayLimit";
            case StreamRequestDenialReason::RelayLimit:
                return "RelayLimit";
            case StreamRequestDenialReason::DirectLimit:
                return "DirectLimit";
            case StreamRequestDenialReason::NotPlaying:
                return "NotPlaying";
            case StreamRequestDenialReason::Other:
                return "Other";
            default:
                return "unknown";
            }
        }

    static const char* fileNameToMimeType(const String& fileName);

    Servent(int);
    ~Servent();

    void    reset();
    bool    initServer(Host &);
    void    initIncoming(std::shared_ptr<ClientSocket>, unsigned int);
    void    initOutgoing(TYPE);
    void    initGIV(const Host &, const GnuID &);
    void    initPCP(const Host &);

    void    checkFree();

    //  funcs for handling status/type
    void                setStatus(STATUS);
    static const char   *getTypeStr(Servent::TYPE t) { return typeMsgs[t]; }
    const char          *getTypeStr() { return getTypeStr(type); }
    const char          *getStatusStr() { return statusMsgs[status]; }
    int                 getOutput();
    void                addBytes(unsigned int);
    bool                isOlderThan(Servent *s)
    {
        if (s)
        {
            unsigned int t = sys->getTime();
            return ((t-lastConnect) > (t-s->lastConnect));
        }else
            return true;
    }

    // static funcs that do the actual work in the servent thread
    static THREAD_PROC  serverProc(ThreadInfo *);
    static THREAD_PROC  outgoingProc(ThreadInfo *);
    static THREAD_PROC  incomingProc(ThreadInfo *);
    static THREAD_PROC  givProc(ThreadInfo *);

    static bool pingHost(Host &, const GnuID &);

    std::string getLocalURL(const std::string& hostHeader);

    // various types of handshaking are needed
    void handshakePLS(ChanInfo &info, HTTP& http);
 
    void    handshakeHTML(char *);
    void    handshakeXML();
    void    handshakeCMD(HTTP&, const std::string&);
    bool    handshakeAuth(HTTP &, const char *);

    bool    handshakeHTTPBasicAuth(HTTP &http);

    bool    handshakeStream(ChanInfo &);
    void    handshakeStream_readHeaders(bool& gotPCP, unsigned int& reqPos, int& nsSwitchNum);
    void    handshakeStream_changeOutputProtocol(bool gotPCP, const ChanInfo& chanInfo);
    bool    handshakeStream_returnResponse(bool gotPCP, bool chanReady,
                                           std::shared_ptr<Channel> ch, ChanHitList* chl,
                                           const ChanInfo& chanInfo);
    void    handshakeStream_returnStreamHeaders(AtomStream& atom,
                                                std::shared_ptr<Channel> ch, const ChanInfo& chanInfo);
    void    handshakeStream_returnHits(AtomStream& atom, const GnuID& channelID, ChanHitList* chl, Host& rhost);
    void    handshakeGiv(const GnuID &);

    void    handshakeICY(Channel::SRC_TYPE, bool);
    void    handshakeIncoming();
    void    handshakeHTTP(HTTP &, bool);
    void    handshakeGET(HTTP &http);
    void    handshakePOST(HTTP &http);
    void    handshakeGIV(const char*);
    void    handshakeSOURCE(char * in, bool isHTTP);

    void    handshakeHTTPPush(const std::string& args);
    void    handshakeWMHTTPPush(HTTP& http, const std::string& path);

    void    handshakeJRPC(HTTP &http);

    void    handshakeLocalFile(const char *, HTTP& http);
    void    invokeCGIScript(HTTP &http, const char* fn);

    static void handshakeOutgoingPCP(AtomStream &, Host &, GnuID &, String &, bool);
    static void handshakeIncomingPCP(AtomStream &, Host &, GnuID &, String &);

    void    processIncomingPCP(bool);

    bool    waitForChannelHeader(ChanInfo &);
    ChanInfo findChannel(char *str, ChanInfo &);

    amf0::Value    getState() override;

    // the "mainloop" of servents
    void    processStream(ChanInfo &);

    void    triggerChannel(char *, ChanInfo::PROTOCOL, bool);
    void    sendRawChannel(bool, bool);
    void    sendRawMetaChannel(int);
    void    sendPCPChannel();

    static void readICYHeader(HTTP &, ChanInfo &, char *, size_t);
    bool    canStream(std::shared_ptr<Channel>, StreamRequestDenialReason *);

    bool    isConnected() { return status == S_CONNECTED; }
    bool    isListening() { return status == S_LISTENING; }

    bool    isAllowed(int);
    bool    isFiltered(int);

    // connection handling funcs
    void    createSocket();
    void    kill();
    void    abort();
    bool    isPrivate();
    bool    isLocal();

    Host    getHost();

    bool    acceptGIV(std::shared_ptr<ClientSocket>);
    bool    sendPacket(ChanPacket &, const GnuID &, const GnuID &, const GnuID &, Servent::TYPE);

    ChanInfo createChannelInfo(GnuID broadcastID, const String& broadcastMsg, cgi::Query& query, const std::string& contentType);

    static bool hasValidAuthToken(const std::string& requestFilename);
    static PlayList::TYPE playListType(ChanInfo &info);

    static std::string formatTimeDifference(unsigned int t, unsigned int currentTime);

    static bool isTerminationCandidate(ChanHit* hit);

    static void writeHeloAtom(AtomStream &atom, bool sendPort, bool sendPing, bool sendBCID, const GnuID& sessionID, uint16_t port, const GnuID& broadcastID);

    TYPE                type;
    std::atomic<STATUS> status;

    static const char   *statusMsgs[], *typeMsgs[];
    unsigned int        lastConnect, lastPing, lastPacket;
    String              agent;

    GnuID               networkID;
    const int           serventIndex;

    GnuID               remoteID;
    GnuID               chanID;
    GnuID               givID; // GIV するチャンネルのID

    ThreadInfo          thread;

    String              loginPassword, loginMount;

    bool                priorityConnect;
    bool                addMetadata;
    int                 nsSwitchNum;

    std::atomic<unsigned int> allow;

    std::shared_ptr<ClientSocket> sock, pushSock;

    std::recursive_mutex lock;

    bool                sendHeader;
    unsigned int        syncPos, streamPos;
    int                 servPort;

    ChanInfo::PROTOCOL  outputProtocol;

    Servent             *next;

    PCPStream           *pcpStream;
    Cookie              cookie;

private:
    void CMD_add_speedtest(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_apply(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_applyflags(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_bump(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_chanfeedlog(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_clear(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_clearlog(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_control_rtmp(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_delete_speedtest(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_dump_hitlists(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_fetch(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_fetch_feeds(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_keep(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_login(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_logout(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_redirect(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_portcheck4(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_portcheck6(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_refresh_speedtest(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_shutdown(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_speedtest_cached_xml(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_stop(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_stop_servent(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_take_speedtest(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_update_channel_info(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_viewxml(const char* cmd, HTTP& http, String& jumpStr);
    void CMD_customizeApperance(const char* cmd, HTTP& http, String& jumpStr);
};

extern const char *nextCGIarg(const char *cp, char *cmd, char *arg);

#endif

