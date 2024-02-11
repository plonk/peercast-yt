#ifndef _CHANINFO_H
#define _CHANINFO_H

// ------------------------------------------------
// File : chaninfo.h
// Date: 4-apr-2002
// Author: giles
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

#include "gnuid.h"
#include "sys.h"
#include "xml.h"
#include "atom.h"
#include "http.h"
#include "varwriter.h"

// ----------------------------------
class TrackInfo
{
public:
    void    clear()
    {
        contact.clear();
        title.clear();
        artist.clear();
        album.clear();
        genre.clear();
    }

    void    convertTo(String::TYPE t)
    {
        contact.convertTo(t);
        title.convertTo(t);
        artist.convertTo(t);
        album.convertTo(t);
        genre.convertTo(t);
    }

    bool    update(const TrackInfo &);

    ::String    contact, title, artist, album, genre;
};

// ----------------------------------
class ChanInfo : public VariableWriter
{
public:
    typedef ::String TYPE;
    static const TYPE
        T_UNKNOWN,
        T_RAW,
        T_MP3,
        T_OGG,
        T_OGM,
        T_MOV,
        T_MPG,
        T_NSV,
        T_FLV,
        T_MKV,
        T_WEBM,
        T_MP4,
        T_WMA,
        T_WMV,
        T_PLS,
        T_ASX;

    enum PROTOCOL
    {
        SP_UNKNOWN,
        SP_HTTP,
        SP_FILE,
        SP_MMS,
        SP_PCP,
        SP_WMHTTP,
        SP_RTMP,
        SP_PIPE,
    };

    enum STATUS
    {
        S_UNKNOWN,
        S_PLAY
    };

    ChanInfo() { init(); }

    void    init();
    void    init(const char *);
    void    init(const char *, GnuID &, TYPE, int);
    void    initNameID(const char *);

    bool        update(const ChanInfo &);
    XML::Node   *createChannelXML();
    XML::Node   *createRelayChannelXML();
    XML::Node   *createTrackXML();
    bool        match(XML::Node *);
    bool        match(ChanInfo &);
    bool        matchNameID(ChanInfo &);

    void    writeInfoAtoms(AtomStream &atom);
    void    writeTrackAtoms(AtomStream &atom);

    void    readInfoAtoms(AtomStream &, int);
    void    readTrackAtoms(AtomStream &, int);

    amf0::Value getState() override;

    unsigned int        getUptime();
    unsigned int        getAge();
    bool                isActive() { return id.isSet(); }
    bool                isPrivate() { return bcID.getFlags() & 1; }
    const char          *getTypeStr();
    std::string         getTypeStringLong();
    const char          *getTypeExt();
    const char          *getMIMEType() const;
    static const char   *getTypeStr(const TYPE&);
    static const char   *getProtocolStr(PROTOCOL);
    static const char   *getTypeExt(TYPE);
    static const char   *getMIMEType(TYPE);
    static TYPE         getTypeFromStr(const char *str);
    static TYPE         getTypeFromMIME(const std::string& mediaType);
    static PROTOCOL     getProtocolFromStr(const char *str);
    const char*         getPlayListExt();

    void setContentType(TYPE type);

    ::String        name;
    GnuID           id, bcID;
    int             bitrate;

    // TYPE はクローズドだから一般性がなく、プロトコル上は文字列でやり
    // とりするので、冗長な気がする。

    ::String        contentType;
    ::String        MIMEType;       // MIME タイプ
    String          streamExt;      // "." で始まる拡張子

    PROTOCOL        srcProtocol;
    unsigned int    lastPlayStart, lastPlayEnd;
    unsigned int    numSkips;
    unsigned int    createdTime;

    STATUS          status;

    TrackInfo       track;
    ::String        desc, genre, url, comment;
};

#endif
