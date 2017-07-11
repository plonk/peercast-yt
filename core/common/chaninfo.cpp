// ------------------------------------------------
// File : chaninfo.cpp
// Date: 4-apr-2002
// Author: giles
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

#include "chaninfo.h"
#include "pcp.h"
#include "chanmgr.h"
#include "playlist.h"

// -----------------------------------
static void readXMLString(String &str, XML::Node *n, const char *arg)
{
    char *p;
    p = n->findAttr(arg);
    if (p)
    {
        str.set(p, String::T_HTML);
        str.convertTo(String::T_ASCII);
    }
}

// -----------------------------------
const char *ChanInfo::getTypeStr()
{
    if (contentTypeStr.isEmpty()) {
        return getTypeStr(contentType);
    }
    else {
        return contentTypeStr.cstr();
    }
}

// -----------------------------------
std::string ChanInfo::getTypeStringLong()
{
    std::string buf = std::string(getTypeStr()) +
        " (" + getMIMEType() + "; " + getTypeExt() + ")";

    if (contentTypeStr == "")
        buf += " [contentTypeStr empty]"; // これが起こるのは何かがおかしい
    if (MIMEType == "")
        buf += " [no styp]";
    if (streamExt == "")
        buf += " [no sext]";

    return buf;
}

// -----------------------------------
const char *ChanInfo::getTypeExt()
{
    if (streamExt.isEmpty()) {
        return getTypeExt(contentType);
    }
    else {
        return streamExt.cstr();
    }
}

// -----------------------------------
const char *ChanInfo::getMIMEType()
{
    if (MIMEType.isEmpty()) {
        return getMIMEType(contentType);
    }
    else {
        return MIMEType.cstr();
    }
}

// -----------------------------------
const char *ChanInfo::getTypeStr(TYPE t)
{
    switch (t)
    {
        case T_RAW: return "RAW";

        case T_MP3: return "MP3";
        case T_OGG: return "OGG";
        case T_OGM: return "OGM";
        case T_WMA: return "WMA";

        case T_MOV: return "MOV";
        case T_MPG: return "MPG";
        case T_NSV: return "NSV";
        case T_WMV: return "WMV";
        case T_FLV: return "FLV";
        case T_MKV: return "MKV";
        case T_WEBM: return "WEBM";

        case T_PLS: return "PLS";
        case T_ASX: return "ASX";

        default: return "UNKNOWN";
    }
}

// -----------------------------------
const char *ChanInfo::getProtocolStr(PROTOCOL t)
{
    switch (t)
    {
        case SP_PEERCAST: return "PEERCAST";
        case SP_HTTP: return "HTTP";
        case SP_FILE: return "FILE";
        case SP_MMS: return "MMS";
        case SP_PCP: return "PCP";
        case SP_WMHTTP: return "WMHTTP";
        case SP_RTMP: return "RTMP";
        default: return "UNKNOWN";
    }
}

// -----------------------------------
ChanInfo::PROTOCOL ChanInfo::getProtocolFromStr(const char *str)
{
    if (Sys::stricmp(str, "PEERCAST")==0)
        return SP_PEERCAST;
    else if (Sys::stricmp(str, "HTTP")==0)
        return SP_HTTP;
    else if (Sys::stricmp(str, "FILE")==0)
        return SP_FILE;
    else if (Sys::stricmp(str, "MMS")==0)
        return SP_MMS;
    else if (Sys::stricmp(str, "PCP")==0)
        return SP_PCP;
    else if (Sys::stricmp(str, "WMHTTP")==0)
        return SP_WMHTTP;
    else if (Sys::stricmp(str, "RTMP")==0)
        return SP_RTMP;
    else
        return SP_UNKNOWN;
}

// -----------------------------------
const char *ChanInfo::getTypeExt(TYPE t)
{
    switch(t)
    {
        case ChanInfo::T_OGM:
        case ChanInfo::T_OGG:
            return ".ogg";
        case ChanInfo::T_MP3:
            return ".mp3";
        case ChanInfo::T_MOV:
            return ".mov";
        case ChanInfo::T_NSV:
            return ".nsv";
        case ChanInfo::T_WMV:
            return ".wmv";
        case ChanInfo::T_WMA:
            return ".wma";
        case ChanInfo::T_FLV:
            return ".flv";
        case ChanInfo::T_MKV:
            return ".mkv";
        case ChanInfo::T_WEBM:
            return ".webm";
        default:
            return "";
    }
}

