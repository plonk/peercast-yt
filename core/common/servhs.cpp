// ------------------------------------------------
// File : servhs.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Servent handshaking, TODO: should be in its own class
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


#include <stdlib.h>
#include "servent.h"
#include "servmgr.h"
#include "html.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "version2.h"

// -----------------------------------
static void termArgs(char *str)
{
	if (str)
	{
		int slen = strlen(str);
		for(int i=0; i<slen; i++)
			if (str[i]=='&') str[i] = 0;
	}
}
// -----------------------------------
char *nextCGIarg(char *cp, char *cmd, char *arg)
{
	if (!*cp)
		return NULL;

	int cnt=0;

	// fetch command
	while (*cp)
	{
		char c = *cp++;
		if (c == '=')
			break;
		else
			*cmd++ = c;

		cnt++;
		if (cnt >= (MAX_CGI_LEN-1))
			break;
	}
	*cmd = 0;

	cnt=0;
	// fetch arg
	while (*cp)
	{
		char c = *cp++;
		if (c == '&')
			break;
		else
			*arg++ = c;

		cnt++;
		if (cnt >= (MAX_CGI_LEN-1))
			break;
	}
	*arg = 0;

	return cp;
}
// -----------------------------------
bool getCGIargBOOL(char *a)
{
	return (strcmp(a,"1")==0);
}
// -----------------------------------
int getCGIargINT(char *a)
{
	return atoi(a);
}

