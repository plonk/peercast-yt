// ------------------------------------------------
// File : servhtml.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		HTML support for servents, TODO: should be in its own class
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

typedef int (*COMPARE_FUNC)(const void *,const void *);
typedef int (*COMPARE_FUNC2)(ChanHitList **, ChanHitList **);
// -----------------------------------
void Servent::addBasicHeader(HTML &html)
{
	html.writeOK(MIME_HTML);
	html.startHTML();
	html.addHead();
	html.startBody();


}

// -----------------------------------
void Servent::addHeader(HTML &html, int sel)
{
	addBasicHeader(html);



	html.startTagEnd("div align=\"center\"",PCX_VERSTRING);

	if (servMgr->downloadURL[0])
	{

		html.startTag("font color=\"#FF0000\"");
			html.startTag("div align=\"center\"");
				html.startTagEnd("h2","! Attention !");
			html.end();
		html.end();

		html.startTag("h3");
			html.startTag("div align=\"center\"");
				html.addLink("/admin?cmd=upgrade","Click here to update your client");
			html.end();
		html.end();
	}

	if (!servMgr->rootMsg.isEmpty())
	{
		String pcMsg = servMgr->rootMsg;
		pcMsg.convertTo(String::T_HTML);
		html.startTag("div align=\"center\"");
			html.startTagEnd("h3",pcMsg.cstr());
		html.end();
	}


	html.startTag("table width=\"100%%\"");

		html.startTag("tr bgcolor=\"#CCCCCC\"");
			
			if (sel >= 0)
			{
				html.startTag("td");
					html.startTag("div align=\"center\"");
					html.addLink("/admin?page=index",sel==1?"<b>Index</b>":"Index");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
						if (servMgr->isRoot)
							html.addLink("/admin?page=chans",sel==2?"<b>All channels</b>":"All channels");
						else
						{
							String url;
							sprintf(url.cstr(),"http://yp.peercast.org?port=%d",servMgr->serverHost.port);
							html.addLink(url.cstr(),"Yellow Pages",true);
						}
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
						html.addLink("/admin?page=mychans",sel==3?"<b>Relayed channels</b>":"Relayed channels");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
						html.addLink("/admin?page=broadcast",sel==8?"<b>Broadcast</b>":"Broadcast");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
					html.addLink("/admin?page=connections",sel==4?"<b>Connections</b>":"Connections");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
					html.addLink("/admin?page=settings",sel==5?"<b>Settings</b>":"Settings");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
					html.addLink("/admin?page=viewlog",sel==7?"<b>View Log</b>":"View Log");
					html.end();
				html.end();

				html.startTag("td");
					html.startTag("div align=\"center\"");
					html.addLink("/admin?page=logout",sel==6?"<b>Logout</b>":"Logout");
					html.end();
				html.end();
			}else{
				html.startTagEnd("td");
			}


		html.end(); // tr

	html.end(); // table

	html.startTagEnd("b","<br>");
}
// -----------------------------------
void Servent::addFooter(HTML &html)
{
	html.startTagEnd("p","<br>");
	html.startTag("table width=\"100%%\"");

		html.startTag("tr bgcolor=\"#CCCCCC\"");
			
			html.startTag("td");
				html.startTagEnd("div align=\"center\"","&copy; <a target=\"_blank\" href=/admin?cmd=redirect&url=www.peercast.org>peercast.org</a> 2004");
			html.end();

		html.end();

	html.end();


	html.end();	// body
	html.end();	// html

}
// -----------------------------------
void Servent::addAdminPage(HTML &html)
{
	html.setRefresh(servMgr->refreshHTML);
	addHeader(html,1);

	html.startTag("table border=\"0\" align=\"center\"");

			html.startTag("tr align=\"center\"");
				html.startTag("td valign=\"top\"");
					addInformation(html);
				html.end();
				html.startTag("td valign=\"top\"");
					addStatistics(html);
				html.end();
			html.end();
	html.end();
	addFooter(html);
}

// -----------------------------------
void Servent::addLogPage(HTML &html)
{
	addHeader(html,7);

	html.addLink("/admin?cmd=clearlog","Clear log<br>");
	html.addLink("#bottom"," View tail<br><br>");

	html.startTag("font size=\"-1\"");

		sys->logBuf->dumpHTML(*html.out);

	html.end();

	html.addLink("/admin?page=viewlog","<br>View top");
	html.startSingleTagEnd("a name=\"bottom\"");

	addFooter(html);
}

// -----------------------------------
void Servent::addLoginPage(HTML &html)
{
	addHeader(html,-1);

	html.startTag("table border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Login</b>"); 
		html.end();

		html.startTag("form method=\"get\" action=\"/admin?\"");

		html.startSingleTagEnd("input name=\"cmd\" type=\"hidden\" value=\"login\"");					

		int row=0;
		// password
		html.startTableRow(row++);
			html.startTagEnd("td","Password");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"pass\" size=\"10\" type=\"password\"");
			html.end();
		html.end();

		// login
		html.startTableRow(row++);
			html.startTagEnd("td","");
			html.startTag("td");
				html.startTagEnd("input name=\"submit\" type=\"submit\" id=\"submit\" value=\"Login\"");					
			html.end();
		html.end();
	html.end();

	addFooter(html);
}
// -----------------------------------
void Servent::addLogoutPage(HTML &html)
{
	addHeader(html,6);

	html.startTag("table border=\"0\" align=\"center\"");

		html.startTag("tr align=\"center\"");
			html.startTag("td colspan=\"2\" valign=\"top\"");

				html.startTag("tr align=\"center\"");
					html.startTag("td");
					if ((servMgr->authType != ServMgr::AUTH_HTTPBASIC) 
					     && !sock->host.isLocalhost()
						)
					{
						html.startTag("form method=\"get\" action=\"/admin\"");
							html.startTagEnd("input name=\"logout\" type=\"submit\" value=\"Logout\"");					
							html.startSingleTagEnd("input name=\"cmd\" type=\"hidden\" value=\"logout\"");					
						html.end();
					}
					html.end();
					html.startTag("td");
						html.startTag("form method=\"get\" action=\"/admin\"");
							html.startTagEnd("input name=\"logout\" type=\"submit\" value=\"Shutdown\"");					
							html.startSingleTagEnd("input name=\"page\" type=\"hidden\" value=\"shutdown\"");					
						html.end();
					html.end();

				html.end();
			html.end();




		html.end(); // form

	html.end(); // table

	
	addFooter(html);
}
// -----------------------------------
void Servent::addShutdownPage(HTML &html)
{
	html.startTag("h1");
		html.startTagEnd("div align=\"center\"","PeerCast will shutdown in 3 seconds");
	html.end();
	servMgr->shutdownTimer = 3;
}

// -----------------------------------
void addChannelSourceTag(HTML &html, Channel *c)
{
	const char *stype = c->getSrcTypeStr();
	const char *ptype = ChanInfo::getProtocolStr(c->info.srcProtocol);

	if (!c->sourceURL.isEmpty())
		html.startTagEnd("td","%s-%s:<br>%s",stype,ptype,c->sourceURL.cstr()); 
	else
	{
		char ipStr[64];
		if (c->sock)
			c->sock->host.toStr(ipStr);
		else
		{
			strcpy(ipStr,"Unknown");
		}

		html.startTagEnd("td","%s-%s:<br>%s",stype,ptype,ipStr); 
	}
}

