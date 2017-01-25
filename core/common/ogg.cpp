// ------------------------------------------------
// File : mp3.cpp
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
#include "ogg.h"


static int test=0;
// ------------------------------------------
void OGGStream::readHeader(Stream &,Channel *)
{
	test = 0;
}

// ------------------------------------------
void OGGStream::readEnd(Stream &,Channel *)
{
}

// ------------------------------------------
int OGGStream::readPacket(Stream &in,Channel *ch)
{
	OggPage ogg;
	ChanPacket pack;

	ogg.read(in);

	if (ogg.isBOS())
	{
		if (!vorbis.needHeader() && !theora.needHeader())
		{
			ch->headPack.len = 0;
		}

		if (ogg.detectVorbis())
			vorbis.bos(ogg.getSerialNo());
		if (ogg.detectTheora())
			theora.bos(ogg.getSerialNo());
	}

	if (ogg.isEOS())
	{

		if (ogg.getSerialNo() == vorbis.serialNo)
		{
			LOG_CHANNEL("Vorbis stream: EOS");
			vorbis.eos();
		}
		if (ogg.getSerialNo() == theora.serialNo)
		{
			LOG_CHANNEL("Theora stream: EOS");
			theora.eos();
		}
	}

	if (vorbis.needHeader() || theora.needHeader())
	{

		if (ogg.getSerialNo() == vorbis.serialNo)
			vorbis.readHeader(ch,ogg);
		else if (ogg.getSerialNo() == theora.serialNo)
			theora.readHeader(ch,ogg);
		else
			throw StreamException("Bad OGG serial no.");


		if (!vorbis.needHeader() && !theora.needHeader())
		{

			ch->info.bitrate = 0;

			if (vorbis.isActive())
				ch->info.bitrate += vorbis.bitrate;

			if (theora.isActive())
			{
				ch->info.bitrate += theora.bitrate;
				ch->info.contentType = ChanInfo::T_OGM;
			}

			
			ch->headPack.type = ChanPacket::T_HEAD;
			ch->headPack.pos = ch->streamPos;

			ch->startTime = sys->getDTime();		

			ch->streamPos += ch->headPack.len;

			ch->newPacket(ch->headPack);
			LOG_CHANNEL("Got %d bytes of headers",ch->headPack.len);
		}

	}else
	{


		pack.init(ChanPacket::T_DATA,ogg.data,ogg.headLen+ogg.bodyLen,ch->streamPos);
		ch->newPacket(pack);

		ch->streamPos+=pack.len;

		if (theora.isActive())
		{
			if (ogg.getSerialNo() == theora.serialNo)
			{
				ch->sleepUntil(theora.getTime(ogg));
			}
		}else if (vorbis.isActive())
		{
			if (ogg.getSerialNo() == vorbis.serialNo)
			{
				ch->sleepUntil(vorbis.getTime(ogg));
			}
		}

	}
	return 0;
}