// -----------------------------------
void Servent::handshakeHTTP(HTTP &http, bool isHTTP)
{
	char *in = http.cmdLine;

	if (http.isRequest("GET /"))
	{
		char *fn = in+4;

		char *pt = strstr(fn,HTTP_PROTO1);
		if (pt)
			pt[-1] = 0;

		if (strncmp(fn,"/admin?",7)==0)
		{
			if (!isAllowed(ALLOW_HTML))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);


			LOG_DEBUG("Admin client");
			handshakeCMD(fn+7);

		}else if (strncmp(fn,"/http/",6)==0)
		{
			String dirName = fn+6;

			if (!isAllowed(ALLOW_HTML))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);			

			if (!handshakeAuth(http,fn,false))
				throw HTTPException(HTTP_SC_UNAUTHORIZED,401);			


			handshakeRemoteFile(dirName);


		}else if (strncmp(fn,"/html/",6)==0)
		{
			String dirName = fn+1;

			if (!isAllowed(ALLOW_HTML))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			if (handshakeAuth(http,fn,true))
				handshakeLocalFile(dirName);


		}else if (strncmp(fn,"/admin/?",8)==0)
		{
			if (!isAllowed(ALLOW_HTML))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			LOG_DEBUG("Admin client");
			handshakeCMD(fn+8);

		}else if (strncmp(fn,"/admin.cgi",10)==0)
		{
			if (!isAllowed(ALLOW_BROADCAST))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			char *pwdArg = getCGIarg(fn,"pass=");
			char *songArg = getCGIarg(fn,"song=");
			char *mountArg = getCGIarg(fn,"mount=");
			char *urlArg = getCGIarg(fn,"url=");

			if (pwdArg && songArg)
			{
				int i;
				int slen = strlen(fn);
				for(i=0; i<slen; i++)
					if (fn[i]=='&') fn[i] = 0;

				Channel *c=chanMgr->channel;
				while (c)
				{
					if ((c->status == Channel::S_BROADCASTING) &&
					   (c->info.contentType == ChanInfo::T_MP3) )
					{
						// if we have a mount point then check for it, otherwise update all channels.

						bool match=true;

						if (mountArg)
							match = strcmp(c->mount,mountArg)==0;

						if (match)
						{
							ChanInfo newInfo = c->info;
							newInfo.track.title.set(songArg,String::T_ESC);
							newInfo.track.title.convertTo(String::T_UNICODE);

							if (urlArg)
								if (urlArg[0])
									newInfo.track.contact.set(urlArg,String::T_ESC);
							LOG_CHANNEL("Channel Shoutcast update: %s",songArg);
							c->updateInfo(newInfo);
						}
					}
					c=c->next;
				}
			}

		}else if (strncmp(fn,"/pls/",5)==0)
		{

			if (!sock->host.isLocalhost())
				if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
					throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			
			ChanInfo info;
			if (servMgr->getChannel(fn+5,info,isPrivate()))
				handshakePLS(info,false);
			else
				throw HTTPException(HTTP_SC_NOTFOUND,404);

		}else if (strncmp(fn,"/stream/",8)==0)
		{

			if (!sock->host.isLocalhost())
				if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
					throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			triggerChannel(fn+8,ChanInfo::SP_HTTP,isPrivate());

			
		}else if (strncmp(fn,"/channel/",9)==0)
		{

			if (!sock->host.isLocalhost())
				if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
					throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			triggerChannel(fn+9,ChanInfo::SP_PCP,false);

		}else 
		{
			while (http.nextHeader());
			http.writeLine(HTTP_SC_FOUND);
			http.writeLineF("Location: /%s/index.html",servMgr->htmlPath);
			http.writeLine("");
		}
	}else if (http.isRequest("GIV"))
	{
		HTTP http(*sock);
	
		while (http.nextHeader()) ;

		if (!isAllowed(ALLOW_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		GnuID id;
		id.clear();

		char *idstr = strstr(in,"/");
		if (idstr)
			id.fromStr(idstr+1);

		char ipstr[64];
		sock->host.toStr(ipstr);

		if (id.isSet())
		{
			// at the moment we don`t really care where the GIV came from, so just give to chan. no. if its waiting.
			Channel *ch = chanMgr->findChannelByID(id);

			if (!ch)
				throw HTTPException(HTTP_SC_NOTFOUND,404);

			if (!ch->acceptGIV(sock))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		
			LOG_DEBUG("Accepted GIV channel %s from: %s",idstr,ipstr);

			sock=NULL;					// release this servent but dont close socket.	
		}else 
		{

			if (!servMgr->acceptGIV(sock))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			LOG_DEBUG("Accepted GIV PCP from: %s",ipstr);
			sock=NULL;					// release this servent but dont close socket.	
		}

	}else if (http.isRequest(PCX_PCP_CONNECT))
	{

		if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);
		
		processIncomingPCP(true);

	}else if (http.isRequest("PEERCAST CONNECT"))
	{
		if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		LOG_DEBUG("PEERCAST client");
		processServent();

	}else if (http.isRequest("SOURCE"))
	{
		if (!isAllowed(ALLOW_BROADCAST))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		char *mount = NULL;

		char *ps;
		if (ps=strstr(in,"ICE/1.0"))
		{
			mount = in+7;
			*ps = 0;
			LOG_DEBUG("ICE 1.0 client to %s",mount?mount:"unknown");
		}else{
			mount = in+strlen(in);
			while (*--mount)
				if (*mount == '/')
				{
					mount[-1] = 0; // password preceeds
					break;
				}
			loginPassword.set(in+7);
			
			LOG_DEBUG("ICY client: %s %s",loginPassword.cstr(),mount?mount:"unknown");
		}

		if (mount)
			loginMount.set(mount);

		handshakeICY(Channel::SRC_ICECAST,isHTTP);
		sock = NULL;	// socket is taken over by channel, so don`t close it

	}else if (http.isRequest(servMgr->password))
	{
		if (!isAllowed(ALLOW_BROADCAST))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		loginPassword.set(servMgr->password);	// pwd already checked

		sock->writeLine("OK2");
		sock->writeLine("icy-caps:11");
		sock->writeLine("");
		LOG_DEBUG("ShoutCast client");

		handshakeICY(Channel::SRC_SHOUTCAST,isHTTP);
		sock = NULL;	// socket is taken over by channel, so don`t close it

	}else
	{
		throw HTTPException(HTTP_SC_BADREQUEST,400);
	}

}
// -----------------------------------
bool Servent::canStream(Channel *ch)
{
	if (ch==NULL)
		return false;

	if (servMgr->isDisabled)
		return false;

	if (!isPrivate())
	{
		if  (
				servMgr->bitrateFull(ch->getBitrate())
				|| ((type == T_RELAY) && servMgr->relaysFull())
				|| ((type == T_DIRECT) && servMgr->directFull())
				|| !ch->isPlaying()
				|| ch->isFull()
			)
			return false;
	}

	return true;
}
// -----------------------------------
void Servent::handshakeIncoming()
{

	setStatus(S_HANDSHAKE);

	char buf[1024];
	sock->readLine(buf,sizeof(buf));

	char sb[64];
	sock->host.toStr(sb);


	if (stristr(buf,RTSP_PROTO1))
	{
		LOG_DEBUG("RTSP from %s '%s'",sb,buf);
		RTSP rtsp(*sock);
		rtsp.initRequest(buf);
		handshakeRTSP(rtsp);
	}else if (stristr(buf,HTTP_PROTO1))
	{
		LOG_DEBUG("HTTP from %s '%s'",sb,buf);
		HTTP http(*sock);
		http.initRequest(buf);
		handshakeHTTP(http,true);
	}else
	{
		LOG_DEBUG("Connect from %s '%s'",sb,buf);
		HTTP http(*sock);
		http.initRequest(buf);
		handshakeHTTP(http,false);
	}

}
// -----------------------------------
void Servent::triggerChannel(char *str, ChanInfo::PROTOCOL proto,bool relay)	
{

	ChanInfo info;

	servMgr->getChannel(str,info,relay);

	if (proto == ChanInfo::SP_PCP)
		type = T_RELAY;
	else
		type = T_DIRECT;

	outputProtocol = proto;

	processStream(false,info);

}
// -----------------------------------
void writePLSHeader(Stream &s, PlayList::TYPE type)
{
	s.writeLine(HTTP_SC_OK);
	s.writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT);

	const char *content;
	switch(type)
	{
		case PlayList::T_PLS:
			content = MIME_XM3U;
			break;
		case PlayList::T_ASX:
			content = MIME_ASX;
			break;
		case PlayList::T_RAM:
			content = MIME_RAM;
			break;
		default:
			content = MIME_TEXT;
			break;
	}
	s.writeLineF("%s %s",HTTP_HS_CONTENT,content);
    s.writeLine("Content-Disposition: inline");
    s.writeLine("Cache-Control: private" );
	s.writeLineF("%s %s",HTTP_HS_CONNECTION,"close");

	s.writeLine("");
}

