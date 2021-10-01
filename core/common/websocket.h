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

#ifndef _WEB_SOCKET_H
#define _WEB_SOCKET_H

#include "stream.h"

namespace ws {

    enum Opcode {
        OP_CONT = 0x0,
        OP_TEXT = 0x1,
        OP_BINARY = 0x2,
        OP_CLOSE = 0x8,
        OP_PING = 0x9,
        OP_PONG = 0xA,
    };

    class Frame
    {
    public:
        Frame(const std::string&);
        //~Frame();

        bool isFinalFragment();
        Opcode getOpcode();
    
        std::string getPayload();

        const char* data() { return m_data.data(); }
        size_t getSize() { return m_data.size(); }

        std::string m_data;
    };

    std::shared_ptr<Frame> createTextFrame(const char*, size_t);
    std::shared_ptr<Frame> createTextFrame(const std::string&);
    std::shared_ptr<Frame> createBinaryFrame(const char*, size_t);
    std::shared_ptr<Frame> createBinaryFrame(const std::string&);
    std::shared_ptr<Frame> recvFrame(Stream&);

} // namespace ws

#endif
