#ifndef _MATROSKA_H
#define _MATROSKA_H

#include <stdint.h>
#include <string>
#include <map>

#include "stream.h"

namespace matroska
{

typedef std::basic_string<uint8_t> byte_string;

const std::map<std::basic_string<uint8_t>, std::string>
ID_TO_NAME = {
    { {0x1A,0x45,0xDF,0xA3}, "EBML" },
    { {0x18,0x53,0x80,0x67}, "Segment" },
    { {0x1F,0x43,0xB6,0x75}, "Cluster" },
    { {0xA3}, "SimpleBlock" },
    { {0x42,0x86}, "EBMLVersion" },
    { {0x42,0xF7}, "EBMLReadVersion" },
    { {0x42,0xF2}, "EBMLMaxIDLength" },
    { {0x42,0xF3}, "EBMLMaxSizeLength" },
    { {0x42,0x82}, "DocType" },
    { {0x42,0x87}, "DocTypeVersion" },
    { {0x42,0x85}, "DocTypeReadVersion" },
    { {0x11,0x4D,0x9B,0x74}, "SeekHead" },
    { {0xEC}, "Void" },
    { {0x16,0x54,0xAE,0x6B}, "Tracks" },
    { {0x12,0x54,0xC3,0x67}, "Tags" },
    { {0x15,0x49,0xA9,0x66}, "Info" },
    { {0xE7}, "Timecode" },
    { {0x1C,0x53,0xBB,0x6B}, "Cues" },
    { {0x2A,0xD7,0xB1}, "TimecodeScale" },
    { {0x4D,0x80}, "MuxingApp" },
    { {0x44,0x89}, "Duration" },
    { {0x57,0x41}, "WritingApp" },
    { {0x73,0xA4}, "SegmentUID" },
    { {0x73,0x73}, "Tag" },
    { {0x63,0xC0}, "Targets" },
    { {0x67,0xC8}, "SimpleTag" },
    { {0x63,0xC5}, "TagTrackUID" },
    { {0x44,0x87}, "TagString" },
    { {0x45,0xA3}, "TagName" },
    { {0x4D,0xBB}, "Seek" },
    { {0x53,0xAB}, "SeekID" },
    { {0x53,0xAC}, "SeekPosition" },
};

// UNKNOWN 値の対応要る？
class VInt
{
public:
    VInt(const std::basic_string<uint8_t>& aBytes)
        : bytes(aBytes)
    {
        if (bytes[0] == 0xff)
            throw std::runtime_error("UNKNOWN value not supported");
        auto nzeroes = numLeadingZeroes(bytes[0]);
        if (nzeroes > 7)
            throw std::runtime_error("bad data");
    }

    uint64_t uint()
    {
        auto zeroes = numLeadingZeroes(bytes[0]);
        auto len = zeroes + 1;
        uint64_t value = ((bytes[0] << len) & 0xff) >> len;
        for (int i = 0; i < zeroes; i++)
        {
            value <<= 8;
            value |= bytes[i+1];
        }
        return value;
    }

    static int numLeadingZeroes(uint8_t byte)
    {
        if (byte >= 0x80) return 0;
        if (byte >= 0x40) return 1;
        if (byte >= 0x20) return 2;
        if (byte >= 0x10) return 3;
        if (byte >= 0x08) return 4;
        if (byte >= 0x04) return 5;
        if (byte >= 0x02) return 6;
        if (byte >= 0x01) return 7;
        return 8;
    }

    static VInt read(Stream& is)
    {
        uint8_t b = is.readChar();
        int nzeroes = numLeadingZeroes(b);
        if (nzeroes > 7)
            throw std::runtime_error("bad data");
        std::basic_string<uint8_t> bytes;
        bytes += b;
        for (int i = 0; i < nzeroes; i++)
            bytes += is.readChar();
        return VInt(bytes);
    }

    std::string toName()
    {
        auto it = ID_TO_NAME.find(bytes);
        if (it != ID_TO_NAME.end())
            return it->second;
        else
            return "Unknown";
    }

    std::basic_string<uint8_t> bytes;
};

} // namespace matroska

#endif