// -----------------------------------
void	Servent::addChanInfo(HTML &html, ChanInfo *info, Channel *ch)
{
	int row=0;

	TrackInfo track;
	String name,genre,url,desc,comment,temp,hitTime;

	track = info->track;
	track.convertTo(String::T_HTML);
	name = info->name;
	name.convertTo(String::T_HTML);
	genre = info->genre;
	genre.convertTo(String::T_HTML);
	url = info->url;
	url.convertTo(String::T_HTML);
	desc = info->desc;
	desc.convertTo(String::T_HTML);
	comment = info->comment;
	comment.convertTo(String::T_HTML);

	char idStr[64];
	info->id.toStr(idStr);

	html.startTag("table border=\"0\" align=\"center\"");
		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\"","<b>Channel Information</b>"); 
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Name");

			html.startTag("td");
				sprintf(temp.data,"peercast://pls/%s",idStr);
				html.addLink(temp.data,name.cstr());
			html.end();
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Genre");
			html.startTagEnd("td",genre.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Desc.");
			html.startTagEnd("td",desc.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","URL");

			html.startTag("td");
				String tmp;
				sprintf(tmp.data,"/admin?cmd=redirect&url=%s",url.cstr());
				html.addLink(tmp.data,url.cstr(),true);
			html.end();

		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Comment");
			html.startTagEnd("td",comment.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","ID");
			html.startTagEnd("td",idStr);
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Type");
			html.startTagEnd("td",ChanInfo::getTypeStr(info->contentType));
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Bitrate");
			html.startTagEnd("td","%d kb/s",info->bitrate);
		html.end();

		if (ch)
		{
			html.startTableRow(row++);
				html.startTagEnd("td","Source");
				addChannelSourceTag(html,ch);
			html.end();

			html.startTableRow(row++);
					html.startTagEnd("td","Uptime");
					String uptime;
					if (info->lastPlayTime)
						uptime.setFromStopwatch(sys->getTime()-info->lastPlayTime);
					else
						uptime.set("-");
					html.startTagEnd("td",uptime.cstr());
			html.end();


			html.startTableRow(row++);
					html.startTagEnd("td","Skips");
					html.startTagEnd("td","%d",info->numSkips);
			html.end();
		
			html.startTableRow(row++);
				html.startTagEnd("td","Status");
				html.startTagEnd("td",ch->getStatusStr());
			html.end();

			html.startTableRow(row++);
				html.startTagEnd("td","Position");
				html.startTagEnd("td","%d",ch->streamPos);
			html.end();

			html.startTableRow(row++);
				html.startTagEnd("td","Head");
				html.startTagEnd("td","%d (%d bytes)",ch->headPack.pos,ch->headPack.len);
			html.end();



		}


		html.startTableRow(row++);
			html.startTagEnd("td colspan=\"2\" align=\"center\"","<b>Current Track</b>");
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Artist");
			html.startTagEnd("td",track.artist.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Title");
			html.startTagEnd("td",track.title.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Album");
			html.startTagEnd("td",track.album.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Genre");
			html.startTagEnd("td",track.genre.cstr());
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Contact");
			html.startTagEnd("td",track.contact.cstr());
		html.end();

	html.end();
}

static int compareHits(ChanHit **c2, ChanHit **c1)
{
	return c1[0]->time-c2[0]->time;
}

// -----------------------------------
void	Servent::addChanHits(HTML &html, ChanHitList *chl, ChanHit *source, ChanInfo &info)
{

	
	html.startTagEnd("br");

	html.startTag("table border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td"," "); 
			html.startTagEnd("td","<b>IP:Port</b>"); 
			html.startTagEnd("td","<b>Hops</b>"); 
			html.startTagEnd("td","<b>Listeners</b>"); 
			html.startTagEnd("td","<b>Relays</b>"); 
			html.startTagEnd("td","<b>Uptime</b>"); 
			html.startTagEnd("td","<b>Skips</b>"); 
			html.startTagEnd("td","<b>Push</b>"); 
			html.startTagEnd("td","<b>Busy</b>"); 
			html.startTagEnd("td","<b>Tracker</b>"); 
			html.startTagEnd("td","<b>Agent</b>"); 
			html.startTagEnd("td","<b>Update</b>"); 
		html.end();

		int i;
		int row=0;

		ChanHit *hits[ChanHitList::MAX_HITS];

		int numHits=0;
		for(i=0; i<ChanHitList::MAX_HITS; i++)
			if (chl->hits[i].host.ip)
				hits[numHits++] = &chl->hits[i];

		qsort(hits,numHits,sizeof(ChanHit*),(COMPARE_FUNC)compareHits);
				
		for(i=0; i<numHits; i++)
		{
			ChanHit *ch = hits[i];
			if (ch->host.ip)
			{
				html.startTableRow(row++);

				bool isSource = false;


				if (source)
					if (source->host.isSame(ch->host))
						isSource = true;

				html.startTagEnd("td",isSource?"*":"");

				// IP
				char ip0Str[64];
				ch->rhost[0].toStr(ip0Str);
				char ip1Str[64];
				ch->rhost[1].toStr(ip1Str);


				// ID
				char idStr[64];
				info.id.toStr(idStr);

				if ((ch->rhost[0].ip != ch->rhost[1].ip) && (ch->rhost[1].isValid()))
				{
					html.startTagEnd("td","%s/%s",ip0Str,ip1Str);
				}else{
					html.startTagEnd("td","%s",ip0Str);
				}

				html.startTagEnd("td","%d",ch->hops);
				html.startTagEnd("td","%d",ch->numListeners);
				html.startTagEnd("td","%d",ch->numRelays);

				String hitTime;
				hitTime.setFromStopwatch(ch->upTime);
				html.startTagEnd("td",hitTime.cstr());

				html.startTagEnd("td","%d",ch->numSkips);

				html.startTagEnd("td","%s",ch->firewalled?"Yes":"No");
				html.startTagEnd("td","%s",ch->busy?"Yes":"No");
				html.startTagEnd("td","%s",ch->tracker?"Yes":"No");


				if (ch->agentStr[0])
					html.startTagEnd("td","%s",ch->agentStr);
				else
					html.startTagEnd("td","-");

				if (ch->time)
					hitTime.setFromStopwatch(sys->getTime()-ch->time);
				else
					hitTime.set("-");
				html.startTagEnd("td",hitTime.cstr());

				html.end();
			}
		}

	html.end();
}
	
// -----------------------------------
void Servent::addSettingsPage(HTML &html)
{
	addHeader(html,5);

	html.startTag("table border=\"0\" align=\"center\"");

		html.startTag("form method=\"get\" action=\"/admin\"");

			html.startTagEnd("input name=\"cmd\" type=\"hidden\" value=\"apply\"");					

			html.startTag("tr align=\"center\"");
				html.startTag("td valign=\"top\"");
					addBroadcasterOptions(html);
				html.end();
			html.end();

			html.startTag("tr align=\"center\"");
				html.startTag("td valign=\"top\"");
					addServerOptions(html);
				html.end();
				html.startTag("td valign=\"top\"");
					addRelayOptions(html);
				html.end();
			html.end();

			html.startTag("tr align=\"center\"");
				html.startTag("td valign=\"top\"");
					addFilterOptions(html);
				html.end();
				html.startTag("td valign=\"top\"");
					addSecurityOptions(html);
				html.end();
			html.end();

			html.startTag("tr align=\"center\"");
				html.startTag("td valign=\"top\"");
					addAuthOptions(html);
				html.end();
				html.startTag("td valign=\"top\"");
					addLogOptions(html);
				html.end();
			html.end();


			if (servMgr->isRoot)
			{
				html.startTag("tr align=\"center\"");
					html.startTag("td valign=\"top\"");
						addRootOptions(html);
					html.end();
				html.end();
			}

			html.startTag("tr align=\"center\"");
				html.startTag("td colspan=\"2\"");
					html.startTagEnd("input name=\"submit\" type=\"submit\" value=\"Save Settings\"");					
				html.end();
			html.end();

		html.end(); // form

	html.end(); // table

	addFooter(html);
}	

// -----------------------------------
void Servent::addWinampSettingsPage(HTML &html)
{
	addBasicHeader(html);

	html.startTag("form method=\"get\" action=\"/admin\"");
		html.startTagEnd("input name=\"submit\" type=\"submit\" value=\"Save Settings\"");					
		html.startTagEnd("input name=\"cmd\" type=\"hidden\" value=\"apply\"");					

		addServerOptions(html);
		//addClientOptions(html);
		addBroadcasterOptions(html);
		addRelayOptions(html);
		addFilterOptions(html);
		addSecurityOptions(html);
		addAuthOptions(html);
		addLogOptions(html);

		if (servMgr->isRoot)
			addRootOptions(html);

	html.end(); // form

	addFooter(html);
}	

// -----------------------------------
static int addStat(HTML &html,int row, int totIn,int totOut, const char *name, Stats::STAT in, Stats::STAT out)
{
	html.startTableRow(row++);
		html.startTagEnd("td",name); 
		unsigned int v;

		if ((in) && (totIn))
		{
			v = stats.getCurrent(in);
			html.startTagEnd("td","%d",v);
			html.startTagEnd("td","%d",totIn?((v*100)/totIn):0);
		}else
		{
			html.startTagEnd("td","-");
			html.startTagEnd("td","-");
		}

		if ((out) && (totOut))
		{
			v = stats.getCurrent(out);
			html.startTagEnd("td","%d",v);
			html.startTagEnd("td","%d",totOut?((v*100)/totOut):0);
		}else
		{
			html.startTagEnd("td","-");
			html.startTagEnd("td","-");
		}

		
	html.end();
	return row;
}
// -----------------------------------
void Servent::addNetStatsPage(HTML &html)
{
	addHeader(html,0);

	int row=0;
	html.startTag("table width=\"50%%\" border=\"0\" align=\"center\"");
		row = 0;
		html.startTag("tr width=\"100%%\" bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"5\"","<b>Packets</b>"); 
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td"," "); 
			html.startTagEnd("td","<b>In</b>"); 
			html.startTagEnd("td","<b>In %s</b>","%%"); 
			html.startTagEnd("td","<b>Out</b>"); 
			html.startTagEnd("td","<b>Out %s</b>","%%"); 
		html.end();

		unsigned int totalIn = stats.getCurrent(Stats::NUMPACKETSIN);
		unsigned int totalOut = stats.getCurrent(Stats::NUMPACKETSOUT);

		row=addStat(html,row,totalIn,totalOut,"Total",Stats::NUMPACKETSIN,Stats::NUMPACKETSOUT);
		row=addStat(html,row,totalIn,totalOut,"Ping",Stats::NUMPINGIN,Stats::NUMPINGOUT);
		row=addStat(html,row,totalIn,totalOut,"Pong",Stats::NUMPONGIN,Stats::NUMPONGOUT);
		row=addStat(html,row,totalIn,totalOut,"Push",Stats::NUMPUSHIN,Stats::NUMPUSHOUT);
		row=addStat(html,row,totalIn,totalOut,"Query",Stats::NUMQUERYIN,Stats::NUMQUERYOUT);
		row=addStat(html,row,totalIn,totalOut,"Hit",Stats::NUMHITIN,Stats::NUMHITOUT);
		row=addStat(html,row,totalIn,totalOut,"Other",Stats::NUMOTHERIN,Stats::NUMOTHEROUT);
		row=addStat(html,row,totalIn,totalOut,"Accepted",Stats::NUMACCEPTED,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Dropped",Stats::NUMDROPPED,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Duplicate",Stats::NUMDUP,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Old",Stats::NUMOLD,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Dead",Stats::NUMDEAD,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Routed",Stats::NUMROUTED,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Broadcasted",Stats::NUMBROADCASTED,Stats::NONE);
		row=addStat(html,row,totalIn,totalOut,"Discarded",Stats::NUMDISCARDED,Stats::NONE);

		html.startTableRow(row++);
			html.startTagEnd("td","Avg. Size"); 

			if (totalIn)
				html.startTagEnd("td","%d",stats.getCurrent(Stats::PACKETDATAIN)/totalIn);
			else
				html.startTagEnd("td","-");
			html.startTagEnd("td","-");

			if (totalOut)
				html.startTagEnd("td","%d",stats.getCurrent(Stats::PACKETDATAOUT)/totalOut);
			else
				html.startTagEnd("td","-");
			html.startTagEnd("td","-");
		html.end();


		int i;
		for(i=0; i<10; i++)
		{
			char str[64];
			sprintf(str,"Hops %d",i+1);
			row=addStat(html,row,totalIn,totalOut,str,(Stats::STAT)((int)Stats::NUMHOPS1+i),Stats::NONE);
		}

		if (totalIn)
		{
			for(i=0; i<servMgr->numVersions; i++)
			{
				html.startTableRow(row++);
					html.startTagEnd("td","v%05X",servMgr->clientVersions[i]); 
					html.startTagEnd("td","%d",servMgr->clientCounts[i]);
					html.startTagEnd("td","%d",(servMgr->clientCounts[i]*100)/totalIn);
					html.startTagEnd("td","-");
					html.startTagEnd("td","-");
				html.end();
			}
		}



		html.startTableRow(row++);
			html.startTagEnd("td colspan=\"5\"","<a href=\"/admin?cmd=clear&packets=1\">Reset</a>");
		html.end();

	html.end();

	addFooter(html);
}	

// -----------------------------------
void Servent::addInformation(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Information</b>"); 
		html.end();


		int row=0;

		// server IP
		char ipStr[64];
		servMgr->serverHost.IPtoStr(ipStr);
		html.startTableRow(row++);
			html.startTagEnd("td","Server IP");
			html.startTagEnd("td","%s",ipStr);
		html.end();

		// uptime
		html.startTableRow(row++);
			html.startTagEnd("td","Uptime");
			String upt;
			upt.setFromStopwatch(servMgr->getUptime());
			upt.convertTo(String::T_HTML);
			html.startTagEnd("td",upt.cstr());
		html.end();

		// channels found
		html.startTableRow(row++);
			html.startTagEnd("td","Channels found");
			html.startTagEnd("td","%d",chanMgr->numHitLists());
		html.end();

		// channels relayed
		html.startTableRow(row++);
			html.startTagEnd("td","Total relays");
			html.startTagEnd("td","%d / %d",chanMgr->numRelayed(),chanMgr->numChannels());
		html.end();

		// direct listeners 
		html.startTableRow(row++);
			html.startTagEnd("td","Total listeners");
			html.startTagEnd("td","%d",chanMgr->numListeners());
		html.end();

		// total streams
		html.startTableRow(row++);
			html.startTagEnd("td","Total streams");
			html.startTagEnd("td","%d / %d",servMgr->numStreams(false),servMgr->numStreams(true));
		html.end();

		// total connected
		html.startTableRow(row++);
			html.startTagEnd("td","Total connected");
			html.startTagEnd("td","%d",servMgr->totalConnected());
		html.end();

		// outgoing
//		html.startTableRow(row++);
//			html.startTagEnd("td","Num outgoing");
//			html.startTagEnd("td","%d",servMgr->numConnected(T_OUTGOING));
//		html.end();

		// host cache
		html.startTableRow(row++);
			html.startTagEnd("td","Host cache (Servents)");
			html.startTagEnd("td","%d - <a href=\"/admin?cmd=clear&hostcache=1\">Clear</a>",servMgr->numHosts(ServHost::T_SERVENT));
		html.end();

		// XML stats
		html.startTableRow(row++);
			html.startTagEnd("td","XML stats");
			html.startTagEnd("td","<a href=\"/admin?cmd=viewxml\">View</a>");
		html.end();

		// Network stats
		html.startTableRow(row++);
			html.startTagEnd("td","Network stats");
			html.startTagEnd("td","<a href=\"/admin?page=viewnet\">View</a>");
		html.end();

	html.end();

}


// --------------------------------------
void printTest(const char *tag, const char *fmt,...)
{
	if (fmt)
	{

		//va_list ap;
  		//va_start(ap, fmt);

		//char tmp[512];
		printf("%f",10);
		//vsprintf(tmp,fmt,ap);
		//startNode(tag,tmp);

	   	//va_end(ap);	
	}else{
		//startNode(tag,NULL);
	}
	//end();
}

// -----------------------------------
void Servent::addStatistics(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");
		int row;

		row = 0;
		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"4\"","<b>Bandwidth</b>"); 
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td"," "); 
			html.startTagEnd("td","<b>In</b>"); 
			html.startTagEnd("td","<b>Out</b>"); 
			html.startTagEnd("td","<b>Total</b>"); 
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Total (Kbit/s)"); 
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESIN)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESOUT)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESIN) + stats.getPerSecond(Stats::BYTESOUT)));
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Internet (Kbit/s)"); 
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN) + stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT)));
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Network (Kbit/s)"); 
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::PACKETDATAIN)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::PACKETDATAOUT)));
			html.startTagEnd("td","%.1f",BYTES_TO_KBPS(stats.getPerSecond(Stats::PACKETDATAIN)+stats.getPerSecond(Stats::PACKETDATAOUT)));
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Packets/sec"); 
			html.startTagEnd("td","%d",stats.getPerSecond(Stats::NUMPACKETSIN));
			html.startTagEnd("td","%d",stats.getPerSecond(Stats::NUMPACKETSOUT));
			html.startTagEnd("td","%d",stats.getPerSecond(Stats::NUMPACKETSIN)+stats.getPerSecond(Stats::NUMPACKETSOUT));
		html.end();

	html.end();


}
// -----------------------------------
void Servent::addServerOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Server</b>"); 
		html.end();

		int row=0;

