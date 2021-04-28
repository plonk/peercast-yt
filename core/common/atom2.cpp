// (c) peercast.org
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

#include "atom2.h"
#include "sstream.h"

Atom::Atom(){}
Atom::Atom(const ID4& name, std::initializer_list<Atom> lst)
    : m_name(name)
{
    for (const auto& atom : lst)
        if (!atom.isNull())
            children.push_back(atom);
}

Atom::Atom(const ID4& name, int n)
    : m_name(name)
{
    StringStream s;
    s.writeInt(n);
    m_data = s.str();
}

Atom::Atom(const ID4& name, short n)
    : m_name(name)
{
    StringStream s;
    s.writeShort(n);
    m_data = s.str();
}

Atom::Atom(const ID4& name, void* data, size_t len)
    : m_name(name)
{
    m_data = std::string((char*) data, (char*) data + len);
}

Atom::Atom(const ID4& name, const char* str)
    : m_name(name)
{
    // include the null character at the end
    m_data = std::string(str, str + strlen(str) + 1);
}

Atom::Atom(const ID4& name, const GnuID& gnuid)
    : m_name(name)
{
    StringStream s;
    s.write(gnuid.id, 16);
    m_data = s.str();
}

Atom::Atom(const ID4& name, const IP& ip)
    : m_name(name)
{
    StringStream s;
    if (ip.isIPv4Mapped()) {
        s.writeInt(ip.ipv4());
    } else {
        in6_addr addr = ip.serialize();
        std::reverse(addr.s6_addr, addr.s6_addr + 16);
        s.write(addr.s6_addr, 16);
    }
    m_data = s.str();
}

bool Atom::isNull() const { return !m_name.isSet(); }
size_t Atom::size() const { return m_data.size(); }

int Atom::getInt() const
{
    StringStream s(m_data);
    return s.readInt();
}

short Atom::getShort() const
{
    StringStream s(m_data);
    return s.readShort();
}

std::string Atom::getBytes() const
{
    return m_data;
}

void Atom::getBytes(void* buf, size_t buflen) const
{
    if (buflen < m_data.size())
        throw GeneralException("insufficient buffer size");

    memcpy(buf, m_data.c_str(), m_data.size());
}

std::string Atom::getString() const
{
    return m_data;
}

IP Atom::getAddress() const
{
    StringStream s(m_data);

    if (m_data.size() == 4) {
        return s.readInt();
    } else if (m_data.size() == 16) {
        in6_addr addr;
        s.read(addr.s6_addr, 16);
        std::reverse(addr.s6_addr, addr.s6_addr + 16);
        return addr;
    } else {
        throw GeneralException("Bad atom data");
    }
}
GnuID Atom::getGnuID() const
{
    if (m_data.size() != 16)
        throw GeneralException("Bad atom data");

    StringStream s(m_data);
    GnuID gnuid;
    s.read(gnuid.id, 16);
    return gnuid;
}

void Atom::write(Stream& s) const
{
    s.write(const_cast<ID4*>(&m_name)->getData(), 4);

    if (children.size()) {
        s.writeInt(children.size() | 0x80000000);
        for (auto& atom : children)
            atom.write(s);
    } else {
        s.writeInt(m_data.size());
        s.write(m_data.c_str(), m_data.size());
    }
}

std::string Atom::serialize() const
{
    StringStream s;

    this->write(s);
    return s.str();
}