// -----------------------------------
void Servent::handshakePLS(ChanInfo &info, bool doneHandshake)
{
	char url[256];

	char in[128];

	if (!doneHandshake)
		while (sock->readLine(in,128));


	if (getLocalURL(url))
	{

		PlayList::TYPE type;

		if ((info.contentType == ChanInfo::T_WMA) || (info.contentType == ChanInfo::T_WMV))
			type = PlayList::T_ASX;
		else if (info.contentType == ChanInfo::T_OGM)
			type = PlayList::T_RAM;
		else
			type = PlayList::T_PLS;

		writePLSHeader(*sock,type);

		PlayList *pls;

		pls = new PlayList(type,1);

		pls->addChannel(url,info);

		pls->write(*sock);

		delete pls;
	}
}
// -----------------------------------
void Servent::handshakePLS(ChanHitList **cl, int num, bool doneHandshake)
{
	char url[256];
	char in[128];

	if (!doneHandshake)
		while (sock->readLine(in,128));

	if (getLocalURL(url))
	{
		writePLSHeader(*sock,PlayList::T_SCPLS);

		PlayList *pls;

		pls = new PlayList(PlayList::T_SCPLS,num);

		for(int i=0; i<num; i++)
			pls->addChannel(url,cl[i]->info);

		pls->write(*sock);

		delete pls;
	}
}
// -----------------------------------
bool Servent::getLocalURL(char *str)
{
	if (!sock)
		throw StreamException("Not connected");


	char ipStr[64];

	Host h;
	
	if (sock->host.localIP())
		h = sock->getLocalHost();
	else
		h = servMgr->serverHost;

	h.port = servMgr->serverHost.port;

	h.toStr(ipStr);

	sprintf(str,"http://%s",ipStr);			
	return true;
}

// -----------------------------------
// Warning: testing RTSP/RTP stuff below.
// .. moved over to seperate app now.
// -----------------------------------
void Servent::handshakePOST()
{
	char tmp[1024];
    while (sock->readLine(tmp,sizeof(tmp)))
		LOG_DEBUG("POST: %s",tmp);

	throw HTTPException(HTTP_SC_BADREQUEST,400);
}


// -----------------------------------
void Servent::handshakeRTSP(RTSP &rtsp)
{
	throw HTTPException(HTTP_SC_BADREQUEST,400);
}
// -----------------------------------
bool Servent::handshakeAuth(HTTP &http,const char *args,bool local)
{
	char user[64],pass[64];
	user[0] = pass[0] = 0;

	char *pwd  = getCGIarg(args, "pass=");

	if ((pwd) && strlen(servMgr->password))
	{
		String tmp = pwd;
		char *as = strstr(tmp.cstr(),"&");
		if (as) *as = 0;
		if (strcmp(tmp,servMgr->password)==0)
		{
		    while (http.nextHeader());
			return true;
		}
	}

	Cookie gotCookie;
	cookie.clear();

    while (http.nextHeader())
	{
		char *arg = http.getArgStr();
		if (!arg)
			continue;

		switch (servMgr->authType)
		{
			case ServMgr::AUTH_HTTPBASIC:
				if (http.isHeader("Authorization"))
					http.getAuthUserPass(user,pass);
				break;
			case ServMgr::AUTH_COOKIE:
				if (http.isHeader("Cookie"))
				{
					LOG_DEBUG("Got cookie: %s",arg);
					char *idp=arg;
					while ((idp = strstr(idp,"id=")))
					{
						idp+=3;
						gotCookie.set(idp,sock->host.ip);
						if (servMgr->cookieList.contains(gotCookie))
						{
							LOG_DEBUG("Cookie found");
							cookie = gotCookie;
							break;
						}

					}
				}
				break;
		}
	}

	if (sock->host.isLocalhost())
		return true;


	switch (servMgr->authType)
	{
		case ServMgr::AUTH_HTTPBASIC:
				
			if ((strcmp(pass,servMgr->password)==0) && strlen(servMgr->password))
				return true;
			break;
		case ServMgr::AUTH_COOKIE:
			if (servMgr->cookieList.contains(cookie))
				return true;
			break;
	}



	if (servMgr->authType == ServMgr::AUTH_HTTPBASIC)
	{
		http.writeLine(HTTP_SC_UNAUTHORIZED);
		http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
	}else if (servMgr->authType == ServMgr::AUTH_COOKIE)
	{
		String file = servMgr->htmlPath;
		file.append("/login.html");
		if (local)
			handshakeLocalFile(file);
		else
			handshakeRemoteFile(file);
	}
	

	return false;
}