#if 0
		// is active
		html.startTableRow(row++);
			html.startTagEnd("td","Active");
			html.startTag("td");
			html.startSingleTagEnd("input type=\"radio\" name=\"serveractive\" value=\"1\" %s",servMgr->server?"checked=\"1\"":"");
			html.startTagEnd("b","On");
			html.startSingleTagEnd("input type=\"radio\" name=\"serveractive\" value=\"0\" %s",!servMgr->server?"checked=\"1\"":"");
			html.startTagEnd("b","Off");
			html.end();
		html.end();

		// port
		html.startTableRow(row++);
			html.startTagEnd("td","Port");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"port\" size=\"5\" type=\"text\" value=\"%d\"",servMgr->serverHost.port);
			html.end();
		html.end();

		// fixed ip
		html.startTableRow(row++);
			html.startTagEnd("td","Fixed IP");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"forceip\" type=\"text\" value=\"%s\"",servMgr->forceIP);
			html.end();
		html.end();

#endif

		// password
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Password");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"passnew\" size=\"10\" type=\"password\" value=\"%s\"",servMgr->password);
			html.end();
		html.end();



		// firewall
		html.startTableRow(row++);
			
			html.startTagEnd("td","Type");
			{
				switch (servMgr->getFirewall())
				{
					case ServMgr::FW_ON:
						html.startTagEnd("td","Firewalled");
						break;
					case ServMgr::FW_OFF:
						html.startTagEnd("td","Normal");
						break;
					default:
						html.startTagEnd("td","Unknown");
						break;
				}
			//}else{
			//	html.startTagEnd("td","Inactive");
			}
		html.end();


		// icy meta interval
		html.startTableRow(row++);
			html.startTagEnd("td","ICY MetaInterval");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"icymeta\" size=\"5\" type=\"text\" value=\"%d\"",chanMgr->icyMetaInterval);
			html.end();
		html.end();

		// mode
		html.startTableRow(row++);
			html.startTagEnd("td","Mode");
			html.startTag("td");
			html.startSingleTagEnd("input type=\"radio\" name=\"root\" value=\"0\" %s",!servMgr->isRoot?"checked=\"1\"":"","Normal");
			html.startTagEnd("i","Normal<br>");
			html.startSingleTagEnd("input type=\"radio\" name=\"root\" value=\"1\" %s",servMgr->isRoot?"checked=\"1\"":"");
			html.startTagEnd("i","Root");
			html.end();
		html.end();

		// refresh HTML
		html.startTableRow(row++);
			html.startTagEnd("td","Refresh HTML (sec)");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"refresh\" size=\"5\" type=\"text\" value=\"%d\"",servMgr->refreshHTML);
			html.end();
		html.end();




	html.end();

}
// -----------------------------------
void Servent::addClientOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Client</b>"); 
		html.end();

		int row=0;

