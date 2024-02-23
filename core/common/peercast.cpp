#include "sys.h"
#include "peercast.h"
#include "channel.h"
#include "servmgr.h"
#include "str.h"
#include "sslclientsocket.h"
#include "yplist.h"

// ---------------------------------
// globals

Sys *sys = nullptr;
ChanMgr *chanMgr;
ServMgr *servMgr;

PeercastInstance *peercastInst = nullptr;
PeercastApplication *peercastApp = nullptr;

// ---------------------------------
void APICALL PeercastInstance::init()
{
    sys = createSys();
    servMgr = new ServMgr();
    chanMgr = new ChanMgr();
    g_ypList = new YPList();

    if (peercastApp->getIniFilename())
        servMgr->loadSettings(peercastApp->getIniFilename());

    SslClientSocket::configureServer(str::STR(peercastApp->getSettingsDirPath(), "/server.crt"),
                                     str::STR(peercastApp->getSettingsDirPath(), "/server.key"));

    servMgr->loadTokenList();

    servMgr->start();
}

// --------------------------------------------------
void    APICALL PeercastInstance::setNotifyMask(int mask)
{
    if (servMgr)
        servMgr->notifyMask = mask;
}

// --------------------------------------------------
int     APICALL PeercastInstance::getNotifyMask()
{
    if (servMgr)
        return servMgr->notifyMask;
    else
        return 0;
}

// --------------------------------------------------
void    APICALL PeercastInstance::setAutoConnect(bool on)
{
    if (servMgr)
        servMgr->autoConnect = on;
}

// --------------------------------------------------
bool    APICALL PeercastInstance::getAutoConnect()
{
    if (servMgr)
        return servMgr->autoConnect;
    else
        return false;
}

// --------------------------------------------------
void    APICALL PeercastInstance::setMaxOutput(int kbps)
{
    if (servMgr)
        servMgr->maxBitrateOut = kbps;
}
// --------------------------------------------------
int     APICALL PeercastInstance::getMaxOutput()
{
    if (servMgr)
        return servMgr->maxBitrateOut;
    else
        return 0;
}

// --------------------------------------------------
void    APICALL PeercastInstance::setMaxRelays(int max)
{
    if (servMgr)
        servMgr->setMaxRelays(max);
}

// --------------------------------------------------
int     APICALL PeercastInstance::getMaxRelays()
{
    if (servMgr)
        return servMgr->maxRelays;
    else
        return 0;
}

// --------------------------------------------------
void    APICALL PeercastInstance::setActive(bool on)
{
    if (servMgr)
    {
        servMgr->autoConnect = on;
        servMgr->autoServe = on;
    }
}

// --------------------------------------------------
bool    APICALL PeercastInstance::getActive()
{
    if (servMgr)
        return servMgr->autoConnect&&servMgr->autoServe;
    else
        return false;
}

// --------------------------------------------------
void    APICALL PeercastInstance::saveSettings()
{
    if (servMgr)
        servMgr->saveSettings(peercastApp->getIniFilename());
}

// --------------------------------------------------
void    APICALL PeercastInstance::quit()
{
    isQuitting = true;
    if (chanMgr)
        chanMgr->quit();
    if (servMgr)
        servMgr->quit();
    // Give threads time to run all the deconstructors.
    sys->sleep(1000);
}

// --------------------------------------------------
void    APICALL PeercastInstance::setServerPort(int port)
{
    if (servMgr)
    {
        servMgr->serverHost.port = port;
        servMgr->restartServer = true;
    }
}

// --------------------------------------------------
int     APICALL PeercastInstance::getServerPort()
{
    if (servMgr)
        return servMgr->serverHost.port;
    else
        return 0;
}

// --------------------------------------------------
void    APICALL PeercastInstance::setServerPassword(const char *pwd)
{
    if (servMgr)
        Sys::strcpy_truncate(servMgr->password, sizeof(servMgr->password), pwd);
}

// --------------------------------------------------
const char *APICALL PeercastInstance::getServerPassword()
{
    return servMgr->password;
}