// -----------------------------------
void Servent::handshakeCMD(char *cmd)
{
	char result[MAX_CGI_LEN];
	char arg[MAX_CGI_LEN];
	char curr[MAX_CGI_LEN];

	char	jumpStr[128];
	char	*jumpArg=NULL;
	bool	retHTML=true;
	strcpy(result,"OK");

	HTTP http(*sock);
	HTML html("",*sock);


	if (!handshakeAuth(http,cmd,true))
		return;

	try
	{
		if (cmpCGIarg(cmd,"cmd=","redirect"))
		{
			char *j = getCGIarg(cmd,"url=");
			if (j)
			{
				termArgs(cmd);
				String url;
				url.set(j,String::T_ESC);
				url.convertTo(String::T_ASCII);

				if (!url.contains("http://"))
					url.prepend("http://");

				html.setRefreshURL(url.cstr());
				html.startHTML();
					html.addHead();
					html.startBody();
						html.startTagEnd("h3","Please wait...");
					html.end();
				html.end();

			}
		}else{

			if (cmpCGIarg(cmd,"cmd=","viewxml"))
			{

				handshakeXML();
				retHTML = false;
			}else if (cmpCGIarg(cmd,"cmd=","clearlog"))
			{
				sys->logBuf->clear();
				sprintf(jumpStr,"/%s/viewlog.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","save"))
			{

				peercastInst->saveSettings();

				sprintf(jumpStr,"/%s/settings.html",servMgr->htmlPath);					
				jumpArg = jumpStr;
			}else if (cmpCGIarg(cmd,"cmd=","reg"))
			{
				char idstr[128];
				chanMgr->broadcastID.toStr(idstr);
				sprintf(jumpStr,"http://www.peercast.org/register/?id=%s",idstr);					
				jumpArg = jumpStr;
			}else if (cmpCGIarg(cmd,"cmd=","edit_bcid"))
			{
				char *cp = cmd;
				GnuID id;
				BCID *bcid;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						id.fromStr(arg);
					else if (strcmp(curr,"del")==0)
						servMgr->removeValidBCID(id);
					else if (strcmp(curr,"valid")==0)
					{
						bcid = servMgr->findValidBCID(id);
						if (bcid)
							bcid->valid = getCGIargBOOL(arg);
					}
				}
				peercastInst->saveSettings();
				sprintf(jumpStr,"/%s/bcid.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","add_bcid"))
			{
				BCID *bcid = new BCID();

				char *cp = cmd;
				bool result=false;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						bcid->id.fromStr(arg);
					else if (strcmp(curr,"name")==0)
						bcid->name.set(arg);
					else if (strcmp(curr,"email")==0)
						bcid->email.set(arg);
					else if (strcmp(curr,"url")==0)
						bcid->url.set(arg);
					else if (strcmp(curr,"valid")==0)
						bcid->valid = getCGIargBOOL(arg);
					else if (strcmp(curr,"result")==0)
						result = true;

				}
	
				LOG_DEBUG("Adding BCID : %s",bcid->name.cstr());			
				servMgr->addValidBCID(bcid);
				peercastInst->saveSettings();
				if (result)
				{
					http.writeLine(HTTP_SC_OK);
					http.writeLine("");
					http.writeString("OK");
				}else
				{
					sprintf(jumpStr,"/%s/bcid.html",servMgr->htmlPath);					
					jumpArg = jumpStr;
				}


			}else if (cmpCGIarg(cmd,"cmd=","apply"))
			{	
				servMgr->numFilters = 0;
				ServFilter *currFilter=servMgr->filters;

				bool brRoot=false;
				bool getUpd=false;
				int showLog=0;
				int allowServer1=0;
				int allowServer2=0;
				int newPort=servMgr->serverHost.port;

				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					// server
					if (strcmp(curr,"serveractive")==0)
						servMgr->autoServe = getCGIargBOOL(arg);
					else if (strcmp(curr,"port")==0)
						newPort = getCGIargINT(arg);
					else if (strcmp(curr,"icymeta")==0)
					{
						int iv = getCGIargINT(arg);
						if (iv < 0) iv = 0;
						else if (iv > 16384) iv = 16384;

						chanMgr->icyMetaInterval = iv;

					}else if (strcmp(curr,"passnew")==0)
						strcpy(servMgr->password,arg);
					else if (strcmp(curr,"root")==0)
						servMgr->isRoot = getCGIargBOOL(arg);
					else if (strcmp(curr,"brroot")==0)
						brRoot = getCGIargBOOL(arg);
					else if (strcmp(curr,"getupd")==0)
						getUpd = getCGIargBOOL(arg);
					else if (strcmp(curr,"huint")==0)
						chanMgr->setUpdateInterval(getCGIargINT(arg));
					else if (strcmp(curr,"forceip")==0)
						servMgr->forceIP = arg;
					else if (strcmp(curr,"htmlPath")==0)
					{
						strcpy(servMgr->htmlPath,"html/");
						strcat(servMgr->htmlPath,arg);
					}else if (strcmp(curr,"djmsg")==0)
					{
						String msg;
						msg.set(arg,String::T_ESC);
						msg.convertTo(String::T_UNICODE);
						chanMgr->setBroadcastMsg(msg);
					}
					else if (strcmp(curr,"pcmsg")==0)
					{
						servMgr->rootMsg.set(arg,String::T_ESC);
						servMgr->rootMsg.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"minpgnu")==0)
						servMgr->minGnuIncoming = atoi(arg);
					else if (strcmp(curr,"maxpgnu")==0)
						servMgr->maxGnuIncoming = atoi(arg);
					


					// connections
					else if (strcmp(curr,"maxcin")==0)
						servMgr->maxControl = getCGIargINT(arg);
					else if (strcmp(curr,"maxsin")==0)
						servMgr->maxServIn = getCGIargINT(arg);

					else if (strcmp(curr,"maxup")==0)
						servMgr->maxBitrateOut = getCGIargINT(arg);
					else if (strcmp(curr,"maxrelays")==0)
						servMgr->setMaxRelays(getCGIargINT(arg));
					else if (strcmp(curr,"maxdirect")==0)
						servMgr->maxDirect = getCGIargINT(arg);
					else if (strcmp(curr,"maxrelaypc")==0)
						chanMgr->maxRelaysPerChannel = getCGIargINT(arg);
					else if (strncmp(curr,"filt_",5)==0)
					{
						char *fs = curr+5;
						{
							if (strncmp(fs,"ip",2)==0)		// ip must be first
							{
								currFilter = &servMgr->filters[servMgr->numFilters];
								currFilter->init();	
								currFilter->host.fromStrIP(arg,DEFAULT_PORT);
								if ((currFilter->host.ip) && (servMgr->numFilters < (ServMgr::MAX_FILTERS-1)))
								{
									servMgr->numFilters++;
									servMgr->filters[servMgr->numFilters].init();	// clear new entry
								}

							}else if (strncmp(fs,"bn",2)==0)
								currFilter->flags |= ServFilter::F_BAN;
							else if (strncmp(fs,"pr",2)==0)
								currFilter->flags |= ServFilter::F_PRIVATE;
							else if (strncmp(fs,"nw",2)==0)
								currFilter->flags |= ServFilter::F_NETWORK;
							else if (strncmp(fs,"di",2)==0)
								currFilter->flags |= ServFilter::F_DIRECT;
						}
					}

					// client
					else if (strcmp(curr,"clientactive")==0)
						servMgr->autoConnect = getCGIargBOOL(arg);
					else if (strcmp(curr,"yp")==0)
					{
						if (!PCP_FORCE_YP)
						{
							String str(arg,String::T_ESC);
							str.convertTo(String::T_ASCII);
							servMgr->rootHost = str;
						}
					}
					else if (strcmp(curr,"deadhitage")==0)
						chanMgr->deadHitAge = getCGIargINT(arg);
					else if (strcmp(curr,"refresh")==0)
						servMgr->refreshHTML = getCGIargINT(arg);
					else if (strcmp(curr,"auth")==0)
					{
						if (strcmp(arg,"cookie")==0)
							servMgr->authType = ServMgr::AUTH_COOKIE;
						else if (strcmp(arg,"http")==0)
							servMgr->authType = ServMgr::AUTH_HTTPBASIC;

					}else if (strcmp(curr,"expire")==0)
					{
						if (strcmp(arg,"session")==0)
							servMgr->cookieList.neverExpire = false;
						else if (strcmp(arg,"never")==0)
							servMgr->cookieList.neverExpire = true;
					}

					else if (strcmp(curr,"logDebug")==0)
						showLog |= atoi(arg)?(1<<LogBuffer::T_DEBUG):0;
					else if (strcmp(curr,"logErrors")==0)
						showLog |= atoi(arg)?(1<<LogBuffer::T_ERROR):0;
					else if (strcmp(curr,"logNetwork")==0)
						showLog |= atoi(arg)?(1<<LogBuffer::T_NETWORK):0;
					else if (strcmp(curr,"logChannel")==0)
						showLog |= atoi(arg)?(1<<LogBuffer::T_CHANNEL):0;

					else if (strcmp(curr,"allowHTML1")==0)
						allowServer1 |= atoi(arg)?(ALLOW_HTML):0;
					else if (strcmp(curr,"allowNetwork1")==0)
						allowServer1 |= atoi(arg)?(ALLOW_NETWORK):0;
					else if (strcmp(curr,"allowBroadcast1")==0)
						allowServer1 |= atoi(arg)?(ALLOW_BROADCAST):0;
					else if (strcmp(curr,"allowDirect1")==0)
						allowServer1 |= atoi(arg)?(ALLOW_DIRECT):0;

					else if (strcmp(curr,"allowHTML2")==0)
						allowServer2 |= atoi(arg)?(ALLOW_HTML):0;
					else if (strcmp(curr,"allowBroadcast2")==0)
						allowServer2 |= atoi(arg)?(ALLOW_BROADCAST):0;

				}



				servMgr->showLog = showLog;
				servMgr->allowServer1 = allowServer1;
				servMgr->allowServer2 = allowServer2;

				if (servMgr->serverHost.port != newPort)
				{
					Host lh(ClientSocket::getIP(NULL),newPort);
					char ipstr[64];
					lh.toStr(ipstr);
					sprintf(jumpStr,"http://%s/%s/settings.html",ipstr,servMgr->htmlPath);					

					servMgr->serverHost.port = newPort;
					servMgr->restartServer=true;
					//html.setRefresh(3);
					//html.setRefreshURL(jumpStr);
					//html.startHTML();
					//html.addHead();
					//	html.startBody();
					//		html.startTagEnd("h1","Please wait...");
					//	html.end();
					//html.end();

					

					//char ipstr[64];
					//servMgr->serverHost.toStr(ipstr);
					//sprintf(jumpStr,"/%s/settings.html",ipstr,servMgr->htmlPath);					
					jumpArg = jumpStr;

				}else
				{
					sprintf(jumpStr,"/%s/settings.html",servMgr->htmlPath);					
					jumpArg = jumpStr;
				}

				peercastInst->saveSettings();

				peercastApp->updateSettings();

				if ((servMgr->isRoot) && (brRoot))
					servMgr->broadcastRootSettings(getUpd);



				

			}else if (cmpCGIarg(cmd,"cmd=","fetch"))
			{

				ChanInfo info;
				String curl;

				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"url")==0)
					{
						curl.set(arg,String::T_ESC);
						curl.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"name")==0)
					{
						info.name.set(arg,String::T_ESC);
						info.name.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"desc")==0)
					{
						info.desc.set(arg,String::T_ESC);
						info.desc.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"genre")==0)
					{
						info.genre.set(arg,String::T_ESC);
						info.genre.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"contact")==0)
					{
						info.url.set(arg,String::T_ESC);
						info.url.convertTo(String::T_UNICODE);
					}else if (strcmp(curr,"bitrate")==0)
					{
						info.bitrate = atoi(arg);
					}else if (strcmp(curr,"type")==0)
					{
						info.contentType = ChanInfo::getTypeFromStr(arg);
					}

				}
				
				info.bcID = chanMgr->broadcastID;

				Channel *c = chanMgr->createChannel(info,NULL);
				if (c)
					c->startURL(curl.cstr());


				sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","stopserv"))
			{

				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"index")==0)
					{
						Servent *s = servMgr->findServentByIndex(atoi(arg));
						if (s)
							s->abort();
					}
				}
				sprintf(jumpStr,"/%s/connections.html",servMgr->htmlPath);					
				jumpArg = jumpStr;


			}else if (cmpCGIarg(cmd,"cmd=","hitlist"))
			{			
					
				bool stayConnected=hasCGIarg(cmd,"relay");

				int index = 0;
				ChanHitList *chl = chanMgr->hitlist;				
				while (chl)
				{
					if (chl->isUsed())
					{
						char tmp[64];
						sprintf(tmp,"c%d=",index);
						if (cmpCGIarg(cmd,tmp,"1"))
						{
							Channel *c;
							if (!(c=chanMgr->findChannelByID(chl->info.id)))
							{
								c = chanMgr->createChannel(chl->info,NULL);
								if (!c)
									throw StreamException("out of channels");
								c->stayConnected = stayConnected;
								c->startGet();
							}
						}
					}
					chl = chl->next;
					index++;
				}

				char *findArg = getCGIarg(cmd,"keywords=");

				if (hasCGIarg(cmd,"relay"))
				{
					sys->sleep(500);
					sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
					jumpArg = jumpStr;
				}
			}else if (cmpCGIarg(cmd,"cmd=","clear"))
			{
				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"hostcache")==0)
						servMgr->clearHostCache(ServHost::T_SERVENT);
					else if (strcmp(curr,"hitlists")==0)
						chanMgr->clearHitLists();
					else if (strcmp(curr,"packets")==0)
					{
						stats.clearRange(Stats::PACKETSSTART,Stats::PACKETSEND);
						servMgr->numVersions = 0;
					}
				}

				sprintf(jumpStr,"/%s/index.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","upgrade"))
			{
				if (servMgr->downloadURL[0])
				{
					sprintf(jumpStr,"/admin?cmd=redirect&url=%s",servMgr->downloadURL);
					jumpArg = jumpStr;
				}




			}else if (cmpCGIarg(cmd,"cmd=","connect"))
			{			
					

				Servent *s = servMgr->servents;
				{
					char tmp[64];
					sprintf(tmp,"c%d=",s->serventIndex);
					if (cmpCGIarg(cmd,tmp,"1"))
					{
						if (hasCGIarg(cmd,"stop"))
							s->thread.active = false;
					}
					s=s->next;
				}
				sprintf(jumpStr,"/%s/connections.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","shutdown"))
			{
				servMgr->shutdownTimer = 1;

			}else if (cmpCGIarg(cmd,"cmd=","stop"))
			{
				GnuID id;
				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						id.fromStr(arg);
				}

				Channel *c = chanMgr->findChannelByID(id);
				if (c)
					c->thread.active = false;

				sys->sleep(500);
				sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","bump"))
			{
				GnuID id;
				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						id.fromStr(arg);
				}

				Channel *c = chanMgr->findChannelByID(id);
				if (c)
					c->bump = true;

				sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","keep"))
			{
				GnuID id;
				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						id.fromStr(arg);
				}

				Channel *c = chanMgr->findChannelByID(id);
				if (c)
					c->stayConnected = true;

				sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
				jumpArg = jumpStr;

			}else if (cmpCGIarg(cmd,"cmd=","relay"))
			{
				ChanInfo info;
				char *cp = cmd;
				while (cp=nextCGIarg(cp,curr,arg))
				{
					if (strcmp(curr,"id")==0)
						info.id.fromStr(arg);
				}


				if (!chanMgr->findChannelByID(info.id))
				{

					ChanHitList *chl = chanMgr->findHitList(info);
					if (!chl)
						throw StreamException("channel not found");


					Channel *c = chanMgr->createChannel(chl->info,NULL);
					if (!c)
						throw StreamException("out of channels");

					c->stayConnected = true;
					c->startGet();
				}

				sprintf(jumpStr,"/%s/relays.html",servMgr->htmlPath);					
				jumpArg = jumpStr;


			}else if (cmpCGIarg(cmd,"net=","add"))
			{
				
				GnuID id;
				id.clear();
				while (cmd=nextCGIarg(cmd,curr,arg))
				{
					if (strcmp(curr,"ip")==0)
					{
						Host h;
						h.fromStrIP(arg,DEFAULT_PORT);
						if (servMgr->addOutgoing(h,id,true))
							LOG_NETWORK("Added connection: %s",arg);

					}else if (strcmp(curr,"id")==0)
					{
						id.fromStr(arg);
					}

				}

			}else if (cmpCGIarg(cmd,"cmd=","logout"))
			{
				jumpArg = "/";
				servMgr->cookieList.remove(cookie);

			}else if (cmpCGIarg(cmd,"cmd=","login"))
			{
				GnuID id;
				char idstr[64];
				id.generate();
				id.toStr(idstr);

				cookie.set(idstr,sock->host.ip);
				servMgr->cookieList.add(cookie);	

				http.writeLine(HTTP_SC_FOUND);
				if (servMgr->cookieList.neverExpire)
					http.writeLineF("%s id=%s; path=/; expires=\"Mon, 01-Jan-3000 00:00:00 GMT\";",HTTP_HS_SETCOOKIE,idstr);
				else
					http.writeLineF("%s id=%s; path=/;",HTTP_HS_SETCOOKIE,idstr);
				http.writeLineF("Location: /%s/index.html",servMgr->htmlPath);
				http.writeLine("");

			}else{

				sprintf(jumpStr,"/%s/index.html",servMgr->htmlPath);					
				jumpArg = jumpStr;
			}
		}

	}catch(StreamException &e)
	{
		html.startTagEnd("h1","ERROR - %s",e.msg);
		LOG_ERROR("html: %s",e.msg);
	}


	if (retHTML)
	{
		if (jumpArg)
		{
			String jmp(jumpArg,String::T_HTML);
			jmp.convertTo(String::T_ASCII);
			html.locateTo(jmp.cstr());
		}
	}


}
// -----------------------------------
static XML::Node *createChannelXML(Channel *c)
{
	XML::Node *n = c->info.createChannelXML();
	n->add(c->createRelayXML(true));
	n->add(c->info.createTrackXML());
//	n->add(c->info.createServentXML());
	return n;
}
// -----------------------------------
static XML::Node *createChannelXML(ChanHitList *chl)
{
	XML::Node *n = chl->info.createChannelXML();
	n->add(chl->createXML());
	n->add(chl->info.createTrackXML());
//	n->add(chl->info.createServentXML());
	return n;
}
// -----------------------------------	
void Servent::handshakeXML()
{
	int i;
				


	XML xml;

	XML::Node *rn = new XML::Node("peercast");
	xml.setRoot(rn);


	rn->add(new XML::Node("servent uptime=\"%d\"",servMgr->getUptime()));

	rn->add(new XML::Node("bandwidth out=\"%d\" in=\"%d\"",
		stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT),
		stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN)
		));

	rn->add(new XML::Node("connections total=\"%d\" relays=\"%d\" direct=\"%d\"",servMgr->totalConnected(),servMgr->numStreams(Servent::T_RELAY,true),servMgr->numStreams(Servent::T_DIRECT,true)));

	XML::Node *an = new XML::Node("channels_relayed total=\"%d\"",chanMgr->numChannels());
	rn->add(an);

	Channel *c = chanMgr->channel;
	while (c)
	{
		if (c->isActive())
			an->add(createChannelXML(c));
		c=c->next;
	}


	// add public channels
	{
		XML::Node *fn = new XML::Node("channels_found total=\"%d\"",chanMgr->numHitLists());
		rn->add(fn);

		ChanHitList *chl = chanMgr->hitlist;
		while (chl)
		{
			if (chl->isUsed())
				fn->add(createChannelXML(chl));
			chl = chl->next;
		}
	}