#if 0
		// is active
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Auto Connect");
			html.startTag("td");
			html.startSingleTagEnd("input type=\"radio\" name=\"clientactive\" value=\"1\" %s",servMgr->autoConnect?"checked=\"1\"":"");
			html.startTagEnd("i","On");
			html.startSingleTagEnd("input type=\"radio\" name=\"clientactive\" value=\"0\" %s",!servMgr->autoConnect?"checked=\"1\"":"");
			html.startTagEnd("i","Off");
			html.end();
		html.end();
#endif


#if 0
		// dead hit age
		html.startTableRow(row++);
			html.startTagEnd("td","Dead Hit Age (sec)");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"deadhitage\" size=\"5\" type=\"text\" value=\"%d\"",chanMgr->deadHitAge);
			html.end();
		html.end();
#endif


	html.end();
}
// -----------------------------------
void Servent::addBroadcasterOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Broadcasting</b>"); 
		html.end();

		int row=0;

		// YP
		html.startTableRow(row++);
			html.startTagEnd("td","YP Address");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"yp\" type=\"text\" value=\"%s\"",servMgr->rootHost.cstr());
			html.end();
		html.end();


		// DJ message
		html.startTableRow(row++);
			html.startTagEnd("td","DJ Message");
			html.startTag("td");
			{
				String djMsg = chanMgr->broadcastMsg;
				djMsg.convertTo(String::T_HTML);
				html.startSingleTagEnd("input name=\"djmsg\" type=\"text\" value=\"%s\"",djMsg.cstr());
			}
			html.end();
		html.end();



	html.end();
}
// -----------------------------------
void Servent::addRelayOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Network</b>"); 
		html.end();

		int row=0;


		// max streams
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Max. Total Streams");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"maxstream\" size=\"5\" type=\"text\" value=\"%d\"",servMgr->maxStreams);
			html.end();
		html.end();

		// max streams/channel
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Max. Streams Per Channel");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"maxlisten\" size=\"5\" type=\"text\" value=\"%d\"",chanMgr->maxStreamsPerChannel);
			html.end();
		html.end();

		// max bitrate out
		html.startTableRow(row++);
			html.startTagEnd("td","Max. Output (Kbits/s)");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"maxup\" size=\"5\" type=\"text\" value=\"%d\"",servMgr->maxBitrate);
			html.end();
		html.end();

		// max control connections
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Max. Controls In");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"maxcin\" size=\"5\" type=\"text\" value=\"%d\"",servMgr->maxControl);
			html.end();
		html.end();




	html.end();
}

// -----------------------------------
void Servent::addFilterOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"5\" ","<b>Filters</b>"); 
		html.end();

		int row=0;


		// ip
		html.startTableRow(row++);
			html.startTagEnd("td","<b>IP Mask</b>");
			html.startTagEnd("td","<b>Network</b>");
			html.startTagEnd("td","<b>Direct</b>");
			html.startTagEnd("td","<b>Private</b>");
			html.startTagEnd("td","<b>Ban</b>");
		html.end();

		// filters
		for(int i=0; i<servMgr->numFilters+1; i++)
		{
			ServFilter *f = &servMgr->filters[i];
			int fl = f->flags;
			char ipstr[64];
			f->host.IPtoStr(ipstr);

			if (i == servMgr->numFilters)
				ipstr[0] = 0;

			html.startTableRow(row++);
				html.startTag("td");
					html.startSingleTagEnd("input name=\"filt_ip%d\" type=\"text\" value=\"%s\"",i,ipstr);
				html.end();
				html.startTag("td");
					html.startSingleTagEnd("input type=\"checkbox\" name=\"filt_nw%d\" value=\"1\" %s",i,fl & ServFilter::F_NETWORK?"checked=\"1\"":"");
				html.end();
				html.startTag("td");
					html.startSingleTagEnd("input type=\"checkbox\" name=\"filt_di%d\" value=\"1\" %s",i,fl & ServFilter::F_DIRECT?"checked=\"1\"":"");
				html.end();
				html.startTag("td");
					html.startSingleTagEnd("input type=\"checkbox\" name=\"filt_pr%d\" value=\"1\" %s",i,fl & ServFilter::F_PRIVATE?"checked=\"1\"":"");
				html.end();
				html.startTag("td");
					html.startSingleTagEnd("input type=\"checkbox\" name=\"filt_bn%d\" value=\"1\" %s",i,fl & ServFilter::F_BAN?"checked=\"1\"":"");
				html.end();
			html.end();
		}


	html.end();
}