// -----------------------------------
void OggSubStream::readHeader(Channel *ch,OggPage &ogg)
{
	if ((pack.bodyLen + ogg.bodyLen) >= OggPacket::MAX_BODYLEN)
		throw StreamException("OGG packet too big");

	if (ch->headPack.len+(ogg.bodyLen+ogg.headLen) >= ChanMeta::MAX_DATALEN)
		throw StreamException("OGG packet too big for headMeta");

// copy complete packet into head packet
	memcpy(&ch->headPack.data[ch->headPack.len],ogg.data,ogg.headLen+ogg.bodyLen);
	ch->headPack.len += ogg.headLen+ogg.bodyLen;

	// add body to packet
	memcpy(&pack.body[pack.bodyLen],&ogg.data[ogg.headLen],ogg.bodyLen);
	pack.bodyLen += ogg.bodyLen;

	pack.addLacing(ogg);

	if (pack.numPackets >= maxHeaders)
		procHeaders(ch);

}
// -----------------------------------
void OggVorbisSubStream::procHeaders(Channel *ch)
{
	unsigned int packPtr=0;

	for(int i=0; i<pack.numPackets; i++)
	{
		MemoryStream vin(&pack.body[packPtr],pack.packetSizes[i]);

		packPtr += pack.packetSizes[i];

		char id[8];

		vin.read(id,7);
		id[7]=0;

		switch (id[0])
		{
			case 1:	// ident
				LOG_CHANNEL("OGG Vorbis Header: Ident (%d bytes)",vin.len);
				readIdent(vin,ch->info);
				break;
			case 3: // comment
				{
					LOG_CHANNEL("OGG Vorbis Header: Comment (%d bytes)",vin.len);
					ChanInfo newInfo = ch->info;
					readComment(vin,newInfo);
					ch->updateInfo(newInfo);
				}
				break;
			case 5: // setup
				LOG_CHANNEL("OGG Vorbis Header: Setup (%d bytes)",vin.len);
				//readSetup(vin);
				break;
			default:
				throw StreamException("Unknown Vorbis packet header type");
				break;
		}
	}

}
// -----------------------------------
double OggTheoraSubStream::getTime(OggPage &ogg)
{
    int64_t iframe=ogg.granPos>>granposShift;
    int64_t pframe=ogg.granPos-(iframe<<granposShift);

    return (iframe+pframe)*frameTime;
}
// -----------------------------------
void OggTheoraSubStream::readInfo(Stream &in, ChanInfo &info)
{
	int verMaj = in.readBits(8);
	int verMin = in.readBits(8);
	int verSub = in.readBits(8);

	int encWidth = in.readBits(16) << 4;
	int encHeight = in.readBits(16) << 4;

	in.readBits(24+24+8+8);

	int fpsNum = in.readBits(32);
	int fpsDen = in.readBits(32);

	float fps = (float)fpsNum/(float)fpsDen;
	frameTime = (double)fpsDen/(double)fpsNum;

	in.readBits(24+24+8);

	bitrate = in.readBits(24) / 1000;
	int quality = in.readBits(6);

	granposShift = in.readBits(5);

	LOG_CHANNEL("OGG Theora Info: %dx%dx%.1ffps %dkbps %dQ %dG",encWidth,encHeight,fps,bitrate,quality,granposShift);


}
// -----------------------------------
void OggTheoraSubStream::procHeaders(Channel *ch)
{
	unsigned int packPtr=0;

	for(int i=0; i<pack.numPackets; i++)
	{
		MemoryStream vin(&pack.body[packPtr],pack.packetSizes[i]);

		packPtr += pack.packetSizes[i];

		unsigned char id[8];

		vin.read(id,7);
		id[7]=0;

		switch (id[0] & 0xff)
		{
			case 128:	// info
				LOG_CHANNEL("OGG Theora Header: Info (%d bytes)",vin.len);
				readInfo(vin,ch->info);
				break;
			default:
				LOG_CHANNEL("OGG Theora Header: Unknown %d (%d bytes)",id[0] & 0xff,vin.len);
				break;
		}



	}

}

// -----------------------------------
double OggVorbisSubStream::getTime(OggPage &ogg)
{
	return (double)ogg.granPos / (double)samplerate;

}

// -----------------------------------
void OggVorbisSubStream::readIdent(Stream &in, ChanInfo &info)
{
	int ver = in.readLong();
	int chans = in.readChar();
	samplerate = in.readLong();
	int brMax = in.readLong();
	int brNom = in.readLong();
	int brLow = in.readLong();
	

	in.readChar();	// skip blocksize 0+1

	LOG_CHANNEL("OGG Vorbis Ident: ver=%d, chans=%d, rate=%d, brMax=%d, brNom=%d, brLow=%d",
		ver,chans,samplerate,brMax,brNom,brLow);


	bitrate = brNom/1000;

	char frame = in.readChar();		// framing bit
	if (!frame)
		throw StreamException("Bad Indent frame");

}

 
// -----------------------------------
void OggVorbisSubStream::readSetup(Stream &in)
{
	// skip everything in packet
	int cnt=0;
	while (!in.eof())
	{
		cnt++;
		in.readChar();
	}

	LOG_CHANNEL("Read %d bytes of Vorbis Setup",cnt);
}
// -----------------------------------
void OggVorbisSubStream::readComment(Stream &in, ChanInfo &info)
{
	int vLen = in.readLong();	// vendor len

	in.skip(vLen);

	char argBuf[8192];

	info.track.clear();

	int cLen = in.readLong();	// comment len
	for(int i=0; i<cLen; i++)
	{
		int l = in.readLong();
		if (l > sizeof(argBuf))
			throw StreamException("Comment string too long");
		in.read(argBuf,l);
		argBuf[l] = 0;
		LOG_CHANNEL("OGG Comment: %s",argBuf);

		char *arg;
		if ((arg=stristr(argBuf,"ARTIST=")))
		{
			info.track.artist.set(arg+7,String::T_ASCII);
			info.track.artist.convertTo(String::T_UNICODE);

		}else if ((arg=stristr(argBuf,"TITLE=")))
		{
			info.track.title.set(arg+6,String::T_ASCII);
			info.track.title.convertTo(String::T_UNICODE);

		}else if ((arg=stristr(argBuf,"GENRE=")))
		{
			info.track.genre.set(arg+6,String::T_ASCII);
			info.track.genre.convertTo(String::T_UNICODE);

		}else if ((arg=stristr(argBuf,"CONTACT=")))
		{
			info.track.contact.set(arg+8,String::T_ASCII);
			info.track.contact.convertTo(String::T_UNICODE);

		}else if ((arg=stristr(argBuf,"ALBUM=")))
		{
			info.track.album.set(arg+6,String::T_ASCII);
			info.track.album.convertTo(String::T_UNICODE);
		}
	}

	char frame = in.readChar();		// framing bit
	if (!frame)
		throw StreamException("Bad Comment frame");

//	updateMeta();

}

