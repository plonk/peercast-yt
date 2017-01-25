// ------------------------------------------------
// File : gnutella.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		GnuPacket is a Gnutella protocol packet.
//		GnuStream is a Stream that reads/writes GnuPackets
//
//
// (c) 2002 peercast.org
// 
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

#include "gnutella.h"
#include "stream.h"
#include "common.h"
#include "servent.h"
#include "servmgr.h"
#include "stats.h"
#include <stdlib.h>

// ---------------------------
const char *GNU_FUNC_STR(int func)
{
	switch (func)
	{
		case GNU_FUNC_PING: return "PING"; break;
		case GNU_FUNC_PONG: return "PONG"; break;
		case GNU_FUNC_QUERY: return "QUERY"; break;
		case GNU_FUNC_HIT: return "HIT"; break;
		case GNU_FUNC_PUSH: return "PUSH"; break;
		default: return "UNKNOWN"; break;
	}
}

// ---------------------------
const char *GnuStream::getRouteStr(R_TYPE r)
{
	switch(r)
	{
		case R_PROCESS: return "PROCESS"; break;
		case R_DEAD: return "DEAD"; break;
		case R_DISCARD: return "DISCARD"; break;
		case R_ACCEPTED: return "ACCEPTED"; break;
		case R_BROADCAST: return "BROADCAST"; break;
		case R_ROUTE: return "ROUTE"; break;
		case R_DUPLICATE: return "DUPLICATE"; break;
		case R_BADVERSION: return "BADVERSION"; break;
		case R_DROP: return "DROP"; break;
		default: return "UNKNOWN"; break;
	}
}
// ---------------------------
void GnuPacket::makeChecksumID()
{
	for(unsigned int i=0; i<len; i++)
		id.id[i%16] += data[i];	
}

// ---------------------------
void GnuPacket::initPing(int t)
{
	func = GNU_FUNC_PING;
	ttl = t;
	hops = 0;
	len = 0;

	id.generate();
}
// ---------------------------
void GnuPacket::initPong(Host &h, bool ownPong, GnuPacket &ping)
{
	func = GNU_FUNC_PONG;
	ttl = ping.hops;
	hops = 0;
	len = 14;
	id = ping.id;

	MemoryStream pk(data,len);

	pk.writeShort(h.port);		// port
	pk.writeLong(SWAP4(h.ip));	// ip
	if (ownPong)
	{
		pk.writeLong(chanMgr->numChannels());	// cnt
		pk.writeLong(servMgr->totalOutput(false));	// total
	}else{
		pk.writeLong(0);				// cnt
		pk.writeLong(0);				// total
	}


}
// ---------------------------
void GnuPacket::initPush(ChanHit &ch, Host &sh)
{
#if 0
	func = GNU_FUNC_PUSH;
	ttl = ch.numHops;
	hops = 0;
	len = 26;
	id.generate();

	MemoryStream data(data,len);

	// ID of Hit packet
	data.write(ch.packetID.id,16);

	// index of channel
	data.writeLong(ch.index);

	data.writeLong(SWAP4(sh.ip));	// ip
	data.writeShort(sh.port);		// port
#endif
}