// -----------------------------------
void Servent::addLogOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Log</b>"); 
		html.end();

		int row=0;



		// Debug
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Debug");
			html.startTag("td");
			html.startSingleTagEnd("input type=\"checkbox\" name=\"logDebug\" value=\"1\" %s",servMgr->showLog&(1<<LogBuffer::T_DEBUG)?"checked=\"1\"":"");
			html.end();
		html.end();

		// Errors
		html.startTableRow(row++);
			html.startTagEnd("td","Errors");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"logErrors\" value=\"1\" %s",servMgr->showLog&(1<<LogBuffer::T_ERROR)?"checked=\"1\"":"");
			html.end();
		html.end();

		// Network
		html.startTableRow(row++);
			html.startTagEnd("td","Network");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"logNetwork\" value=\"1\" %s",servMgr->showLog&(1<<LogBuffer::T_NETWORK)?"checked=\"1\"":"");
			html.end();
		html.end();

		// channels
		html.startTableRow(row++);
			html.startTagEnd("td","Channels");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"logChannel\" value=\"1\" %s",servMgr->showLog&(1<<LogBuffer::T_CHANNEL)?"checked=\"1\"":"");
			html.end();
		html.end();

	html.end();
}
// -----------------------------------
void Servent::addRootOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Root Mode</b>"); 
		html.end();

		int row=0;

		// host update interval
		html.startTableRow(row++);
			html.startTagEnd("td","Host Update (sec)");
			html.startTag("td");
				html.startSingleTagEnd("input name=\"huint\" size=\"5\" type=\"text\" value=\"%d\"",chanMgr->hostUpdateInterval);
			html.end();
		html.end();

		// Message
		html.startTableRow(row++);
			html.startTagEnd("td","Message");
			html.startTag("td");
				String pcMsg = servMgr->rootMsg;
				pcMsg.convertTo(String::T_HTML);
				html.startSingleTagEnd("input name=\"pcmsg\" size=\"50\" type=\"text\" value=\"%s\"",pcMsg.cstr());
			html.end();
		html.end();

		// get update
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Get Update");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"getupd\" value=\"1\"");
			html.end();
		html.end();

		// broadcast settings
		html.startTableRow(row++);
			html.startTagEnd("td width=\"50%%\" ","Send");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"brroot\" value=\"1\"");
			html.end();
		html.end();




	html.end();
}
// -----------------------------------
void Servent::addAuthOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");

		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"2\" ","<b>Authentication</b>"); 
		html.end();

		int row=0;

		html.startTableRow(row++);
			html.startTagEnd("td","HTML Authentication");
				html.startTag("td");
					html.startSingleTagEnd("input type=\"radio\" name=\"auth\" value=\"cookie\" %s",servMgr->authType==ServMgr::AUTH_COOKIE?"checked=\"1\"":"");
					html.startTagEnd("i","Cookies<br>");
					html.startSingleTagEnd("input type=\"radio\" name=\"auth\" value=\"http\" %s",servMgr->authType==ServMgr::AUTH_HTTPBASIC?"checked=\"1\"":"");
					html.startTagEnd("i","Basic HTTP");
			html.end();
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Cookies Expire");
				html.startTag("td");
					html.startSingleTagEnd("input type=\"radio\" name=\"expire\" value=\"session\" %s",servMgr->cookieList.neverExpire==false?"checked=\"1\"":"");
					html.startTagEnd("i","End of session<br>");
					html.startSingleTagEnd("input type=\"radio\" name=\"expire\" value=\"never\" %s",servMgr->cookieList.neverExpire==true?"checked=\"1\"":"");
					html.startTagEnd("i","Never");
			html.end();
		html.end();


	html.end();
}	
// -----------------------------------
void Servent::addSecurityOptions(HTML &html)
{
	html.startTag("table width=\"100%%\" border=\"0\" align=\"center\"");
		
		html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
			html.startTagEnd("td colspan=\"3\" ","<b>Security</b>"); 
		html.end();

		int row=0;


		html.startTableRow(row++);
			html.startTagEnd("td","<b>Allow on port:</b>");
			html.startTagEnd("td","<b>%d<b>",servMgr->serverHost.port);
			html.startTagEnd("td","<b>%d<b>",servMgr->serverHost.port+1);
		html.end();

		unsigned int a1 = servMgr->allowServer1;
		unsigned int a2 = servMgr->allowServer2;

		// port 1
		html.startTableRow(row++);
			html.startTagEnd("td","HTML");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowHTML1\" value=\"1\" %s",a1&ALLOW_HTML?"checked=\"1\"":"");
				//html.addOptionBox("allowHTML1",s1->isAllowed(ALLOW_HTML)?0:1,"Allow","1","Deny","0",NULL);
			html.end();
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowHTML2\" value=\"1\" %s",a2&ALLOW_HTML?"checked=\"1\"":"");
				//html.addOptionBox("allowHTML2",s2->isAllowed(ALLOW_HTML)?0:1,"Allow","1","Deny","0",NULL);
			html.end();
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Broadcasting");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowBroadcast1\" value=\"1\" %s",a1&ALLOW_BROADCAST?"checked=\"1\"":"");
			html.end();
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowBroadcast2\" value=\"1\" %s",a2&ALLOW_BROADCAST?"checked=\"1\"":"");
			html.end();
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Network");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowNetwork1\" value=\"1\" %s",a1&ALLOW_NETWORK?"checked=\"1\"":"");
			html.end();
			html.startTag("td");
				//html.startSingleTagEnd("input type=\"checkbox\" name=\"allowNetwork2\" value=\"1\" %s",a2&ALLOW_NETWORK?"checked=\"1\"":"");
			html.end();
		html.end();

		html.startTableRow(row++);
			html.startTagEnd("td","Direct");
			html.startTag("td");
				html.startSingleTagEnd("input type=\"checkbox\" name=\"allowDirect1\" value=\"1\" %s",a1&ALLOW_DIRECT?"checked=\"1\"":"");
			html.end();
			html.startTag("td");
				//html.startSingleTagEnd("input type=\"checkbox\" name=\"allowDirect2\" value=\"1\" %s",a2&ALLOW_DIRECT?"checked=\"1\"":"");
			html.end();
		html.end();


	html.end();
}
// -----------------------------------
void Servent::addConnectionsPage(HTML &html)
{

	html.setRefresh(servMgr->refreshHTML);
	addHeader(html,4);

	html.startTag("table border=\"0\" width=\"95%%\" align=\"center\"");

		html.startTag("form method=\"get\" action=\"/admin\"");

			html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
				html.startTagEnd("td",""); 
				html.startTagEnd("td","<b>Type<b>"); 
				html.startTagEnd("td","<b>Status</b>"); 
				html.startTagEnd("td","<b>Time</b>"); 
				html.startTagEnd("td","<b>IP:Port (net)</b>"); 
				html.startTagEnd("td","<b>In (pack/s)</b>"); 
				html.startTagEnd("td","<b>Out (pack/s)</b>"); 
				html.startTagEnd("td","<b>Queue<br>(nrm/pri)</b>"); 
				html.startTagEnd("td","<b>Route</b>"); 
				html.startTagEnd("td","<b>Agent</b>"); 
				html.startTagEnd("td","<b>Kbits/s</b>"); 
			html.end();


			int cnt=0;
			Servent *s = servMgr->servents;
			while (s)
			{
				if (s->type != Servent::T_NONE)
				{
					Host h = s->getHost();
					{
						int ip = h.ip;
						int port = h.port;

						Host h(ip,port);
						char hostName[256];
						h.toStr(hostName);

						if (s->priorityConnect)
							strcat(hostName,"*");

						if (s->networkID.isSet())
						{
							char netidStr[64];
							s->networkID.toStr(netidStr);
							strcat(hostName,"<br>");
							strcat(hostName,"(");
							strcat(hostName,netidStr);
							strcat(hostName,")");
						}

						unsigned int tnum = 0;
						char tdef = 's';
						if (s->lastConnect)
							tnum = sys->getTime()-s->lastConnect;

						html.startTableRow(cnt++);

						String tmp;
						html.startTag("td");
							html.startTag("b","");
								sprintf(tmp.data,"/admin?cmd=stopserv&index=%d",s->serventIndex);
								html.addLink(tmp.data,"Stop");
							html.end();
						html.end();

						if (s->type == Servent::T_STREAM)
						{
							html.startTagEnd("td",s->getTypeStr());
							html.startTagEnd("td",s->getStatusStr());
							html.startTagEnd("td","%d%c",tnum,tdef);
							html.startTagEnd("td",hostName);
							html.startTagEnd("td","-");
							html.startTagEnd("td","%d",s->syncPos);
							html.startTagEnd("td","-");
							html.startTagEnd("td","-");
						}else{

							html.startTagEnd("td",s->getTypeStr());
							html.startTagEnd("td",s->getStatusStr());
							html.startTagEnd("td","%d%c",tnum,tdef);
							html.startTagEnd("td",hostName);
							if (tnum)
							{
								html.startTagEnd("td","%d (%.1f)",s->gnuStream.packetsIn,((float)s->gnuStream.packetsIn)/((float)tnum));
								html.startTagEnd("td","%d (%.1f)",s->gnuStream.packetsOut,((float)s->gnuStream.packetsOut)/((float)tnum));
							}else{
								html.startTagEnd("td","-");
								html.startTagEnd("td","-");
							}
							//html.startTagEnd("td","%d / %d",s->outPacketsNorm.numPending(),s->outPacketsPri.numPending());
							html.startTagEnd("td","%s %d / %d",
								s->flowControl?"FC":"",
								s->outPacketsNorm.numPending(),	s->outPacketsPri.numPending()
								);

							int nr = s->seenIDs.numUsed();
							unsigned int tim = sys->getTime()-s->seenIDs.getOldest();
						
							String tstr;
							tstr.setFromStopwatch(tim);

							if (nr)
								html.startTagEnd("td","%s (%d)",tstr.cstr(),nr);
							else
								html.startTagEnd("td","-");

						}

						html.startTagEnd("td",s->agent.cstr());

						if (s->sock)
						{
							unsigned int tot = s->sock->bytesInPerSec+s->sock->bytesOutPerSec;
							html.startTagEnd("td","%.1f",BYTES_TO_KBPS(tot));
						}else
							html.startTagEnd("td","-");

						html.end();	// tr

					}
				}
				s=s->next;
			}
		html.end();
	html.end();

	addFooter(html);
}
// -----------------------------------
static int compareNamesDown(ChanHitList **c2, ChanHitList **c1)
{return stricmp(c1[0]->info.name.cstr(),c2[0]->info.name.cstr());}
static int compareBitratesDown(ChanHitList **c2, ChanHitList **c1)
{return c1[0]->info.bitrate-c2[0]->info.bitrate;}
static int compareListenersDown(ChanHitList **c2, ChanHitList **c1)
{return c1[0]->numListeners()-c2[0]->numListeners();}
static int compareHitsDown(ChanHitList **c2, ChanHitList **c1)
{return c1[0]->numHits()-c2[0]->numHits();}
static int compareTypesDown(ChanHitList **c2, ChanHitList **c1)
{return stricmp(ChanInfo::getTypeStr(c1[0]->info.contentType),ChanInfo::getTypeStr(c2[0]->info.contentType));}
static int compareGenresDown(ChanHitList **c2, ChanHitList **c1)
{return stricmp(c1[0]->info.genre.cstr(),c2[0]->info.genre.cstr());}