// -----------------------------------
const char *ChanInfo::getMIMEType(TYPE t)
{
    switch(t)
    {
        case ChanInfo::T_OGG:
            return MIME_XOGG;
        case ChanInfo::T_OGM:
            return MIME_XOGG;
        case ChanInfo::T_MP3:
            return MIME_MP3;
        case ChanInfo::T_MOV:
            return MIME_MOV;
        case ChanInfo::T_MPG:
            return MIME_MPG;
        case ChanInfo::T_NSV:
            return MIME_NSV;
        case ChanInfo::T_ASX:
            return MIME_ASX;
        case ChanInfo::T_WMA:
            return MIME_WMA;
        case ChanInfo::T_WMV:
            return MIME_WMV;
        case ChanInfo::T_FLV:
            return MIME_FLV;
        case ChanInfo::T_MKV:
            return MIME_MKV;
        case ChanInfo::T_WEBM:
            return MIME_WEBM;
        default:
            return "application/octet-stream";
    }
}

// -----------------------------------
ChanInfo::TYPE ChanInfo::getTypeFromMIME(const std::string& mediaType)
{
    if (mediaType == MIME_XOGG)
        return T_OGG;
    else if (mediaType == MIME_XOGG)
        return T_OGM;
    else if (mediaType == MIME_MP3)
        return T_MP3;
    else if (mediaType == MIME_MOV)
        return T_MOV;
    else if (mediaType == MIME_MPG)
        return T_MPG;
    else if (mediaType == MIME_NSV)
        return T_NSV;
    else if (mediaType == MIME_ASX)
        return T_ASX;
    else if (mediaType == MIME_WMA)
        return T_WMA;
    else if (mediaType == MIME_WMV)
        return T_WMV;
    else if (mediaType == MIME_FLV)
        return T_FLV;
    else if (mediaType == MIME_MKV)
        return T_MKV;
    else if (mediaType == MIME_WEBM)
        return T_WEBM;
    else
        return T_UNKNOWN;
}

// -----------------------------------
ChanInfo::TYPE ChanInfo::getTypeFromStr(const char *str)
{
    if (Sys::stricmp(str, "MP3")==0)
        return T_MP3;
    else if (Sys::stricmp(str, "OGG")==0)
        return T_OGG;
    else if (Sys::stricmp(str, "OGM")==0)
        return T_OGM;
    else if (Sys::stricmp(str, "RAW")==0)
        return T_RAW;
    else if (Sys::stricmp(str, "NSV")==0)
        return T_NSV;
    else if (Sys::stricmp(str, "WMA")==0)
        return T_WMA;
    else if (Sys::stricmp(str, "WMV")==0)
        return T_WMV;
    else if (Sys::stricmp(str, "FLV")==0)
        return T_FLV;
    else if (Sys::stricmp(str, "MKV")==0)
        return T_MKV;
    else if (Sys::stricmp(str, "WEBM")==0)
        return T_WEBM;
    else if (Sys::stricmp(str, "PLS")==0)
        return T_PLS;
    else if (Sys::stricmp(str, "M3U")==0)
        return T_PLS;
    else if (Sys::stricmp(str, "ASX")==0)
        return T_ASX;
    else
        return T_UNKNOWN;
}

// -----------------------------------
bool    ChanInfo::matchNameID(ChanInfo &inf)
{
    if (inf.id.isSet())
        if (id.isSame(inf.id))
            return true;

    if (!inf.name.isEmpty())
        if (name.contains(inf.name))
            return true;

    return false;
}

// -----------------------------------
bool    ChanInfo::match(ChanInfo &inf)
{
    bool matchAny=true;

    if (inf.status != S_UNKNOWN)
    {
        if (status != inf.status)
            return false;
    }

    if (inf.bitrate != 0)
    {
        if (bitrate == inf.bitrate)
            return true;
        matchAny = false;
    }

    if (inf.id.isSet())
    {
        if (id.isSame(inf.id))
            return true;
        matchAny = false;
    }

    if (inf.contentType != T_UNKNOWN)
    {
        if (contentType == inf.contentType)
            return true;
        matchAny = false;
    }

    if (!inf.name.isEmpty())
    {
        if (name.contains(inf.name))
            return true;
        matchAny = false;
    }

    if (!inf.genre.isEmpty())
    {
        if (genre.contains(inf.genre))
            return true;
        matchAny = false;
    }

    return matchAny;
}

