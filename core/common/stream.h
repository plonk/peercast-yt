// ------------------------------------------------
// File : stream.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _STREAM_H
#define _STREAM_H

// -------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "sys.h"
#include "id.h"

// -------------------------------------
class Stream
{
public:
    Stream()
    : writeCRLF(true)
    , bitsBuffer(0)
    , bitsPos(0)
    {
    }

    virtual ~Stream() {}

    virtual int readUpto(void *, int) { return 0; }
    virtual int read(void *, int) = 0;
    virtual void write(const void *, int) = 0;
    virtual bool eof()
    {
        throw StreamException("Stream can`t eof");
        return false;
    }

    virtual void rewind()
    {
        throw StreamException("Stream can`t rewind");
    }

    virtual void seekTo(int)
    {
        throw StreamException("Stream can`t seek");
    }

    void writeTo(Stream &out, int len);
    virtual void skip(int i);

    virtual void close()
    {
    }

    virtual void    setReadTimeout(unsigned int )
    {
    }
    virtual void    setWriteTimeout(unsigned int )
    {
    }
    virtual void    setPollRead(bool)
    {
    }

    virtual int     getPosition() { return 0; }

    // binary
    char    readChar()
    {
        char v;
        read(&v, 1);
        return v;
    }
    short   readShort()
    {
        short v;
        read(&v, 2);
        CHECK_ENDIAN2(v);
        return v;
    }
    long    readLong()
    {
        long v = 0;
        read(&v, 4);
        CHECK_ENDIAN4(v);
        return v;
    }
    int readInt()
    {
        return readLong();
    }
    ID4 readID4()
    {
        ID4 id;
        read(id.getData(), 4);
        return id;
    }
    int readInt24()
    {
        int v=0;
        read(&v, 3);
        CHECK_ENDIAN3(v);
        return v;
    }

    long readTag()
    {
        long v = readLong();
        return SWAP4(v);
    }

    int readString(char *s, int max)
    {
        int cnt=0;
        while (max)
        {
            char c = readChar();
            *s++ = c;
            cnt++;
            max--;
            if (!c)
                break;
        }
        return cnt;
    }

    std::string readLine();

    std::string read(int remaining)
    {
        std::string res;

        uint8_t buffer[4096];

        while (remaining > 0)
        {
            int readSize = std::min(remaining, 4096);
            int r = read(buffer, readSize);
            res += std::string(buffer, buffer + r);
            remaining -= r;
        }
        return res;
    }

    virtual bool    readReady(int timeoutMilliseconds = 0) { return true; }
    virtual int numPending() { return 0; }

    void writeID4(ID4 id)
    {
        write(id.getData(), 4);
    }

    void    writeChar(char v)
    {
        write(&v, 1);
    }
    void    writeShort(short v)
    {
        CHECK_ENDIAN2(v);
        write(&v, 2);
    }
    void    writeLong(long v)
    {
        CHECK_ENDIAN4(v);
        write(&v, 4);
    }
    void writeInt(int v) { writeLong(v); }

    void    writeTag(long v)
    {
        //CHECK_ENDIAN4(v);
        writeLong(SWAP4(v));
    }

    void    writeTag(const char id[4])
    {
        write(id, 4);
    }

    int writeUTF8(unsigned int);

    // text
    int readLine(char *in, int max);

    int     readWord(char *, int);
    int     readBase64(char *, int);

    void    write(const char *, va_list);
    void    writeLine(const char *);
    void    writeLineF(const char *, ...) __attribute__ ((format (printf, 2, 3)));
    void    writeString(const char *);
    void    writeString(const std::string& s)
    {
        write(s.data(), s.size());
    }
    void    writeString(const String& s)
    {
        write(s.c_str(), s.size());
    }
    void    writeStringF(const char *, ...) __attribute__ ((format (printf, 2, 3)));

    bool    writeCRLF;

    int     readBits(int);

    void    updateTotals(unsigned int, unsigned int);

    unsigned char   bitsBuffer;
    unsigned int    bitsPos;

    unsigned int totalBytesIn();
    unsigned int totalBytesOut();
    unsigned int lastBytesIn();
    unsigned int lastBytesOut();
    unsigned int bytesInPerSec();
    unsigned int bytesOutPerSec();

    class Stat
    {
    public:
        const double Exp = 9.0/10.0;

        Stat ()
            : m_totalBytesIn(0)
            , m_totalBytesOut(0)
            , m_lastBytesIn(0)
            , m_lastBytesOut(0)
            , m_bytesInPerSec(0)
            , m_bytesOutPerSec(0)
            , m_bytesInPerSecAvg(0)
            , m_bytesOutPerSecAvg(0)
            , m_lastUpdate(0)
            , m_startTime(0)
        {}

        void update(unsigned int in, unsigned int out)
        {
            double now = sys->getDTime();

            if (m_lastUpdate == 0.0)
            {
                m_startTime = now;
                m_lastUpdate = now;
            }

            m_totalBytesIn  += in;
            m_totalBytesOut += out;

            double tdiff = now - m_lastUpdate;
            if (tdiff >= 1.0)
            {
                m_bytesInPerSec     = (m_totalBytesIn - m_lastBytesIn) / tdiff;
                m_bytesOutPerSec    = (m_totalBytesOut - m_lastBytesOut) / tdiff;

                m_bytesInPerSecAvg  = Exp * m_bytesInPerSecAvg + (1-Exp) * m_bytesInPerSec;
                m_bytesOutPerSecAvg = Exp * m_bytesOutPerSecAvg + (1-Exp) * m_bytesOutPerSec;

                m_lastBytesIn       = m_totalBytesIn;
                m_lastBytesOut      = m_totalBytesOut;

                m_lastUpdate        = now;
            }
        }