static int compareNamesUp(ChanHitList **c1, ChanHitList **c2)
{return stricmp(c1[0]->info.name.cstr(),c2[0]->info.name.cstr());}
static int compareBitratesUp(ChanHitList **c1, ChanHitList **c2)
{return c1[0]->info.bitrate-c2[0]->info.bitrate;}
static int compareListenersUp(ChanHitList **c1, ChanHitList **c2)
{return c1[0]->numListeners()-c2[0]->numListeners();}
static int compareHitsUp(ChanHitList **c1, ChanHitList **c2)
{return c1[0]->numHits()-c2[0]->numHits();}
static int compareTypesUp(ChanHitList **c1, ChanHitList **c2)
{return stricmp(ChanInfo::getTypeStr(c1[0]->info.contentType),ChanInfo::getTypeStr(c2[0]->info.contentType));}
static int compareGenresUp(ChanHitList **c1, ChanHitList **c2)
{return stricmp(c1[0]->info.genre.cstr(),c2[0]->info.genre.cstr());}


static COMPARE_FUNC2 compareFuncs[]=
{
	compareNamesDown,compareNamesUp,
	compareBitratesDown,compareBitratesUp,
	compareListenersDown,compareListenersUp,
	compareHitsDown,compareHitsUp,
	compareTypesDown,compareTypesUp,
	compareGenresDown,compareGenresUp,
};
// -----------------------------------
static void addChanInfoLink(HTML &html, ChanInfo &info,bool fromRelay)
{
	char tmp[128];
	char idstr[64];
	info.id.toStr(idstr);
	sprintf(tmp,"/admin?page=chaninfo&id=%s&relay=%d",idstr,fromRelay?1:0);
	html.addLink(tmp,"Info"); 
}
// -----------------------------------
void addChannelInfo(HTML &html, ChanInfo &info, bool fromRelay, bool showPlay, bool showInfo, bool showURL, bool showRelay)
{
	String name,url,desc;
	name = info.name;
	name.convertTo(String::T_HTML);
	TrackInfo track = info.track;
	track.convertTo(String::T_HTML);
	url = info.url;
	url.convertTo(String::T_ASCII);
	desc = info.desc;
	desc.convertTo(String::T_HTML);

	char idStr[64];
	info.id.toStr(idStr);
	{
		String tmp;
		sprintf(tmp.data,"/pls/%s",idStr);
		html.startTag("td align=\"left\"");

			html.startTagEnd("b",name.cstr());

			html.startTag("font size=\"-1\"","");
				if (!desc.isEmpty())
					html.startTagEnd("i","<br>%s",desc.cstr());

				if (!track.artist.isEmpty() || !track.title.isEmpty())
					html.startTagEnd("i","<br>(%s - %s)",track.artist.cstr(),track.title.cstr());
			html.end();

			html.startTag("font size=\"-1\"");
				if (showPlay)
				{
					html.startTag("b","<br>");
						html.addLink(tmp.data,"Play");
					html.end();
				}

				if ((!fromRelay) && (showRelay))
				{
					html.startTag("b"," - ");
						sprintf(tmp.data,"/admin?cmd=relay&id=%s",idStr);
						html.addLink(tmp.data,"Relay");
					html.end();
				}

				if (showRelay)
				{
					html.startTag("b"," - ");
						addChanInfoLink(html,info,fromRelay);
					html.end();
				}

				if ((!url.isEmpty()) && (showURL))
				{
					html.startTag("b"," - ");
					if (strstr(url.cstr(),"mailto:"))
					{
						html.addLink(url.cstr(),"MAIL");
					}else{
						String tmp;
						sprintf(tmp.data,"/admin?cmd=redirect&url=%s",url.cstr());
						html.addLink(tmp.data,"WWW",true);
					}
					html.end();
				}

				if (fromRelay)
				{
					html.startTag("b"," - ");
						sprintf(tmp.data,"/admin?cmd=bump&id=%s",idStr);
						html.addLink(tmp.data,"Bump");
					html.end();
					html.startTag("b"," - ");
						sprintf(tmp.data,"/admin?cmd=keep&id=%s",idStr);
						html.addLink(tmp.data,"Keep");
					html.end();
					html.startTag("b"," - ");
						sprintf(tmp.data,"/admin?cmd=stop&id=%s",idStr);
						html.addLink(tmp.data,"Stop");
					html.end();
				}

			html.end();


		html.end();
	}
}
// -----------------------------------
void Servent::addAllChannelsPage(HTML &html, SORT sort, bool dir, ChanInfo *info)
{
	//while (chanMgr->numFinds) 
	//	sys->sleepIdle();

	//bool showFind = ((sys->getTime()-chanMgr->lastHit)>15) && (!chanMgr->searchActive);
	bool showFind = true;

	if (!showFind)
		html.setRefresh(servMgr->refreshHTML);
	addHeader(html,2);


	//bool showFind = chanMgr->numFinds==0;

		html.startTag("table border=\"0\" align=\"center\"");
			//html.startTag("form method=\"get\" action=\"/admin\"");
			html.startTag("form method=\"get\" action=\"/admin\"");

			if (!showFind)
			{
				html.startTagEnd("input name=\"cmd\" type=\"hidden\" value=\"stopfind\"");					
				html.startTag("tr");
					html.startTag("td");
						html.startTagEnd("input name=\"stop\" type=\"submit\" value=\"  Stop Search \"");					
					html.end();
				html.end();
			}else
			{

				html.startTagEnd("input name=\"page\" type=\"hidden\" value=\"chans\"");					

				html.startTag("tr");
					html.startTagEnd("td");
					html.startTagEnd("td","Name");
					html.startTagEnd("td","Genre");
					html.startTagEnd("td","Bitrate");
					html.startTagEnd("td","ID");
				html.end();

				html.startTag("tr");
					html.startTag("td");
						html.startTagEnd("input name=\"find\" type=\"submit\" value=\"  Search  \"");					
					html.end();

					String name,genre;
					int bitrate;
					//ChanInfo *info = &chanMgr->searchInfo;
					
					name = info->name;
					genre = info->genre;
					bitrate = info->bitrate;
					name.convertTo(String::T_HTML);
					genre.convertTo(String::T_HTML);
					char idStr[64],brStr[64];

					if (info->id.isSet())
						info->id.toStr(idStr);
					else
						idStr[0] = 0;

					if (bitrate)
						sprintf(brStr,"%d",bitrate);
					else
						brStr[0] = 0;


					html.startTag("td");
						html.startSingleTagEnd("input name=\"name\" type=\"text\" value=\"%s\"",name.cstr());
					html.end();

					html.startTag("td");
						html.startSingleTagEnd("input name=\"genre\" type=\"text\" value=\"%s\"",genre.cstr());
					html.end();

					html.startTag("td");
						html.startSingleTagEnd("input name=\"bitrate\" size=\"5\"  type=\"text\" value=\"%s\"",brStr);
					html.end();

					html.startTag("td");
						html.startSingleTagEnd("input name=\"id\" size=\"34\" type=\"text\" value=\"%s\"",idStr);
					html.end();

				html.end();	// tr
			}
					
			html.end();	// form
		html.end();	// table


		html.startTag("table border=\"0\" width=\"95%%\" align=\"center\"");

#if 0
			html.startTag("form method=\"get\" action=\"/admin\"");

				html.startTag("tr");
					html.startTagEnd("input name=\"play\" type=\"submit\" value=\"Play selected\"");					
					html.startTagEnd("input name=\"relay\" type=\"submit\" value=\"Relay selected\"");					
					html.startTagEnd("input name=\"cmd\" type=\"hidden\" value=\"hitlist\"");					
				html.end();
#endif
				html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
#if 0
					html.startTagEnd("td width=\"1%%\"",""); 
#endif
					html.startTagEnd("td","<b><a href=\"/admin?page=chans&sort=name&dir=%s\">Channel</a></b>",dir?"down":"up"); 
					html.startTagEnd("td","<b><a href=\"/admin?page=chans&sort=genre&dir=%s\">Genre</a></b>",dir?"down":"up"); 
					html.startTagEnd("td width=\"1%%\"","<b><a href=\"/admin?page=chans&sort=bitrate&dir=%s\">Bitrate (kb/s)</a></b>",dir?"down":"up"); 
					html.startTagEnd("td width=\"1%%\"","<b><a href=\"/admin?page=chans&sort=type&dir=%s\">Type</a></b>",dir?"down":"up"); 
					html.startTagEnd("td width=\"2%%\"","<b><a href=\"/admin?page=chans&sort=hosts&dir=%s\">Hits</a></b>",dir?"down":"up"); 
				html.end();

			
				ChanHitList *hits[ChanMgr::MAX_HITLISTS];
				int numHits=0;
				int i;
				for(i=0; i<ChanMgr::MAX_HITLISTS; i++)
				{
					ChanHitList *chl = &chanMgr->hitlists[i];
					if (chl->isUsed())
						if (chl->info.match(*info))
							hits[numHits++] = chl;
				}

				if (!numHits)
				{
					html.startTag("tr");
						if (showFind)
							html.startTag("td","No channels found");
						else
							html.startTag("td","Searching...");
					html.end();
				}else
				{
					qsort(hits,numHits,sizeof(ChanHitList*),(COMPARE_FUNC)compareFuncs[sort*2 + (dir?0:1)]);

					for(i=0; i<numHits; i++)
					{
						ChanHitList *chl = hits[i];

						html.startTableRow(i);
	#if 0
							html.startTag("td");
								html.startSingleTagEnd("input type=\"checkbox\" name=\"c%d\" value=\"1\"",chl->index);
							html.end();
	#endif
							addChannelInfo(html,chl->info,false,true,true,true,true);

							String genre = chl->info.genre;
							genre.convertTo(String::T_HTML);
							html.startTagEnd("td",genre.cstr()); 

							html.startTagEnd("td","%d",chl->info.bitrate); 

							html.startTagEnd("td",ChanInfo::getTypeStr(chl->info.contentType)); 

							html.startTagEnd("td","%d / %d",chl->numListeners(),chl->numHits()); 

						html.end();	// tr
					}
				}
#if 0
			html.end();	// form
#endif
		html.end();	// table
	//}else{
	//	html.startTagEnd("h3","No channels found.");
	//}

	addFooter(html);
}

