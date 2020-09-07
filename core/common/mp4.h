// ------------------------------------------------
// File : mp4.h
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

#ifndef _MP4_H
#define _MP4_H

class MP4Stream : public ChannelStream
{
public:
    MP4Stream() : m_buffer(new uint8_t[2*1024*1024])
    {
    }
    ~MP4Stream()
     {
         delete[] m_buffer;
     }
    void readHeader(Stream &, std::shared_ptr<Channel>) override;
    int  readPacket(Stream &, std::shared_ptr<Channel>) override;
    void readEnd(Stream &, std::shared_ptr<Channel>) override;

 private:
    uint8_t *m_buffer;
    const int MAX_OUTGOING_PACKET_SIZE = 15 * 1024;
};

class MP4Box
{
 public:

 MP4Box() : m_data(nullptr)
        {
        }
    
    ~MP4Box()
        {
            if (m_data)
                delete[] m_data;
        }

    std::string type()
        {
            if (m_data) {
                // this is dangerous.
                return std::string(m_data + 4, m_data + 8);
            } else {
                return "";
            }
        }

    uint8_t *data() {
        return m_data;
    }

    void data(uint8_t *aData) {
        m_data = aData;
    }

    size_t size() {
        if (m_data) {
            return m_data[0] << 24 |
                m_data[1] << 16 |
                m_data[2] << 8|
                m_data[3];
        } else {
            return 0;
        }
    }

    uint8_t *m_data;
};

#endif