#if 0
	if (servMgr->isRoot)
	{
		// add private channels
		{
			XML::Node *pn = new XML::Node("priv_channels");
			rn->add(pn);

			ChanHitList *chl = chanMgr->hitlist;
			while (chl)
			{
				if (chl->isUsed())
					if (chl->info.isPrivate())
						pn->add(createChannelXML(chl));
				chl = chl->next;
			}
		}
	}
#endif

	XML::Node *hc = new XML::Node("host_cache");
	for(i=0; i<ServMgr::MAX_HOSTCACHE; i++)
	{
		ServHost *sh = &servMgr->hostCache[i];
		if (sh->type != ServHost::T_NONE)
		{
			char ipstr[64];
			sh->host.toStr(ipstr);

			hc->add(new XML::Node("host ip=\"%s\" type=\"%s\" time=\"%d\"",ipstr,ServHost::getTypeStr(sh->type),sh->time));

		}
	}
	rn->add(hc);


    sock->writeLine(HTTP_SC_OK);
	sock->writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT);
    sock->writeLineF("%s %s",HTTP_HS_CONTENT,MIME_XML);
	sock->writeLine("Connection: close");

    sock->writeLine("");

	xml.write(*sock);

}
// -----------------------------------
void Servent::readICYHeader(HTTP &http, ChanInfo &info, char *pwd)
{
	char *arg = http.getArgStr();
	if (!arg) return;

	if (http.isHeader("x-audiocast-name") || http.isHeader("icy-name") || http.isHeader("ice-name"))
	{
		info.name.set(arg,String::T_ASCII);
		info.name.convertTo(String::T_UNICODE);

	}else if (http.isHeader("x-audiocast-url") || http.isHeader("icy-url") || http.isHeader("ice-url"))
		info.url.set(arg,String::T_ASCII);
	else if (http.isHeader("x-audiocast-bitrate") || (http.isHeader("icy-br")) || http.isHeader("ice-bitrate") || http.isHeader("icy-bitrate"))
		info.bitrate = atoi(arg);
	else if (http.isHeader("x-audiocast-genre") || http.isHeader("ice-genre") || http.isHeader("icy-genre"))
	{
		info.genre.set(arg,String::T_ASCII);
		info.genre.convertTo(String::T_UNICODE);

	}else if (http.isHeader("x-audiocast-description") || http.isHeader("ice-description"))
	{
		info.desc.set(arg,String::T_ASCII);
		info.desc.convertTo(String::T_UNICODE);

	}else if (http.isHeader("Authorization"))
		http.getAuthUserPass(NULL,pwd);
	else if (http.isHeader(PCX_HS_CHANNELID))
		info.id.fromStr(arg);
	else if (http.isHeader("ice-password"))
	{
		if (pwd)
			if (strlen(arg) < 64)
				strcpy(pwd,arg);
	}else if (http.isHeader("content-type"))
	{
		if (stristr(arg,MIME_OGG))
			info.contentType = ChanInfo::T_OGG;
		else if (stristr(arg,MIME_XOGG))
			info.contentType = ChanInfo::T_OGG;

		else if (stristr(arg,MIME_MP3))
			info.contentType = ChanInfo::T_MP3;
		else if (stristr(arg,MIME_XMP3))
			info.contentType = ChanInfo::T_MP3;

		else if (stristr(arg,MIME_WMA))
			info.contentType = ChanInfo::T_WMA;
		else if (stristr(arg,MIME_WMV))
			info.contentType = ChanInfo::T_WMV;
		else if (stristr(arg,MIME_ASX))
			info.contentType = ChanInfo::T_ASX;

		else if (stristr(arg,MIME_NSV))
			info.contentType = ChanInfo::T_NSV;
		else if (stristr(arg,MIME_RAW))
			info.contentType = ChanInfo::T_RAW;

		else if (stristr(arg,MIME_MMS))
			info.srcProtocol = ChanInfo::SP_MMS;
		else if (stristr(arg,MIME_XPCP))
			info.srcProtocol = ChanInfo::SP_PCP;
		else if (stristr(arg,MIME_XPEERCAST))
			info.srcProtocol = ChanInfo::SP_PEERCAST;

		else if (stristr(arg,MIME_XSCPLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_PLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_XPLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_M3U))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_MPEGURL))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_TEXT))
			info.contentType = ChanInfo::T_PLS;


	}

}