// --------------------------------------------------
void    APICALL PeercastInstance::callLocalURL(const char *url)
{
    if (sys && servMgr)
        sys->callLocalURL(url, servMgr->serverHost.port);
}

// --------------------------------------------------
namespace peercast
{

std::string log_escape(const std::string& str)
{
    std::string result;

    for (auto it = str.cbegin(); it != str.cend(); ++it) {
        unsigned char c = *it;
        if (c >= 0x20 && c <= 0x7E) { // ascii printables
            result.push_back(*it);
        } else {
            char tmp[5];

            std::snprintf(tmp, sizeof(tmp), "[%02X]", c);
            result += tmp;
        }
    }
    return result;
}

}

// --------------------------------------------------
thread_local std::vector<std::function<void(LogBuffer::TYPE type, const char*)>>* AUX_LOG_FUNC_VECTOR = nullptr;
void ADDLOG(const char *fmt, va_list ap, LogBuffer::TYPE type)
{
    // ガード。
    if (!servMgr) return;
    if (servMgr->pauseLog) return;
    if (!sys) return;

    // 1024バイトに切り詰める。[バグ]std::stringクラスを使っているので、
    // シグナルハンドラーからのログ出力に使われるとメモリアロケーター
    // を危険に使用する。
    const int MAX_LINELEN = 1024;
    std::string tmp = str::vformat(fmt, ap);

    // スレッドIDを前置。
    std::string tname = sys->getThreadName();
    if (tname.empty()) {
        tmp = str::format("[%s] %s",
                          sys->getThreadIdString().c_str(),
                          tmp.c_str());
    } else {
        tmp = str::format("[%-15s(%s)] %s",
                          tname.c_str(),
                          sys->getThreadIdString().c_str(),
                          tmp.c_str());
    }

    if (str::validate_utf8(tmp)) {
        tmp = str::truncate_utf8(tmp, MAX_LINELEN);
    } else {
        tmp = peercast::log_escape(tmp);
        tmp = str::truncate_utf8(tmp, MAX_LINELEN);
    }

    const char* str = tmp.c_str();

    // ログレベルに関わらず出力する。
    if (AUX_LOG_FUNC_VECTOR) {
        for (auto func : *AUX_LOG_FUNC_VECTOR) {
            func(type, str);
        }
    }

    if (servMgr->logLevel() > type) return;

    if (type != LogBuffer::T_NONE)
        sys->logBuf->write(str, type);

    peercastApp->printLog(type, str);
}

// --------------------------------------------------
void LOG(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_DEBUG);
    va_end(ap);
}

// --------------------------------------------------
void LOG_TRACE(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_TRACE);
    va_end(ap);
}

// --------------------------------------------------
void LOG_DEBUG(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_DEBUG);
    va_end(ap);
}

// --------------------------------------------------
void LOG_INFO(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_INFO);
    va_end(ap);
}

// --------------------------------------------------
void LOG_WARN(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_WARN);
    va_end(ap);
}

// --------------------------------------------------
void LOG_ERROR(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_ERROR);
    va_end(ap);
}

// --------------------------------------------------
void LOG_FATAL(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ADDLOG(fmt, ap, LogBuffer::T_FATAL);
    va_end(ap);
}

// --------------------------------------------------
#include "notif.h"

namespace peercast {

void notifyMessage(ServMgr::NOTIFY_TYPE type, const std::string& message)
{
    Notification notif(sys->getTime(), type, message);
    g_notificationBuffer.addNotification(notif);
    peercastApp->notifyMessage(type, message.c_str());
}

} // namespace peercast

// --------------------------------------------------
amf0::Value PeercastApplication::getState()
{
    return amf0::Value::object({
            { "path", getPath() },
            { "settingsDirPath", getSettingsDirPath() },
            { "iniFilename", getIniFilename() },
            { "tokenListFilename", getTokenListFilename() },
            { "cacheDirPath", getCacheDirPath() },
            { "stateDirPath", getStateDirPath() },
        });
}
