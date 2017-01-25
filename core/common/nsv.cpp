// ------------------------------------------------
// File : nsv.cpp
// Date: 28-may-2003
// Author: giles
//
// (c) 2002-3 peercast.org
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

#include "nsv.h"

// ------------------------------------------
void NSVStream::readEnd(Stream &,Channel *)
{
}

// ------------------------------------------
void NSVStream::readHeader(Stream &,Channel *)
{
}
// ------------------------------------------
int NSVStream::readPacket(Stream &in,Channel *ch)
{
	ChanPacket pack;

	if (ch->icyMetaInterval)
	{

		int rlen = ch->icyMetaInterval;

		while (rlen)
		{
			int rl = rlen;
			if (rl > ChanMgr::MAX_METAINT)
				rl = ChanMgr::MAX_METAINT;

			pack.init(ChanPacket::T_DATA,pack.data,rl,ch->streamPos);
			in.read(pack.data,pack.len);
			ch->newPacket(pack);
			ch->checkReadDelay(pack.len);
			ch->streamPos+=pack.len;

			rlen-=rl;
		}

		unsigned char len;
		in.read(&len,1);
		if (len)
		{
			if (len*16 > 1024) len = 1024/16;
			char buf[1024];
			in.read(buf,len*16);
			ch->processMp3Metadata(buf);
		}

	}else{

		pack.init(ChanPacket::T_DATA,pack.data,ChanMgr::MAX_METAINT,ch->streamPos);
		in.read(pack.data,pack.len);
		ch->newPacket(pack);
		ch->checkReadDelay(pack.len);

		ch->streamPos += pack.len;
	}
	return 0;
}