// -----------------------------------
bool OggPage::isBOS()
{
	return (data[5] & 0x02) != 0;
}
// -----------------------------------
bool OggPage::isEOS()
{
	return (data[5] & 0x04) != 0;
}
// -----------------------------------
bool OggPage::isNewPacket()
{
	return (data[5] & 0x01) == 0;
}
// -----------------------------------
bool OggPage::isHeader()
{
	return ((*(unsigned int *)&data[6]) || (*(unsigned int *)&data[10])) == 0;
}
// -----------------------------------
unsigned int OggPage::getSerialNo()
{
	return *(unsigned int *)&data[14];
}

// -----------------------------------
void OggPage::read(Stream &in)
{
	// skip until we get OGG capture pattern
	bool gotOgg=false;
	while (!gotOgg)
	{
		if (in.readChar() == 'O')
			if (in.readChar() == 'g')
				if (in.readChar() == 'g')
					if (in.readChar() == 'S')
						gotOgg = true;
		if (!gotOgg)
			LOG_CHANNEL("Skipping OGG packet");
	}



	memcpy(&data[0],"OggS",4);

	in.read(&data[4],27-4);

	int numSegs = data[26];
	bodyLen = 0;

	// read segment table
	in.read(&data[27],numSegs);
	for(int i=0; i<numSegs; i++)
		bodyLen += data[27+i];

	if (bodyLen >= MAX_BODYLEN)		
		throw StreamException("OGG body too big");

	headLen = 27+numSegs;

	if (headLen > MAX_HEADERLEN)
		throw StreamException("OGG header too big");

	in.read(&data[headLen],bodyLen);

	granPos = *(unsigned int *)&data[10];
	granPos <<= 32;
	granPos |= *(unsigned int *)&data[6];


	#if 0
		LOG_DEBUG("OGG Packet - page %d, id = %x - %s %s %s - %d:%d - %d segs, %d bytes",
			*(unsigned int *)&data[18],
			*(unsigned int *)&data[14],
			data[5]&0x1?"cont":"new",
			data[5]&0x2?"bos":"",
			data[5]&0x4?"eos":"",
			(unsigned int)(granPos>>32),
			(unsigned int)(granPos&0xffffffff),
			numSegs,
			headLen+bodyLen);

	#endif

}
// -----------------------------------
bool OggPage::detectVorbis()
{
	return memcmp(&data[headLen+1],"vorbis",6) == 0;
}
// -----------------------------------
bool OggPage::detectTheora()
{
	return memcmp(&data[headLen+1],"theora",6) == 0;
}

// -----------------------------------
void	OggPacket::addLacing(OggPage &ogg)
{
	
	int numSegs = ogg.data[26];
	for(int i=0; i<numSegs; i++)
	{
		int seg = ogg.data[27+i];

		packetSizes[numPackets]+=seg;

		if (seg < 255)
		{
			numPackets++;
			if (numPackets >= MAX_PACKETS)
				throw StreamException("Too many OGG packets");
			packetSizes[numPackets]=0;
		}
	}

}