// -----------------------------------
bool ChanInfo::update(const ChanInfo &info)
{
    bool changed = false;

    // check valid id
    if (!info.id.isSet())
        return false;

    // only update from chaninfo that has full name etc..
    if (info.name.isEmpty())
        return false;

    // check valid broadcaster key
    if (bcID.isSet())
    {
        if (!bcID.isSame(info.bcID))
        {
            LOG_ERROR("ChanInfo BC key not valid");
            return false;
        }
    }else
    {
        bcID = info.bcID;
    }

    if (bitrate != info.bitrate)
    {
        bitrate = info.bitrate;
        changed = true;
    }

    if (contentType != info.contentType)
    {
        contentType = info.contentType;
        changed = true;
    }

    if (!contentTypeStr.isSame(info.contentTypeStr))
    {
        contentTypeStr = info.contentTypeStr;
        changed = true;
    }

    if (!MIMEType.isSame(info.MIMEType))
    {
        MIMEType = info.MIMEType;
        changed = true;
    }

    if (!streamExt.isSame(info.streamExt))
    {
        streamExt = info.streamExt;
        changed = true;
    }

    if (!desc.isSame(info.desc))
    {
        desc = info.desc;
        changed = true;
    }

    if (!name.isSame(info.name))
    {
        name = info.name;
        changed = true;
    }

    if (!comment.isSame(info.comment))
    {
        comment = info.comment;
        changed = true;
    }

    if (!genre.isSame(info.genre))
    {
        genre = info.genre;
        changed = true;
    }

    if (!url.isSame(info.url))
    {
        url = info.url;
        changed = true;
    }

    if (track.update(info.track))
        changed = true;

    return changed;
}

// -----------------------------------
void ChanInfo::initNameID(const char *n)
{
    init();
    id.fromStr(n);
    if (!id.isSet())
        name.set(n);
}

// -----------------------------------
void ChanInfo::init()
{
    status = S_UNKNOWN;
    name.clear();
    bitrate = 0;
    contentType = T_UNKNOWN;
    contentTypeStr.clear();
    MIMEType.clear();
    streamExt.clear();
    srcProtocol = SP_UNKNOWN;
    id.clear();
    url.clear();
    genre.clear();
    comment.clear();
    track.clear();
    lastPlayStart = 0;
    lastPlayEnd = 0;
    numSkips = 0;
    bcID.clear();
    createdTime = 0;
}

// -----------------------------------
void ChanInfo::readTrackXML(XML::Node *n)
{
    track.clear();
    readXMLString(track.title, n, "title");
    readXMLString(track.contact, n, "contact");
    readXMLString(track.artist, n, "artist");
    readXMLString(track.album, n, "album");
    readXMLString(track.genre, n, "genre");
}

// -----------------------------------
unsigned int ChanInfo::getUptime()
{
    // calculate uptime and cap if requested by settings.
    unsigned int upt;
    upt = lastPlayStart?(sys->getTime()-lastPlayStart):0;
    if (chanMgr->maxUptime)
        if (upt > chanMgr->maxUptime)
            upt = chanMgr->maxUptime;
    return upt;
}

// -----------------------------------
unsigned int ChanInfo::getAge()
{
    return sys->getTime()-createdTime;
}

// ------------------------------------------
void ChanInfo::readTrackAtoms(AtomStream &atom, int numc)
{
    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);
        if (id == PCP_CHAN_TRACK_TITLE)
        {
            atom.readString(track.title.data, sizeof(track.title.data), d);
        }else if (id == PCP_CHAN_TRACK_CREATOR)
        {
            atom.readString(track.artist.data, sizeof(track.artist.data), d);
        }else if (id == PCP_CHAN_TRACK_URL)
        {
            atom.readString(track.contact.data, sizeof(track.contact.data), d);
        }else if (id == PCP_CHAN_TRACK_ALBUM)
        {
            atom.readString(track.album.data, sizeof(track.album.data), d);
        }else
            atom.skip(c, d);
    }
}