// ---------------------------
bool GnuPacket::initHit(Host &h, Channel *ch, GnuPacket *query, bool push, bool busy, bool stable, bool tracker, int maxttl)
{
	if (!ch)
		return false;

	func = GNU_FUNC_HIT;
	hops = 0;
	id.generate();

	ttl = maxttl;


	MemoryStream mem(data,MAX_DATA);

	mem.writeChar(1);			// num hits
	mem.writeShort(h.port);		// port
	mem.writeLong(SWAP4(h.ip));	// ip

	if (query)
		mem.writeLong(0);			// speed - route
	else
		mem.writeLong(1);			// broadcast

	
	//mem.writeLong(ch->index);				// index
	mem.writeLong(0);				// index
	mem.writeShort(ch->getBitrate());	// bitrate
	mem.writeShort(ch->localListeners());		// num listeners

	mem.writeChar(0);	// no name

	XML xml;
	XML::Node *cn = ch->info.createChannelXML();
	cn->add(ch->info.createTrackXML());
	xml.setRoot(cn);
	xml.writeCompact(mem);

	mem.writeChar(0);							// extra null 


	// QHD
	mem.writeLong('PCST');				// vendor ID		
	mem.writeChar(2);					// public sector length 

	int f1 = 0, f2 = 0;

	f1 = 1 | 4 | 8 | 32 | 64;	// use push | busy | stable | broadcast | tracker 

	if (push) f2 |= 1;			
	if (busy) f2 |= 4;
	if (stable) f2 |= 8;
	if (!query) f2 |= 32;
	if (tracker) f2 |= 64;
	
	mem.writeChar(f1);
	mem.writeChar(f2);

	{
		// write private sector
		char pbuf[256];
		MemoryStream pmem(pbuf,sizeof(pbuf));
		XML xml;
		XML::Node *pn = servMgr->createServentXML();
		xml.setRoot(pn);
		xml.writeCompact(pmem);
		pmem.writeChar(0);			// add null terminator
		if (pmem.pos <= 255)
		{
			mem.writeChar(pmem.pos);
			mem.write(pmem.buf,pmem.pos);
		}else
			mem.writeChar(0);
	}


	// queryID/not used
	if (query)
		mem.write(query->id.id,16);					
	else
		mem.write(id.id,16);					

	len = mem.pos;

	LOG_NETWORK("Created Hit packet: %d bytes",len);

	if (len >= MAX_DATA)
		return false;

//	servMgr->addReplyID(id);
	return true;
}


// ---------------------------
void GnuPacket::initFind(const char *str, XML *xml, int maxTTL)
{

	func = GNU_FUNC_QUERY;
	ttl = maxTTL;
	hops = 0;
	id.generate();

	MemoryStream mem(data,MAX_DATA);

	mem.writeShort(0);		// min speed

	if (str)
	{
		int slen = strlen(str);
		mem.write((void *)str,slen+1);	// string
	}else
		mem.writeChar(0);		// null string

	
	if (xml)
		xml->writeCompact(mem);

	len = mem.pos;
}

// ---------------------------
void GnuStream::ping(int ttl)
{
	GnuPacket ping;
	ping.initPing(ttl);
//	servMgr->addReplyID(ping.id);
	sendPacket(ping);
	LOG_NETWORK("ping out %02x%02x%02x%02x",ping.id.id[0],ping.id.id[1],ping.id.id[2],ping.id.id[3]);
}

// ---------------------------
void GnuStream::sendPacket(GnuPacket &p)
{
	try 
	{
		lock.on();
		packetsOut++;
		stats.add(Stats::NUMPACKETSOUT);

		switch(p.func)
		{
			case GNU_FUNC_PING: stats.add(Stats::NUMPINGOUT); break;
			case GNU_FUNC_PONG: stats.add(Stats::NUMPONGOUT); break;
			case GNU_FUNC_QUERY: stats.add(Stats::NUMQUERYOUT); break;
			case GNU_FUNC_HIT: stats.add(Stats::NUMHITOUT); break;
			case GNU_FUNC_PUSH: stats.add(Stats::NUMPUSHOUT); break;
			default: stats.add(Stats::NUMOTHEROUT); break;
		}


		write(p.id.id,16);
		writeChar(p.func);	// ping func
		writeChar(p.ttl);	// ttl
		writeChar(p.hops);	// hops
		writeLong(p.len);	// len

		if (p.len)
			write(p.data,p.len);

		stats.add(Stats::PACKETDATAOUT,23+p.len);

		lock.off();
	}catch(StreamException &e)
	{		
		lock.off();
		throw e;
	}
}
// ---------------------------
bool GnuStream::readPacket(GnuPacket &p)
{
	try 
	{
		lock.on();
		packetsIn++;
		stats.add(Stats::NUMPACKETSIN);

		read(p.id.id,16);
		p.func = readChar();
		p.ttl = readChar();
		p.hops = readChar();
		p.len = readLong();


		if ((p.hops >= 1) && (p.hops <= 10))
			stats.add((Stats::STAT)((int)Stats::NUMHOPS1+p.hops-1));

		stats.add(Stats::PACKETDATAIN,23+p.len);

		switch(p.func)
		{
			case GNU_FUNC_PING: stats.add(Stats::NUMPINGIN); break;
			case GNU_FUNC_PONG: stats.add(Stats::NUMPONGIN); break;
			case GNU_FUNC_QUERY: stats.add(Stats::NUMQUERYIN); break;
			case GNU_FUNC_HIT: stats.add(Stats::NUMHITIN); break;
			case GNU_FUNC_PUSH: stats.add(Stats::NUMPUSHIN); break;
			default: stats.add(Stats::NUMOTHERIN); break;
		}


		if (p.len)
		{
			if (p.len > GnuPacket::MAX_DATA)
			{
				while (p.len--)
					readChar();
				lock.off();
				return false;
			}
			read(p.data,p.len);
		}

		lock.off();
		return true;
	}catch(StreamException &e)
	{		
		lock.off();
		throw e;
	}
}

