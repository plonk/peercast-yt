// ------------------------------------------------
// File : mms.cpp
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

#include "channel.h"
#include "mms.h"
#include "asf.h"

// ------------------------------------------
ASFInfo parseASFHeader(Stream &in);


// ------------------------------------------
void MMSStream::readEnd(Stream &,Channel *)
{
}

// ------------------------------------------
void MMSStream::readHeader(Stream &,Channel *)
{
}
// ------------------------------------------
int MMSStream::readPacket(Stream &in,Channel *ch)
{
	{
		ASFChunk chunk;

		chunk.read(in);

		switch (chunk.type)
		{
			case 0x4824:		// asf header
			{
				MemoryStream mem(ch->headPack.data,sizeof(ch->headPack.data));

				chunk.write(mem);




				MemoryStream asfm(chunk.data,chunk.dataLen);
				ASFObject asfHead;
				asfHead.readHead(asfm);

				ASFInfo asf = parseASFHeader(asfm);
				LOG_DEBUG("ASF Info: pnum=%d, psize=%d, br=%d",asf.numPackets,asf.packetSize,asf.bitrate);
				for(int i=0; i<ASFInfo::MAX_STREAMS; i++)
				{
					ASFStream *s = &asf.streams[i];
					if (s->id)
						LOG_DEBUG("ASF Stream %d : %s, br=%d",s->id,s->getTypeName(),s->bitrate);
				}

				ch->info.bitrate = asf.bitrate/1000;

				ch->headPack.type = ChanPacket::T_HEAD;
				ch->headPack.len = mem.pos;
				ch->headPack.pos = ch->streamPos;
				ch->newPacket(ch->headPack);

				ch->streamPos += ch->headPack.len;

				break;
			}
			case 0x4424:		// asf data
			{

				ChanPacket pack;

				MemoryStream mem(pack.data,sizeof(pack.data));

				chunk.write(mem);

				pack.type = ChanPacket::T_DATA;
				pack.len = mem.pos;
				pack.pos = ch->streamPos;

				ch->newPacket(pack);
				ch->streamPos += pack.len;

				break;
			}
			default:
				throw StreamException("Unknown ASF chunk");

		}

	}
	return 0;
}


// -----------------------------------
ASFInfo parseASFHeader(Stream &in)
{
	ASFInfo asf;

	try
	{
		int numHeaders = in.readLong();

		in.readChar();
		in.readChar();

		LOG_CHANNEL("ASF Headers: %d",numHeaders);
		for(int i=0; i<numHeaders; i++)
		{

			ASFObject obj;

			unsigned int l = obj.readHead(in);
			obj.readData(in,l);


			MemoryStream data(obj.data,obj.lenLo);


			switch (obj.type)
			{
				case ASFObject::T_FILE_PROP:
				{
					data.skip(32);

					unsigned int dpLo = data.readLong();
					unsigned int dpHi = data.readLong();

					data.skip(24);

					data.readLong();
					//data.writeLong(1);	// flags = broadcast, not seekable

					int min = data.readLong();
					int max = data.readLong();
					int br = data.readLong();

					if (min != max)
						throw StreamException("ASF packetsizes (min/max) must match");

					asf.packetSize = max;
					asf.bitrate = br;
					asf.numPackets = dpLo;
					break;
				}
				case ASFObject::T_STREAM_BITRATE:
				{
					int cnt = data.readShort();
					for(int i=0; i<cnt; i++)
					{
						unsigned int id = data.readShort();
						int bitrate = data.readLong();
						if (id < ASFInfo::MAX_STREAMS)
							asf.streams[id].bitrate = bitrate;
					}
					break;
				}
				case ASFObject::T_STREAM_PROP:
				{
					ASFStream s;
					s.read(data);
					asf.streams[s.id].id = s.id;
					asf.streams[s.id].type = s.type;
					break;
				}
			}

		}
	}catch(StreamException &e)
	{
		LOG_ERROR("ASF: %s",e.msg);
	}

	return asf;
}


