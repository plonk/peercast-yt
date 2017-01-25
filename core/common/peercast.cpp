#include "sys.h"
#include "peercast.h"
#include "channel.h"
#include "servmgr.h"



// ---------------------------------
// globals

Sys *sys=NULL;
ChanMgr *chanMgr;
ServMgr *servMgr;

PeercastInstance *peercastInst=NULL;
PeercastApplication *peercastApp=NULL;


// ---------------------------------
void APICALL PeercastInstance::init()
{
	sys = createSys();
	servMgr = new ServMgr();
	chanMgr = new ChanMgr();

	if (peercastApp->getIniFilename())
		servMgr->loadSettings(peercastApp->getIniFilename());

	servMgr->start();
}
// --------------------------------------------------
void	APICALL PeercastInstance::setNotifyMask(int mask)
{
	if (servMgr)
		servMgr->notifyMask = mask;
}
// --------------------------------------------------
int		APICALL PeercastInstance::getNotifyMask()
{
	if (servMgr)
		return servMgr->notifyMask;
	else	
		return 0;
}

// --------------------------------------------------
void	APICALL PeercastInstance::setAutoConnect(bool on)
{
	if (servMgr)
		servMgr->autoConnect = on;
}
// --------------------------------------------------
bool	APICALL PeercastInstance::getAutoConnect()
{
	if (servMgr)
		return servMgr->autoConnect;
	else
		return false;
}
// --------------------------------------------------
void	APICALL PeercastInstance::setMaxOutput(int kbps)
{
	if (servMgr)
		servMgr->maxBitrateOut = kbps;
}
// --------------------------------------------------
int		APICALL PeercastInstance::getMaxOutput()
{
	if (servMgr)
		return servMgr->maxBitrateOut;
	else
		return 0;
}
// --------------------------------------------------
void	APICALL PeercastInstance::setMaxRelays(int max)
{
	if (servMgr)
		servMgr->setMaxRelays(max);
}
// --------------------------------------------------
int		APICALL PeercastInstance::getMaxRelays()
{
	if (servMgr)
		return servMgr->maxRelays;
	else
		return 0;
}
// --------------------------------------------------
void	APICALL PeercastInstance::setActive(bool on)
{
	if (servMgr)
	{
		servMgr->autoConnect = on;
		servMgr->autoServe = on;
	}
}
// --------------------------------------------------
bool	APICALL PeercastInstance::getActive()
{
	if (servMgr)
		return servMgr->autoConnect&&servMgr->autoServe;
	else
		return false;
}
// --------------------------------------------------
void	APICALL PeercastInstance::saveSettings()
{
	if (servMgr)
		servMgr->saveSettings(peercastApp->getIniFilename());
}
// --------------------------------------------------
void	APICALL PeercastInstance::quit()
{
	isQuitting = true;
	if (chanMgr)
		chanMgr->quit();
	if (servMgr)
		servMgr->quit();
}
// --------------------------------------------------
void	APICALL	PeercastInstance::setServerPort(int port)
{
	if (servMgr)
	{
		servMgr->serverHost.port = port;
		servMgr->restartServer = true;
	}
}
// --------------------------------------------------
int		APICALL	PeercastInstance::getServerPort()
{
	if (servMgr)
		return servMgr->serverHost.port;
	else
		return 0;
}
// --------------------------------------------------
void	APICALL	PeercastInstance::setServerPassword(const char *pwd)
{
	if (servMgr)
		strcpy(servMgr->password,pwd);
}
// --------------------------------------------------
const char *APICALL	PeercastInstance::getServerPassword()
{
	return servMgr->password;
}
// --------------------------------------------------
void	APICALL PeercastInstance::callLocalURL(const char *url)
{
	if (sys && servMgr)
		sys->callLocalURL(url,servMgr->serverHost.port);
}


// --------------------------------------------------
void ADDLOG(const char *fmt,va_list ap,LogBuffer::TYPE type)
{
	if(sys)
	{
		const int MAX_LINELEN = 1024;

		char str[MAX_LINELEN+1];
		vsnprintf(str,MAX_LINELEN-1,fmt,ap);
		str[MAX_LINELEN-1]=0;

		if (type != LogBuffer::T_NONE)
			sys->logBuf->write(str,type);

		peercastApp->printLog(type,str);
	}
}
// --------------------------------------------------
void LOG(const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	ADDLOG(fmt,ap,LogBuffer::T_DEBUG);
   	va_end(ap);	
}

// --------------------------------------------------
void LOG_ERROR(const char *fmt,...)
{
	if (servMgr)
	{
		if ((servMgr->showLog & (1<<LogBuffer::T_ERROR)) && (!servMgr->pauseLog))
		{
			va_list ap;
  			va_start(ap, fmt);
			ADDLOG(fmt,ap,LogBuffer::T_ERROR);
   			va_end(ap);	
		}
	}
}
// --------------------------------------------------
void LOG_DEBUG(const char *fmt,...)
{
	if (servMgr)
	{
		if ((servMgr->showLog & (1<<LogBuffer::T_DEBUG)) && (!servMgr->pauseLog))
		{
			va_list ap;
  			va_start(ap, fmt);
			ADDLOG(fmt,ap,LogBuffer::T_DEBUG);
   			va_end(ap);	
		}
	}
}
// --------------------------------------------------
void LOG_NETWORK(const char *fmt,...)
{
	if (servMgr)
	{
		if ((servMgr->showLog & (1<<LogBuffer::T_NETWORK)) && (!servMgr->pauseLog))
		{
			va_list ap;
  			va_start(ap, fmt);
			ADDLOG(fmt,ap,LogBuffer::T_NETWORK);
   			va_end(ap);	
		}
	}
}
// --------------------------------------------------
void LOG_CHANNEL(const char *fmt,...)
{
	if (servMgr)
	{
		if ((servMgr->showLog & (1<<LogBuffer::T_CHANNEL)) && (!servMgr->pauseLog))
		{
			va_list ap;
  			va_start(ap, fmt);
			ADDLOG(fmt,ap,LogBuffer::T_CHANNEL);
   			va_end(ap);	
		}
	}
}