// -----------------------------------
void Servent::addWinampChansPage(HTML &html, const char *wildcard, const char *type, bool stop)
{
	int maxHits=ChanMgr::MAX_HITLISTS;

	//bool stop = (wildcard==NULL) || (type==NULL);

	if (!strlen(type))
		type = "name";

	if ((chanMgr->numHitLists() < maxHits) && (!stop))
		html.setRefresh(servMgr->refreshHTML);
	addBasicHeader(html);


	ChanHitList *hits[ChanMgr::MAX_HITLISTS];
	int numHits=0;
	int i;

	ChanInfo searchInfo;
	searchInfo.init();

	if (strcmp(type,"name")==0)
		searchInfo.name.set(wildcard);
	else if (strcmp(type,"genre")==0)
		searchInfo.genre.set(wildcard);
	else if (strcmp(type,"bitrate")==0)
		searchInfo.bitrate = atoi(wildcard);

	for(i=0; i<ChanMgr::MAX_HITLISTS; i++)
	{
		ChanHitList *chl = &chanMgr->hitlists[i];
		if (chl->isUsed() && chl->isAvailable())
			if (chl->info.match(searchInfo))
				hits[numHits++] = chl;
	}

	bool maxShown=false;
	if (numHits >= maxHits)
	{
		numHits = maxHits;
		maxShown = true; 
	}


	if (servMgr->downloadURL[0])
	{

		html.startTag("font color=\"#FF0000\"");
			html.startTag("div align=\"center\"");
				html.startTagEnd("b","! Attention !");
			html.end();
		html.end();

		html.startTag("b");
			html.startTag("div align=\"center\"");
				html.addLink("/admin?cmd=upgrade","Click here to update your client",true);
			html.end();
		html.end();
	}

	if (!servMgr->rootMsg.isEmpty())
	{
		String pcMsg = servMgr->rootMsg;
		pcMsg.convertTo(String::T_HTML);
		html.startTag("div align=\"center\"");
			html.startTagEnd("b",pcMsg.cstr());
		html.end();
	}


//	html.startTagEnd("b","-%s -%s",wildcard,type);

	html.startTag("form method=\"get\" action=\"/admin?\"");

		html.startSingleTagEnd("input name=\"page\" type=\"hidden\" value=\"winamp-chans\"");					
		if (!stop)
		{
			html.startTagEnd("input name=\"stop\" type=\"submit\" value=\"Stop Search\"");					
			html.startSingleTagEnd("input name=\"wildcard\" type=\"hidden\" value=\"%s\"",wildcard);					
			html.startSingleTagEnd("input name=\"type\" type=\"hidden\" value=\"%s\"",type);					
		}else
		{
			char optStr[256];

			html.startTag("select name=\"type\"");
				sprintf(optStr,"option value=\"name\" %s",(strcmp(type,"name")==0)?"selected":"");
				html.startTagEnd(optStr,"Name");
				sprintf(optStr,"option value=\"genre\" %s",(strcmp(type,"genre")==0)?"selected":"");
				html.startTagEnd(optStr,"Genre");
				sprintf(optStr,"option value=\"bitrate\" %s",(strcmp(type,"bitrate")==0)?"selected":"");
				html.startTagEnd(optStr,"BitRate");
			html.end();

			html.startSingleTagEnd("input name=\"wildcard\" type=\"text\" value=\"%s\"",wildcard);					
			html.startTagEnd("input name=\"search\" type=\"submit\" value=\"Search\"");					
		}

		//html.startTagEnd("input name=\"search\" type=\"submit\" value=\"Stop Search\"");					
	html.end();


#if 1
		html.startTag("table border=\"0\" width=\"100%%\" align=\"center\"");

				html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
					html.startTagEnd("td width=\"5%%\"","<b><font size=\"-1\">Play</font></b>"); 
					//html.startTagEnd("td width=\"20%%\"","<b><font size=\"-1\">Type</font></b>"); 
					html.startTagEnd("td width=\"95%%\"","<b><font size=\"-1\">PeerCast Channel</font></b>"); 
				html.end();

			
				if (numHits)
				{

					qsort(hits,numHits,sizeof(ChanHitList*),(COMPARE_FUNC)compareNamesUp);

					for(i=0; i<numHits; i++)
					{
						ChanHitList *chl = hits[i];

						html.startTableRow(i);

							char idStr[64];
							chl->info.id.toStr(idStr);
							String playURL;
							sprintf(playURL.data,"/pls/%s",idStr);

							String genre = chl->info.genre;
							genre.convertTo(String::T_HTML);
							String name = chl->info.name;
							name.convertTo(String::T_HTML);
							html.startTagEnd("td","<font face=\"Webdings\" size=\"+2\"><a href=\"%s\">U</a></font>",playURL.cstr());
							//html.startTagEnd("td","<font size=\"-1\">%s %d kb/s</font>",chl->info.getTypeStr(),chl->info.bitrate);

							html.startTagEnd("td","<font size=\"-1\"><b>%s</b><br>%s %d kb/s - (%s)</font>",name.cstr(),ChanInfo::getTypeStr(chl->info.contentType),chl->info.bitrate,genre.cstr());

							//addChannelInfoShort(html,chl->info);

							//String genre = chl->info.genre;
							//genre.convertTo(String::T_HTML);
							//html.startTagEnd("td",genre.cstr()); 

							//html.startTagEnd("td","%d",chl->info.bitrate); 

							//html.startTagEnd("td",chl->info.getTypeStr()); 

						html.end();	// tr
					}

				}
		html.end();	// table


	if (stop)
		html.startTagEnd("b","<font size=\"-1\">Displayed %d out of %d channels.</b>",numHits,chanMgr->numHitLists());
	else if (!stop)
		html.startTagEnd("b","Searching...");

#endif

	addFooter(html);
}

