// ------------------------------------------------
// File : url.h
// Date: 20-feb-2004
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

#include "url.h"
#include "socket.h"
#include "http.h"
#include "servent.h"
#include "servmgr.h"
#include "peercast.h"
#include "version2.h"

// ------------------------------------------------
void URLSource::stream(Channel *ch)
{
	
	String url;
	while (ch->thread.active && !peercastInst->isQuitting)
	{
		if (url.isEmpty())
			url = baseurl;

		url = streamURL(ch,url.cstr());
	}

}
// ------------------------------------------------
int URLSource::getSourceRate()
{
	if (inputStream)
		return inputStream->bytesInPerSec;
	else
		return 0;
}

// ------------------------------------------------
::String URLSource::streamURL(Channel *ch, const char *url)
{
	String nextURL;

	if (peercastInst->isQuitting || !ch->thread.active)
		return nextURL;


	String urlTmp;
	urlTmp.set(url);

	char *fileName = urlTmp.cstr();

	PlayList *pls=NULL;
	ChannelStream *source=NULL;

	LOG_CHANNEL("Fetch URL=%s",fileName);

	try
	{

		// get the source protocol
		if (strnicmp(fileName,"http://",7)==0) 	
		{
			ch->info.srcProtocol = ChanInfo::SP_HTTP;
			fileName += 7;
		}
		else if (strnicmp(fileName,"mms://",6)==0) 	
		{
			ch->info.srcProtocol = ChanInfo::SP_MMS;
			fileName += 6;
		}
		else if (strnicmp(fileName,"pcp://",6)==0) 	
		{
			ch->info.srcProtocol = ChanInfo::SP_PCP;
			fileName += 6;
		}
		else if (strnicmp(fileName,"file://",7)==0) 	
		{
			ch->info.srcProtocol = ChanInfo::SP_FILE;
			fileName += 7;
		}
		else 
		{
			ch->info.srcProtocol = ChanInfo::SP_FILE;
		}

		// default to mp3 for shoutcast servers
		if (ch->info.contentType == ChanInfo::T_PLS)
			ch->info.contentType = ChanInfo::T_MP3;


		ch->setStatus(Channel::S_CONNECTING);

		if ((ch->info.srcProtocol == ChanInfo::SP_HTTP) || (ch->info.srcProtocol == ChanInfo::SP_PCP) || (ch->info.srcProtocol == ChanInfo::SP_MMS))
		{

			if ((ch->info.contentType == ChanInfo::T_WMA) || (ch->info.contentType == ChanInfo::T_WMV))
				ch->info.srcProtocol = ChanInfo::SP_MMS;

			
			LOG_CHANNEL("Channel source is HTTP");

			ClientSocket *inputSocket = sys->createSocket();
			if (!inputSocket)
				throw StreamException("Channel cannot create socket");


			inputStream = inputSocket;


			char *dir = strstr(fileName,"/");
			if (dir)
				*dir++=0;


			LOG_CHANNEL("Fetch Host=%s",fileName);
			if (dir)
				LOG_CHANNEL("Fetch Dir=%s",dir);


			Host host;
			host.fromStrName(fileName,80);

			inputSocket->open(host);
			inputSocket->connect();

			HTTP http(*inputSocket);
			http.writeLineF("GET /%s HTTP/1.0",dir?dir:"");

			http.writeLineF("%s %s",HTTP_HS_HOST,fileName);
			http.writeLineF("%s %s",HTTP_HS_CONNECTION,"close");
			http.writeLineF("%s %s",HTTP_HS_ACCEPT,"*/*");

			if (ch->info.srcProtocol == ChanInfo::SP_MMS)
			{
				http.writeLineF("%s %s",HTTP_HS_AGENT,"NSPlayer/4.1.0.3856");
				http.writeLine("Pragma: no-cache,rate=1.000000,request-context=1");
				//http.writeLine("Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=4294967295:4294967295,request-context=22605256,max-duration=0");
				http.writeLine("Pragma: xPlayStrm=1");
				http.writeLine("Pragma: xClientGUID={c77e7400-738a-11d2-9add-0020af0a3278}");
				http.writeLine("Pragma: stream-switch-count=2");
				http.writeLine("Pragma: stream-switch-entry=ffff:1:0 ffff:2:0");
			}else
			{
				http.writeLineF("%s %s",HTTP_HS_AGENT,PCX_AGENT);  
				http.writeLineF("%s %d",PCX_HS_PCP,1);  
				http.writeLine("Icy-MetaData:1");				// fix by ravon 
			}

			http.writeLine("");

			int res = http.readResponse();


			String name = ch->info.name;

			while (http.nextHeader())
			{				

				LOG_CHANNEL("Fetch HTTP: %s",http.cmdLine);

				ChanInfo tmpInfo = ch->info;
				Servent::readICYHeader(http,ch->info,NULL);

				if (!tmpInfo.name.isEmpty())
					ch->info.name = tmpInfo.name;
				if (!tmpInfo.genre.isEmpty())
					ch->info.genre = tmpInfo.genre;
				if (!tmpInfo.url.isEmpty())
					ch->info.url = tmpInfo.url;

				if (http.isHeader("icy-metaint"))
					ch->icyMetaInterval = http.getArgInt();
				else if (http.isHeader("Location:"))
					nextURL.set(http.getArgStr());

				char *arg = http.getArgStr();
				if (arg)
				{
					if (http.isHeader("content-type"))
					{
						if (stristr(arg,MIME_XSCPLS))
							pls = new PlayList(PlayList::T_SCPLS, 1000);
						else if (stristr(arg,MIME_PLS))
							pls = new PlayList(PlayList::T_PLS, 1000);
						else if (stristr(arg,MIME_XPLS))
							pls = new PlayList(PlayList::T_PLS, 1000);
						else if (stristr(arg,MIME_M3U))
							pls = new PlayList(PlayList::T_PLS, 1000);
						else if (stristr(arg,MIME_TEXT))
							pls = new PlayList(PlayList::T_PLS, 1000);
						else if (stristr(arg,MIME_ASX))
							pls = new PlayList(PlayList::T_ASX, 1000);
						else if (stristr(arg,MIME_MMS))
							ch->info.srcProtocol = ChanInfo::SP_MMS;
					}
				}

			}

			if ((!nextURL.isEmpty()) && (res==302))
			{
				LOG_CHANNEL("Channel redirect: %s",nextURL.cstr());
				inputSocket->close();
				delete inputSocket;
				inputSocket = NULL;
				return nextURL;
			}

			if (res!=200)
			{
				LOG_ERROR("HTTP response: %d",res);
				throw StreamException("Bad HTTP connect");
			}


		}else if (ch->info.srcProtocol == ChanInfo::SP_FILE)
		{

			LOG_CHANNEL("Channel source is FILE");

			FileStream *fs = new FileStream();
			fs->openReadOnly(fileName);
			inputStream = fs;

			ChanInfo::TYPE fileType = ChanInfo::T_UNKNOWN;
			// if filetype is unknown, try and figure it out from file extension.
			//if ((info.srcType == ChanInfo::T_UNKNOWN) || (info.srcType == ChanInfo::T_PLAYLIST))
			{
				const char *ext = fileName+strlen(fileName);
				while (*--ext)
					if (*ext=='.')
					{
						ext++;
						break;
					}

				fileType = ChanInfo::getTypeFromStr(ext);
			}


			if (ch->info.bitrate)
				ch->readDelay = true;


			if (fileType == ChanInfo::T_PLS) 
				pls = new PlayList(PlayList::T_PLS, 1000);
			else if (fileType == ChanInfo::T_ASX) 
				pls = new PlayList(PlayList::T_ASX, 1000);
			else
				ch->info.contentType = fileType;

		}else
		{
			throw StreamException("Unsupported URL");
		}

		
		if (pls)
		{

			LOG_CHANNEL("Channel is Playlist");

			pls->read(*inputStream);

			inputStream->close();
			delete inputStream;
			inputStream = NULL;

			int urlNum=0;
			String url;

			LOG_CHANNEL("Playlist: %d URLs",pls->numURLs);
			while ((ch->thread.active) && (pls->numURLs) && (!peercastInst->isQuitting))
			{
				if (url.isEmpty())
				{
					url = pls->urls[urlNum%pls->numURLs];
					urlNum++;
				}
				try
				{
					url = streamURL(ch,url.cstr());
				}catch(StreamException &)
				{}
			}

			delete pls;
			
		}else
		{

			// if we didn`t get a channel id from the source, then create our own (its an original broadcast)
			if (!ch->info.id.isSet())
			{
				ch->info.id = chanMgr->broadcastID;
				ch->info.id.encode(NULL,ch->info.name.cstr(),ch->info.genre,ch->info.bitrate);
			}

			if (ch->info.contentType == ChanInfo::T_ASX)
				ch->info.contentType = ChanInfo::T_WMV;

			ch->setStatus(Channel::S_BROADCASTING);

			inputStream->setReadTimeout(60);	// use longer read timeout

			source = ch->createSource();

			ch->readStream(*inputStream,source);

			inputStream->close();
		}

	}catch(StreamException &e)
	{
		ch->setStatus(Channel::S_ERROR);
		LOG_ERROR("Channel error: %s",e.msg);
		sys->sleep(1000);
	}


	ch->setStatus(Channel::S_CLOSING);
	if (inputStream)
	{
		delete inputStream;
		inputStream = NULL;
	}

	if (source)
		delete source;


	return nextURL;

}
