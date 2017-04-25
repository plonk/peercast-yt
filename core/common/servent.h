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
#include "gnutella.h"
#include "channel.h"
#include "http.h"
#include "pcp.h"

class HTML;

class AtomStream;

// ----------------------------------
// Servent handles the actual connection between clients
class Servent
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
        T_PGNU              // old protocol connection
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

    // enum PROTOCOL
    // {
    //     P_UNKNOWN,
    //     P_GNUTELLA06,
    //     P_PCP
    // };

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

    Servent(int);
    ~Servent();

    void    reset();
    bool    initServer(Host &);
    void    initIncoming(ClientSocket *, unsigned int);
    void    initOutgoing(TYPE);
    void    initGIV(Host &, GnuID &);
    void    initPCP(Host &);

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
    static THREAD_PROC  pcpProc(ThreadInfo *);
    static THREAD_PROC  fetchProc(ThreadInfo *);

    static bool pingHost(Host &, GnuID &);

    bool    getLocalURL(char *);

    // various types of handshaking are needed
    void    handshakePLS(ChanHitList **, int, bool);
    void    handshakePLS(ChanInfo &, bool);

    void    handshakeHTML(char *);
    void    handshakeXML();
    void    handshakeCMD(char *);
    bool    handshakeAuth(HTTP &, const char *, bool);
    void    handshakeIn();
    void    handshakeOut();

    bool    handshakeHTTPBasicAuth(HTTP &http);

    void    processOutPCP();
    void    processOutChannel();

    bool    handshakeStream(ChanInfo &);
    void    handshakeGiv(GnuID &);

    void    handshakeICY(Channel::SRC_TYPE, bool);
    void    handshakeIncoming();
    void    handshakePOST();
    void    handshakeHTTP(HTTP &, bool);
    void    handshakeGET(HTTP &http);
    void    handshakePOST(HTTP &http);
    void    handshakeGIV(const char*);
    void    handshakeSOURCE(char * in, bool isHTTP);

    void    handshakeHTTPPush(const std::string& args);
    void    handshakeWMHTTPPush(HTTP& http, const std::string& path);

    void    handshakeJRPC(HTTP &http);

    void    handshakeRemoteFile(const char *);
    void    handshakeLocalFile(const char *);

    void    handshakePHP(HTML&, const char*, const char*);


    static void handshakeOutgoingPCP(AtomStream &, Host &, GnuID &, String &, bool);
    static void handshakeIncomingPCP(AtomStream &, Host &, GnuID &, String &);

    void    processIncomingPCP(bool);

    bool    waitForChannelHeader(ChanInfo &);
    ChanInfo findChannel(char *str, ChanInfo &);

    bool    writeVariable(Stream &, const String &);


    // the "mainloop" of servents
    void    processGnutella();
    void    processRoot();
    void    processServent();
    void    processStream(bool, ChanInfo &);
    void    processPCP(bool, bool);

    bool    procAtoms(AtomStream &);
    void    procRootAtoms(AtomStream &, int);
    void    procHeloAtoms(AtomStream &, int, bool);
    void    procGetAtoms(AtomStream &, int);

    void    triggerChannel(char *, ChanInfo::PROTOCOL, bool);
    void    sendPeercastChannel();
    void    sendRawChannel(bool, bool);
//  void    sendRawMultiChannel(bool, bool);
    void    sendRawMetaChannel(int);
    void    sendPCPChannel();
    void    checkPCPComms(Channel *, AtomStream &);

    static void readICYHeader(HTTP &, ChanInfo &, char *, size_t);
    bool    canStream(Channel *);

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

    bool    outputPacket(GnuPacket &, bool);
    bool    hasSeenPacket(GnuPacket &p) { return seenIDs.contains(p.id); }
    bool    acceptGIV(ClientSocket *);
    bool    sendPacket(ChanPacket &, GnuID &, GnuID &, GnuID &, Servent::TYPE);


    TYPE                type;
    STATUS              status;

    static const char   *statusMsgs[], *typeMsgs[];
    GnuStream           gnuStream;
    GnuPacket           pack;
    unsigned int        lastConnect, lastPing, lastPacket;
    String              agent;

    GnuIDList           seenIDs;
    GnuID               networkID;
    const int           serventIndex;

    GnuID               remoteID;
    GnuID               chanID;
    GnuID               givID;

    ThreadInfo          thread;

    String              loginPassword, loginMount;

    bool                priorityConnect;
    bool                addMetadata;
    int                 nsSwitchNum;

    unsigned int        allow;

    ClientSocket        *sock, *pushSock;

    WLock               lock;

    bool                sendHeader;
    unsigned int        syncPos, streamPos;
    int                 servPort;

    ChanInfo::PROTOCOL  outputProtocol;

    GnuPacketBuffer     outPacketsNorm, outPacketsPri;

    bool                flowControl;

    Servent             *next;

    PCPStream           *pcpStream;
    Cookie              cookie;

private:
    void CMD_redirect(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_viewxml(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_clearlog(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_save(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_reg(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_edit_bcid(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_add_bcid(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_apply(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_fetch(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_stopserv(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_hitlist(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_clear(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_upgrade(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_connect(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_shutdown(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_stop(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_bump(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_keep(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_relay(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_net_add(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_logout(char *cmd, HTTP& http, HTML& html, char jumpStr[]);
    void CMD_login(char *cmd, HTTP& http, HTML& html, char jumpStr[]);

};

extern char *nextCGIarg(char *cp, char *cmd, char *arg);


#endif

