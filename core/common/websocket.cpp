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

#include "websocket.h"

namespace ws {

std::shared_ptr<Frame> createTextFrame(const char* payload, size_t plsize)
{
    std::string str;
    str.push_back(0x81);
    if (plsize >= 65536) {
        str.push_back(127);
        str.push_back((plsize >> 56)&0xFF);
        str.push_back((plsize >> 48)&0xFF);
        str.push_back((plsize >> 40)&0xFF);
        str.push_back((plsize >> 32)&0xFF);
        str.push_back((plsize >> 24)&0xFF);
        str.push_back((plsize >> 16)&0xFF);
        str.push_back((plsize >> 8 )&0xFF);
        str.push_back((plsize >> 0 )&0xFF);
    } else if (plsize >= 126) {
        str.push_back(126);
        str.push_back((plsize >> 8 )&0xFF);
        str.push_back((plsize >> 0 )&0xFF);
    } else {
        str.push_back(plsize);
    }
    str += std::string(payload, payload + plsize);

    return std::make_shared<Frame>(str);
}

Frame::Frame(const std::string& str)
    : m_data(str)
{
}

} // namespace ws