// ---------------------------
GnuStream::R_TYPE GnuStream::processPacket(GnuPacket &in, Servent *serv, GnuID &routeID)
{

	R_TYPE ret = R_DISCARD;

	MemoryStream data(in.data,in.len);

	Host remoteHost = serv->getHost();

	in.ttl--;
	in.hops++;

	routeID = in.id;



	switch(in.func)
	{
		case GNU_FUNC_PING: // ping
			{
				LOG_NETWORK("ping: from %d.%d.%d.%d : %02x%02x%02x%02x",
							remoteHost.ip>>24&0xff,remoteHost.ip>>16&0xff,remoteHost.ip>>8&0xff,remoteHost.ip&0xff,
							in.id.id[0],in.id.id[1],in.id.id[2],in.id.id[3]
					);
				Host sh = servMgr->serverHost;
				if (sh.isValid())
				{
					if ((servMgr->getFirewall() != ServMgr::FW_ON) && (!servMgr->pubInFull()))
					{
						GnuPacket pong;
						pong.initPong(sh,true,in);
						if (serv->outputPacket(pong,true))
							LOG_NETWORK("pong out");
					}
					ret = R_BROADCAST;			
				}
			}
			break;
		case GNU_FUNC_PONG: // pong
			{
				{
					int ip,port,cnt,total;
					port = data.readShort();
					ip = data.readLong();
					cnt = data.readLong();
					total = data.readLong();

					ip = SWAP4(ip);


					Host h;
					h.ip = ip;
					h.port = port;

					char sIP[64],rIP[64];
					h.toStr(sIP);
					remoteHost.toStr(rIP);
					
					LOG_NETWORK("pong: %s via %s : %02x%02x%02x%02x",sIP,ip,rIP,in.id.id[0],in.id.id[1],in.id.id[2],in.id.id[3]);


					ret = R_DISCARD;

					if (h.isValid())
					{

						#if 0
						// accept if this pong is a reply from one of our own pings, otherwise route back
						if (servMgr->isReplyID(in.id))
						{
							servMgr->addHost(h,ServHost::T_SERVENT,sys->getTime());
							ret = R_ACCEPTED;
						}else
							ret = R_ROUTE;
						#endif
					}

				}
			}
			break;
		case GNU_FUNC_QUERY: // query
			ret = R_BROADCAST;

			{
				Host sh = servMgr->serverHost;
				if (!sh.isValid())
					sh.ip = 127<<24|1;

				char words[256];
				short spd = data.readShort();
				data.readString(words,sizeof(words));
				words[sizeof(words)-1] = 0;

				MemoryStream xm(&data.buf[data.pos],data.len-data.pos);
				xm.buf[xm.len] = 0;

				Channel *hits[16];
				int numHits=0;

				if (strncmp(xm.buf,"<?xml",5)==0)
				{
					XML xml;
					xml.read(xm);
					XML::Node *cn = xml.findNode("channel");
					if (cn)
					{
						ChanInfo info;
						info.init(cn);
						info.status = ChanInfo::S_PLAY;
						numHits = chanMgr->findChannels(info,hits,16);
					}
					LOG_NETWORK("query XML: %s : found %d",xm.buf,numHits);
				}else{
					ChanInfo info;
					info.name.set(words);
					info.genre.set(words);
					info.id.fromStr(words);
					info.status = ChanInfo::S_PLAY;
					numHits = chanMgr->findChannels(info,hits,16);
					LOG_NETWORK("query STR: %s : found %d",words,numHits);
				}

			

				for(int i=0; i<numHits; i++)
				{
					bool push = (servMgr->getFirewall()!=ServMgr::FW_OFF);
					bool busy = (servMgr->pubInFull() && servMgr->outFull()) || servMgr->relaysFull();
					bool stable = servMgr->totalStreams>0;
					bool tracker = 	hits[i]->isBroadcasting();

					GnuPacket hit;
					if (hit.initHit(sh,hits[i],&in,push,busy,stable,tracker,in.hops))
						serv->outputPacket(hit,true);
				}
			}
			break;
		case GNU_FUNC_PUSH:	// push
			{

				GnuID pid;
				data.read(pid.id,16);
				
				//LOG("push serv= %02x%02x%02x%02x",servMgr->id[0],servMgr->id[1],servMgr->id[2],servMgr->id[3]);
				//LOG("pack = %02x%02x%02x%02x",id[0],id[1],id[2],id[3]);


				int index = data.readLong();
				int ip = data.readLong();
				int port = data.readShort();

			
				ip = SWAP4(ip);

				Host h(ip,port);
				char hostName[64];
				h.toStr(hostName);

#if 0
				if (servMgr->isReplyID(pid))
				{
#if 0
					Channel *c = chanMgr->findChannelByIndex(index);

					if (!c)
					{
						LOG_NETWORK("push 0x%x to %s: Not found",index,hostName);
					}else
					{
						if (!c->isFull() && !servMgr->streamFull())
						{
							LOG_NETWORK("push: 0x%x to %s: OK",index,hostName);

							Servent *s = servMgr->allocServent();
							if (s)
								s->initGIV(h,c->info.id);
						}else
							LOG_NETWORK("push: 0x%x to %s: FULL",index,hostName);
					}
#endif
					ret = R_ACCEPTED;
				}else{
					LOG_NETWORK("push: 0x%x to %s: ROUTE",index,hostName);
					routeID = pid;
					ret = R_ROUTE;
				}
#endif		
			}
			break;
		case GNU_FUNC_HIT: // hit
			{
				ret = R_DISCARD;

				ChanHit hit;
				if (readHit(data,hit,in.hops,in.id))
				{

					char flstr[64];
					flstr[0]=0;
					if (hit.firewalled) strcat(flstr,"Push,");
					if (hit.tracker) strcat(flstr,"Tracker,");
					
#if 0
					if ((spd == 0) && (!isBroadcastHit))
					{
						if (servMgr->isReplyID(queryID))
						{
							ret = R_ACCEPTED;
							LOG_NETWORK("self-hit: %s 0x%02x %s %d chan",hostName,f2,flstr,num);
						}else
						{
							routeID = queryID;
							ret = R_ROUTE;
							LOG_NETWORK("route-hit: %s 0x%02x %s %d chan",hostName,f2,flstr,num);
						}
					}else
					{
						ret = R_BROADCAST;
						LOG_NETWORK("broadcast-hit: %s 0x%02x %s %d chan",hostName,f2,flstr,num);
					}
#else
					ret = R_BROADCAST;
					LOG_NETWORK("broadcast-hit: %s",flstr);
#endif
				}
			}
			break;
		default:
			LOG_NETWORK("packet: %d",in.func);
			break;
	}

	
	if ((in.ttl > 10) || (in.hops > 10) || (in.ttl==0))
		if ((ret == R_BROADCAST) || (ret == R_ROUTE))
			ret = R_DEAD;

	return ret;
}