// ------------------------------------------
void ChanInfo::readInfoAtoms(AtomStream &atom, int numc)
{
    for (int i=0; i<numc; i++)
    {
        int c, d;
        ID4 id = atom.read(c, d);
        if (id == PCP_CHAN_INFO_NAME)
        {
            atom.readString(name.data, sizeof(name.data), d);
        }else if (id == PCP_CHAN_INFO_BITRATE)
        {
            bitrate = atom.readInt();
        }else if (id == PCP_CHAN_INFO_GENRE)
        {
            atom.readString(genre.data, sizeof(genre.data), d);
        }else if (id == PCP_CHAN_INFO_URL)
        {
            atom.readString(url.data, sizeof(url.data), d);
        }else if (id == PCP_CHAN_INFO_DESC)
        {
            atom.readString(desc.data, sizeof(desc.data), d);
        }else if (id == PCP_CHAN_INFO_COMMENT)
        {
            atom.readString(comment.data, sizeof(comment.data), d);
        }else if (id == PCP_CHAN_INFO_TYPE)
        {
            char type[16];
            atom.readString(type, sizeof(type), d);
            contentType = ChanInfo::getTypeFromStr(type);
            contentTypeStr = type;
        }else if (id == PCP_CHAN_INFO_STREAMTYPE)
        {
            atom.readString(MIMEType.data, sizeof(MIMEType.data), d);
        }else if (id == PCP_CHAN_INFO_STREAMEXT)
        {
            atom.readString(streamExt.data, sizeof(streamExt.data), d);
        }else
            atom.skip(c, d);
    }
}

// -----------------------------------
void ChanInfo::writeInfoAtoms(AtomStream &atom)
{
    int natoms = 7;

    natoms += !MIMEType.isEmpty();
    natoms += !streamExt.isEmpty();

    atom.writeParent(PCP_CHAN_INFO, natoms);
        atom.writeString(PCP_CHAN_INFO_NAME, name.cstr());
        atom.writeInt(PCP_CHAN_INFO_BITRATE, bitrate);
        atom.writeString(PCP_CHAN_INFO_GENRE, genre.cstr());
        atom.writeString(PCP_CHAN_INFO_URL, url.cstr());
        atom.writeString(PCP_CHAN_INFO_DESC, desc.cstr());
        atom.writeString(PCP_CHAN_INFO_COMMENT, comment.cstr());
        atom.writeString(PCP_CHAN_INFO_TYPE, getTypeStr());
        if (!MIMEType.isEmpty())
            atom.writeString(PCP_CHAN_INFO_STREAMTYPE, MIMEType.cstr());
        if (!streamExt.isEmpty())
            atom.writeString(PCP_CHAN_INFO_STREAMEXT, streamExt.cstr());
}

// -----------------------------------
void ChanInfo::writeTrackAtoms(AtomStream &atom)
{
    atom.writeParent(PCP_CHAN_TRACK, 4);
        atom.writeString(PCP_CHAN_TRACK_TITLE, track.title.cstr());
        atom.writeString(PCP_CHAN_TRACK_CREATOR, track.artist.cstr());
        atom.writeString(PCP_CHAN_TRACK_URL, track.contact.cstr());
        atom.writeString(PCP_CHAN_TRACK_ALBUM, track.album.cstr());
}

// -----------------------------------
XML::Node *ChanInfo::createChannelXML()
{
    char idStr[64];

    String nameUNI = name;
    nameUNI.convertTo(String::T_UNICODESAFE);

    String urlUNI = url;
    urlUNI.convertTo(String::T_UNICODESAFE);

    String genreUNI = genre;
    genreUNI.convertTo(String::T_UNICODESAFE);

    String descUNI = desc;
    descUNI.convertTo(String::T_UNICODESAFE);

    String commentUNI;
    commentUNI = comment;
    commentUNI.convertTo(String::T_UNICODESAFE);

    id.toStr(idStr);

    return new XML::Node("channel name=\"%s\" id=\"%s\" bitrate=\"%d\" type=\"%s\" genre=\"%s\" desc=\"%s\" url=\"%s\" uptime=\"%d\" comment=\"%s\" skips=\"%d\" age=\"%d\" bcflags=\"%d\"",
        nameUNI.cstr(),
        idStr,
        bitrate,
        getTypeStr(),
        genreUNI.cstr(),
        descUNI.cstr(),
        urlUNI.cstr(),
        getUptime(),
        commentUNI.cstr(),
        numSkips,
        getAge(),
        bcID.getFlags()
        );
}

