#ifndef _PEERCAST_H
#define _PEERCAST_H

class ChanInfo;
class Sys;

#define APICALL

#include "servmgr.h"
#include "logbuf.h"

// ------------------------------------------------------------
// This is the interface from the application to the core.
class PeercastInstance
{
public:
    PeercastInstance()
    :isQuitting(false)
    {}

    virtual void    APICALL init();

    virtual void    APICALL setAutoConnect(bool);
    virtual bool    APICALL getAutoConnect();

    virtual void    APICALL setActive(bool);
    virtual bool    APICALL getActive();

    virtual int     APICALL getMaxOutput();
    virtual void    APICALL setMaxOutput(int);

    virtual int     APICALL getMaxRelays();
    virtual void    APICALL setMaxRelays(int);

    virtual void    APICALL setServerPort(int);
    virtual int     APICALL getServerPort();
    virtual void    APICALL setServerPassword(const char *);
    virtual const char *APICALL getServerPassword();

    virtual void    APICALL saveSettings();

    virtual void    APICALL callLocalURL(const char *);

    virtual void    APICALL setNotifyMask(int);
    virtual int     APICALL getNotifyMask();

    virtual void    APICALL quit();

    virtual Sys *   APICALL createSys()=0;

    std::atomic_bool isQuitting;
};

// ------------------------------------------------------------
// This is the interface from the core to the application.
class PeercastApplication
{
public:

    virtual const char * APICALL getPath() { return "./"; }
    virtual const char * APICALL getIniFilename() { return "peercast.ini"; }
    virtual const char * APICALL getTokenListFilename() { return "tokens.json"; }
    virtual void    APICALL printLog(LogBuffer::TYPE, const char *) {}
    virtual void    APICALL addChannel(ChanInfo *) {}
    virtual void    APICALL delChannel(ChanInfo *) {}
    virtual void    APICALL resetChannels() {}
    virtual void    APICALL test(char *) {}
    virtual const char *APICALL getClientTypeOS() = 0;
    virtual void    APICALL updateSettings() {}

    virtual void    APICALL notifyMessage(ServMgr::NOTIFY_TYPE, const char *) {}
    virtual void    APICALL channelStart(ChanInfo *) {}
    virtual void    APICALL channelStop(ChanInfo *) {}
    virtual void    APICALL channelUpdate(ChanInfo *) {}
};

// ----------------------------------
extern PeercastInstance *peercastInst;
extern PeercastApplication *peercastApp;

// ----------------------------------
#ifdef WIN32
extern "C"
{
__declspec( dllexport ) PeercastInstance * newPeercast(PeercastApplication *);
};
#endif

// ----------------------------------
namespace peercast {
void notifyMessage(ServMgr::NOTIFY_TYPE, const std::string&);
}

#endif