        void update() { update(0, 0); }

        unsigned int    totalBytesIn() { update(); return m_totalBytesIn; }
        unsigned int    totalBytesOut() { update(); return m_totalBytesOut; }
        unsigned int    lastBytesIn() { update(); return m_lastBytesIn; }
        unsigned int    lastBytesOut() { update(); return m_lastBytesOut; }
        unsigned int    bytesInPerSec() { update(); return m_bytesInPerSec; }
        unsigned int    bytesOutPerSec() { update(); return m_bytesOutPerSec; }

        unsigned int    bytesInPerSecAvg() { update(); return m_bytesInPerSecAvg; }
        unsigned int    bytesOutPerSecAvg() { update(); return m_bytesOutPerSecAvg; }

        unsigned int    m_totalBytesIn, m_totalBytesOut;
        unsigned int    m_lastBytesIn, m_lastBytesOut;
        unsigned int    m_bytesInPerSec, m_bytesOutPerSec;

        double          m_bytesInPerSecAvg;
        double          m_bytesOutPerSecAvg;

        double          m_lastUpdate;
        double          m_startTime;
    };

    Stat stat;
};

// -------------------------------------
class FileStream : public Stream
{
public:
    FileStream() { file=NULL; }
    ~FileStream() { close(); }

    void    openReadOnly(const char *);
    void    openReadOnly(const std::string& fn) { openReadOnly(fn.c_str()); }
    void    openReadOnly(int fd);
    void    openWriteReplace(const char *);
    void    openWriteReplace(const std::string& fn) { openWriteReplace(fn.c_str()); }
    void    openWriteAppend(const char *);
    void    openWriteAppend(const std::string& fn) { openWriteAppend(fn.c_str()); }
    bool    isOpen() { return file!=NULL; }
    int     length();
    int     pos();

    void    seekTo(int) override;
    int     getPosition() override { return pos(); }
    void    flush();
    int     read(void *, int) override;
    void    write(const void *, int) override;
    bool    eof() override;
    void    rewind() override;
    void    close() override;

    FILE *file;
};

// -------------------------------------
class MemoryStream : public Stream
{
public:
    MemoryStream(void *p, int l, bool aOwn = false)
        : buf((char *)p)
        , len(l)
        , pos(0)
        , own(aOwn)
    {
    }

    MemoryStream(int l)
        : buf(new char[l])
        , len(l)
        , pos(0)
        , own(true)
    {
    }

    ~MemoryStream()
    {
        if (own)
            free();
    }

    void readFromFile(FileStream &file)
    {
        free(); // free old buffer

        len = file.length();
        buf = new char[len];
        pos = 0;
        own = true;
        file.read(buf, len);
    }

    int read(void *p, int l) override
    {
        if (pos+l <= len)
        {
            memcpy(p, &buf[pos], l);
            pos += l;
            return l;
        }else
        {
            memset(p, 0, l);
            return 0;
        }
    }

    void write(const void *p, int l) override
    {
        if ((pos+l) > len)
            throw StreamException("Stream - premature end of write()");
        memcpy(&buf[pos], p, l);
        pos += l;
    }

    bool eof() override
    {
        return pos >= len;
    }

    void rewind() override
    {
        pos = 0;
    }

    void seekTo(int p) override
    {
        pos = p;
    }

    int getPosition() override
    {
        return pos;
    }

    void    convertFromBase64();

    char    *buf;
    int     len, pos;
    bool    own;

private:
    void free()
    {
        delete[] buf;
        buf = nullptr;
    }
};

// --------------------------------------------------
class IndirectStream : public Stream
{
public:

    void init(Stream *s)
    {
        stream = s;
    }

    int read(void *p, int l) override
    {
        return stream->read(p, l);
    }

    void write(const void *p, int l) override
    {
        stream->write(p, l);
    }

    bool eof() override
    {
        return stream->eof();
    }

    void close() override
    {
        stream->close();
    }

    Stream *stream;
};

// --------------------------------------------------
class WriteBufferedStream : public IndirectStream
{
    static const int kBufSize = 64 * 1024;

public:
    WriteBufferedStream(Stream *s)
    {
        init(s);
    }

    ~WriteBufferedStream()
    {
        try
        {
            flush();
        }
        catch (StreamException& e)
        {
            LOG_ERROR("StreamException in dtor of WriteBufferedStream: %s", e.msg);
        }
    }

    int read(void *p, int l) override
    {
        flush();
        return stream->read(p, l);
    }

    void flush()
    {
        if (buf.size() > 0)
        {
            stream->write(buf.c_str(), buf.size());
            buf.clear();
        }
    }

    void write(const void *p, int l) override
    {
        if (l > kBufSize)
        {
            flush();
            stream->write(p, l);
        } else if (buf.size() + l > kBufSize)
        {
            for (int i = 0; i < l; i++)
                buf.push_back(static_cast<const char*>(p)[i]);
            flush();
        } else
        {
            for (int i = 0; i < l; i++)
                buf.push_back(static_cast<const char*>(p)[i]);
        }
    }

    void close() override
    {
        flush();
        stream->close();
    }

    std::string buf;
};
#endif