// -----------------------------------
void Servent::handshakeICY(Channel::SRC_TYPE type, bool isHTTP)
{
	ChanInfo info;

	HTTP http(*sock);

	// default to mp3 for shoutcast DSP (doesn`t send content-type)
	if (type == Channel::SRC_SHOUTCAST)
		info.contentType = ChanInfo::T_MP3;

	while (http.nextHeader())
	{
		LOG_DEBUG("ICY %s",http.cmdLine);
		readICYHeader(http,info,loginPassword.cstr());
	}


		
	// check password before anything else, if needed
	if (loginPassword != servMgr->password)
	{
		if (!sock->host.isLocalhost() || !loginPassword.isEmpty())
			throw HTTPException(HTTP_SC_UNAUTHORIZED,401);
	}

		
	// we need a valid IP address before we start
	servMgr->checkFirewall();


	// attach channel ID to name, channel ID is also encoded with IP address 
	// to help prevent channel hijacking. 


	info.id = chanMgr->broadcastID;
	info.id.encode(NULL,info.name.cstr(),loginMount.cstr(),info.bitrate);

	LOG_DEBUG("Incoming source: %s : %s",info.name.cstr(),ChanInfo::getTypeStr(info.contentType));


	if (isHTTP)
		sock->writeStringF("%s\n\n",HTTP_SC_OK);
	else
		sock->writeLine("OK");

	Channel *c = chanMgr->findChannelByID(info.id);
	if (c)
	{
		LOG_CHANNEL("ICY channel already active, closing old one");
		c->thread.shutdown();
	}


	info.comment = chanMgr->broadcastMsg;
	info.bcID = chanMgr->broadcastID;

	c = chanMgr->createChannel(info,loginMount.cstr());
	if (!c)
		throw HTTPException(HTTP_SC_UNAVAILABLE,503);

	c->startICY(sock,type);
}