// -----------------------------------
void Servent::addMyChannelsPage(HTML &html)
{
	html.setRefresh(servMgr->refreshHTML);
	addHeader(html,3);

	Channel *clist[ChanMgr::MAX_CHANNELS];


	ChanInfo info;
	int num = chanMgr->findChannels(info,clist,ChanMgr::MAX_CHANNELS);

	//if (num)
	if (1)
	{		
		//int totListen=0;

		html.startTag("table border=\"0\" width=\"95%%\" align=\"center\"");

					html.startTagEnd("td","<b>Channel</b>"); 
					html.startTagEnd("td","<b>Genre</b>"); 
					html.startTagEnd("td width=\"1%%\"","<b>Bitrate (kb/s)</b>"); 
					html.startTagEnd("td width=\"1%%\"","<b>Stream</b>"); 
					html.startTagEnd("td width=\"2%%\"","<b>Relays</b>"); 
					html.startTagEnd("td width=\"2%%\"","<b>Listeners</b>"); 
					html.startTagEnd("td width=\"1%%\"","<b>Status</b>"); 
					html.startTagEnd("td width=\"1%%\"","<b>Keep</b>"); 
				html.end();


				for(int i=0; i<num; i++)
				{
					Channel *c = clist[i];

					char idStr[64];
					c->getIDStr(idStr);

					html.startTableRow(i);


					String genre = c->info.genre;
					genre.convertTo(String::T_HTML);

					addChannelInfo(html,c->info,true,true,true,true,true);

					html.startTagEnd("td",genre.cstr()); 

					// bitrate
					if (c->getBitrate())
						html.startTagEnd("td","%d",c->getBitrate()); 
					else
						html.startTagEnd("td","-"); 



					// stream/type
					html.startTag("td align=\"center\""); 
					{
						String path;
						c->getStreamPath(path.data);
						html.addLink(path.cstr(),ChanInfo::getTypeStr(c->info.contentType));						
					}
					html.end();


					// relays
					html.startTagEnd("td","%d",c->numRelays()); 
					// listeners
					html.startTagEnd("td","%d",c->numListeners()); 
					// status
					html.startTagEnd("td",c->getStatusStr()); 
					// keep
					html.startTagEnd("td",c->stayConnected?"Yes":"No"); 

					html.end();
					//totListen += c->listeners;
				}
		html.end();	// table
	
	}else{
		html.startTagEnd("h3","No channels available.");
	}
	addFooter(html);
}
// -----------------------------------
void Servent::addBroadcastPage(HTML &html)
{
	addHeader(html,8);

	Channel *clist[ChanMgr::MAX_CHANNELS];


	ChanInfo info;
	int num = chanMgr->findChannels(info,clist,ChanMgr::MAX_CHANNELS);

	//if (num)
	if (1)
	{		
		//int totListen=0;


		html.startTag("table border=\"0\" align=\"center\"");
			html.startTag("form method=\"get\" action=\"/admin\"");
				html.startSingleTagEnd("input name=\"cmd\" type=\"hidden\" value=\"fetch\"");					

				html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
					html.startTagEnd("td colspan=\"2\" ","<b>External Source</b>"); 
				html.end();

				int row=0;

				html.startTableRow(row++);
					html.startTagEnd("td","URL (Required)"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"url\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Name"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"name\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Description"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"desc\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Genre"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"genre\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Contact"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"contact\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Bitrate (kb/s)"); 
					html.startTag("td");
						html.startSingleTagEnd("input name=\"bitrate\" size=\"40\" type=\"text\" value=\"%s\"","");
					html.end();
				html.end();

				html.startTableRow(row++);
					html.startTagEnd("td","Type"); 
					html.startTag("td");
						html.startTag("select name=\"type\"");
							html.startTagEnd("option value=\"UNKNOWN\" selected","Unknown");
							html.startTagEnd("option value=\"MP3\"","MP3");
							html.startTagEnd("option value=\"OGG\"","OGG");
							html.startTagEnd("option value=\"WMA\"","WMA");
							html.startTagEnd("option value=\"NSV\"","NSV");
							html.startTagEnd("option value=\"WMV\"","WMV");
							html.startTagEnd("option value=\"RAW\"","RAW");
						html.end();
					html.end();
				html.end();


				html.startTableRow(row++);
					html.startTag("td colspan=\"2\" align=\"center\"");
						html.startSingleTagEnd("input name=\"stream\" type=\"submit\" value=\"Create Relay\"");					
					html.end();
				html.end();

			html.end();
		html.end();

		html.startTagEnd("br"); 


		html.startTag("table border=\"0\" width=\"95%%\" align=\"center\"");

				html.startTag("tr bgcolor=\"#cccccc\" align=\"center\"");
					html.startTagEnd("td","<b>Channel</b>"); 
					html.startTagEnd("td","<b>Source</b>"); 
					html.startTagEnd("td","<b>Pos</b>"); 
					html.startTagEnd("td","<b>Bitrate (kb/s)</b>"); 
					html.startTagEnd("td","<b>Type</b>"); 
				html.end();


				for(int i=0; i<num; i++)
				{
					Channel *c = clist[i];
					if (c->isBroadcasting())
					{
						unsigned int uptime = c->info.lastPlayTime?(sys->getTime()-c->info.lastPlayTime):0;
						String uptimeStr;
						uptimeStr.setFromStopwatch(uptime);

						char idStr[64];
						c->getIDStr(idStr);

						html.startTableRow(i);

						String name = c->info.name;
						name.convertTo(String::T_HTML);

						char ipStr[64];
						servMgr->serverHost.toStr(ipStr);

						// use global peercast URL as name link
						String temp;
						html.startTag("td");
							sprintf(temp.data,"peercast://pls/%s?ip=%s",idStr,ipStr);
							html.addLink(temp.data,name.cstr());
						html.end();

						addChannelSourceTag(html,c);

						html.startTagEnd("td","%d",c->streamPos); 
						
						if (c->getBitrate())
							html.startTagEnd("td","%d",c->getBitrate()); 
						else
							html.startTagEnd("td","-"); 

						html.startTagEnd("td",ChanInfo::getTypeStr(c->info.contentType)); 

						html.end();
						//totListen += c->listeners;
					}
				}
		html.end();	// table


	
	}
	addFooter(html);
}