// ---------------------------
bool GnuStream::readHit(Stream &data, ChanHit &ch,int hops,GnuID &id)
{
	int i;
	int num = data.readChar();	// hits
	int port = data.readShort();		// port
	int ip = data.readLong();		// ip
	ip = SWAP4(ip);
	int spd = data.readLong();		// speed/broadcast

	Host h(ip,port);
	char hostName[64];

	h.IPtoStr(hostName);

	bool dataValid=true;

	ChanHit *hits[100];
	int numHits=0;

	for(i=0; i<num; i++)
	{
		int index,bitrate,listeners;


		index = data.readLong();		// index
		bitrate = data.readShort();		// bitrate
		listeners = data.readShort();	// listeners

		// read name .. not used.
		char fname[256];
		data.readString(fname,sizeof(fname));
		fname[sizeof(fname)-1] = 0;

		ch.init();
		ch.firewalled = false;		// default to NO as we dont get the info until the next section.
		ch.host = h;
		ch.numListeners = listeners;
		ch.numHops = hops;
		ch.rhost[0] = ch.host;

		ChanInfo info;


		{
			char xmlData[4000];
			int xlen = data.readString(xmlData,sizeof(xmlData));

			if ((strncmp(xmlData,"<?xml",5)==0) && (xlen < GnuPacket::MAX_DATA))
			{
				//LOG_NETWORK("Hit XML: %s",xmlData);

				MemoryStream xm(xmlData,xlen);
				XML xml;
				xml.read(xm);
				XML::Node *n = xml.findNode("channel");
				if (n)
				{
					info.init(n);
					char idStr[64];
					info.id.toStr(idStr);
					LOG_NETWORK("got hit %s %s",idStr,info.name.cstr());

					ch.upTime = n->findAttrInt("uptime");

				}else
					LOG_NETWORK("Missing Channel node");
			}else
			{
				LOG_NETWORK("Missing XML data");
				//LOG_NETWORK("%s",xmlData);
				dataValid = false;
			}
		}

		if (info.id.isSet())
		{
			if (!chanMgr->findHitList(info))
				chanMgr->addHitList(info);

			ch.recv = true;
			ch.chanID = info.id;
			ChanHit *chp = chanMgr->addHit(ch);		

			if ((chp) && (numHits<100))
				hits[numHits++] = chp;
		}

	}


	int vendor = data.readLong();	// vendor ID

	int pubLen = data.readChar();	// public sec length - should be 2

	int f1 = data.readChar() & 0xff; // flags 1
	int f2 = data.readChar() & 0xff; // flags 2

	pubLen -= 2;
	while (pubLen-->0)
		data.readChar();


	char agentStr[16];
	agentStr[0]=0;
	int maxPreviewTime=0;

	// read private sector with peercast servant specific info
	int privLen = data.readChar();	

	if (privLen)
	{
		char privData[256];
		data.read(privData,privLen);
		if (strncmp(privData,"<?xml",5)==0)
		{
			MemoryStream xm(privData,privLen);
			XML xml;
			xml.read(xm);
			XML::Node *sn = xml.findNode("servent");
			if (sn)
			{
				char *ag = sn->findAttr("agent");
				if (ag)
				{
					strncpy(agentStr,ag,16);
					agentStr[15]=0;
				}
				maxPreviewTime = sn->findAttrInt("preview");
			}

		}
	}


	// not used anymore
	GnuID queryID;
	data.read(queryID.id,16);

	bool isBroadcastHit=false;
	if (f1 & 32)
		isBroadcastHit = (f2 & 32)!=0;

	for(i=0; i<numHits; i++)
	{
		if (f1 & 1)
			hits[i]->firewalled = (f2 & 1)!=0;

		if (f1 & 64)
			hits[i]->tracker = (f2 & 64)!=0;

	}

	return dataValid;
}