// -----------------------------------
XML::Node *ChanInfo::createQueryXML()
{
    char buf[512];
    char idStr[64];

    String nameHTML = name;
    nameHTML.convertTo(String::T_HTML);
    String genreHTML = genre;
    genreHTML.convertTo(String::T_HTML);

    buf[0]=0;
    if (!nameHTML.isEmpty())
    {
        strcat(buf, " name=\"");
        strcat(buf, nameHTML.cstr());
        strcat(buf, "\"");
    }

    if (!genreHTML.isEmpty())
    {
        strcat(buf, " genre=\"");
        strcat(buf, genreHTML.cstr());
        strcat(buf, "\"");
    }

    if (id.isSet())
    {
        id.toStr(idStr);
        strcat(buf, " id=\"");
        strcat(buf, idStr);
        strcat(buf, "\"");
    }

    return new XML::Node("channel %s", buf);
}

// -----------------------------------
XML::Node *ChanInfo::createRelayChannelXML()
{
    char idStr[64];

    id.toStr(idStr);

    return new XML::Node("channel id=\"%s\" uptime=\"%d\" skips=\"%d\" age=\"%d\"",
        idStr,
        getUptime(),
        numSkips,
        getAge()
        );
}

// -----------------------------------
XML::Node *ChanInfo::createTrackXML()
{
    String titleUNI = track.title;
    titleUNI.convertTo(String::T_UNICODESAFE);

    String artistUNI = track.artist;
    artistUNI.convertTo(String::T_UNICODESAFE);

    String albumUNI = track.album;
    albumUNI.convertTo(String::T_UNICODESAFE);

    String genreUNI = track.genre;
    genreUNI.convertTo(String::T_UNICODESAFE);

    String contactUNI = track.contact;
    contactUNI.convertTo(String::T_UNICODESAFE);

    return new XML::Node("track title=\"%s\" artist=\"%s\" album=\"%s\" genre=\"%s\" contact=\"%s\"",
        titleUNI.cstr(),
        artistUNI.cstr(),
        albumUNI.cstr(),
        genreUNI.cstr(),
        contactUNI.cstr()
        );
}

// -----------------------------------
void ChanInfo::init(XML::Node *n)
{
    init();

    updateFromXML(n);
}

// -----------------------------------
void ChanInfo::updateFromXML(XML::Node *n)
{
    String typeStr, idStr;

    readXMLString(name, n, "name");
    readXMLString(genre, n, "genre");
    readXMLString(url, n, "url");
    readXMLString(desc, n, "desc");

    int br = n->findAttrInt("bitrate");
    if (br)
        bitrate = br;

    readXMLString(typeStr, n, "type");
    if (!typeStr.isEmpty()) {
        contentType = getTypeFromStr(typeStr.cstr());
        contentTypeStr = typeStr;
    }

    readXMLString(idStr, n, "id");
    if (!idStr.isEmpty())
        id.fromStr(idStr.cstr());

    readXMLString(comment, n, "comment");

    XML::Node *tn = n->findNode("track");
    if (tn)
        readTrackXML(tn);
}

// -----------------------------------
void ChanInfo::init(const char *n, GnuID &cid, TYPE tp, int br)
{
    init();

    name.set(n);
    bitrate = br;
    contentType = tp;
    id = cid;
}

// -----------------------------------
void ChanInfo::init(const char *fn)
{
    init();

    if (fn)
        name.set(fn);
}

// -----------------------------------
void ChanInfo::setContentType(TYPE type)
{
    this->contentType    = type;
    this->contentTypeStr = getTypeStr(type);
    this->MIMEType       = getMIMEType(type);
    this->streamExt      = getTypeExt(type);
}

// -----------------------------------
bool TrackInfo::update(const TrackInfo &inf)
{
    bool changed = false;

    if (!contact.isSame(inf.contact))
    {
        contact = inf.contact;
        changed = true;
    }

    if (!title.isSame(inf.title))
    {
        title = inf.title;
        changed = true;
    }

    if (!artist.isSame(inf.artist))
    {
        artist = inf.artist;
        changed = true;
    }

    if (!album.isSame(inf.album))
    {
        album = inf.album;
        changed = true;
    }

    if (!genre.isSame(inf.genre))
    {
        genre = inf.genre;
        changed = true;
    }

    return changed;
}

// -----------------------------------
const char* ChanInfo::getPlayListExt()
{
    switch (PlayList::getPlayListType(contentType))
    {
    case PlayList::T_ASX:
        return ".asx";
    case PlayList::T_RAM:
        return ".ram";
    case PlayList::T_PLS:
        return ".m3u"; // or could be .pls ...
    case PlayList::T_SCPLS:
        return ".pls";
    case PlayList::T_NONE:
    default:
        return "";
    }
}