// -----------------------------------
void Servent::handshakeLocalFile(const char *fn)
{
	HTTP http(*sock);
	String fileName;
	
	fileName = peercastApp->getPath();
	
	fileName.append(fn);

	LOG_DEBUG("Writing HTML file: %s",fileName.cstr());

	HTML html("",*sock);

	char *args = strstr(fileName.cstr(),"?");
	if (args)
		*args++=0;

	if (fileName.contains(".htm"))
	{
		html.writeOK(MIME_HTML);
		html.writeTemplate(fileName.cstr(),args);

	}else if (fileName.contains(".css"))
	{
		html.writeOK(MIME_CSS);
		html.writeRawFile(fileName.cstr());
	}else if (fileName.contains(".jpg"))
	{
		html.writeOK(MIME_JPEG);
		html.writeRawFile(fileName.cstr());
	}else if (fileName.contains(".gif"))
	{
		html.writeOK(MIME_GIF);
		html.writeRawFile(fileName.cstr());
	}else if (fileName.contains(".png"))
	{
		html.writeOK(MIME_PNG);
		html.writeRawFile(fileName.cstr());
	}
}

// -----------------------------------
void Servent::handshakeRemoteFile(const char *dirName)
{
	ClientSocket *rsock = sys->createSocket();
	if (!rsock)
		throw HTTPException(HTTP_SC_UNAVAILABLE,503);


	const char *hostName = "www.peercast.org";	// hardwired for "security"

	Host host;
	host.fromStrName(hostName,80);


	rsock->open(host);
	rsock->connect();

	HTTP rhttp(*rsock);

	rhttp.writeLineF("GET /%s HTTP/1.0",dirName);
	rhttp.writeLineF("%s %s",HTTP_HS_HOST,hostName);
	rhttp.writeLineF("%s %s",HTTP_HS_CONNECTION,"close");
	rhttp.writeLineF("%s %s",HTTP_HS_ACCEPT,"*/*");
	rhttp.writeLine("");

	String contentType;
	bool isTemplate = false;
	while (rhttp.nextHeader())
	{
		char *arg = rhttp.getArgStr();
		if (arg) 
		{
			if (rhttp.isHeader("content-type"))
				contentType = arg;
		}
	}

	MemoryStream mem(100*1024);
	while (!rsock->eof())
	{
		int len=0;
		char buf[4096];
		len = rsock->readUpto(buf,sizeof(buf));
		if (len==0)
			break;
		else
			mem.write(buf,len);

	}
	rsock->close();

	int fileLen = mem.getPosition();
	mem.len = fileLen;
	mem.rewind();


	if (contentType.contains(MIME_HTML))
		isTemplate = true;

	sock->writeLine(HTTP_SC_OK);
	sock->writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT);
	sock->writeLineF("%s %s",HTTP_HS_CACHE,"no-cache");
	sock->writeLineF("%s %s",HTTP_HS_CONNECTION,"close");
	sock->writeLineF("%s %s",HTTP_HS_CONTENT,contentType.cstr());

	sock->writeLine("");

	if (isTemplate)
	{
		HTML html("",*sock);
		html.readTemplate(mem,sock,0);
	}else
		sock->write(mem.buf,fileLen);

	mem.free();
}
