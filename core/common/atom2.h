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

#ifndef _ATOM2_H
#define _ATOM2_H

#include <string>

#include "ip.h"
#include "gnuid.h"
#include "stream.h"
#include "id.h"

// ------------------------------------------------
class Atom
{
public:
    Atom();
    Atom(const ID4& name, std::initializer_list<Atom> lst);
    Atom(const ID4& name, int);
    Atom(const ID4& name, short);
    Atom(const ID4& name, char);
    Atom(const ID4& name, void*, size_t);
    Atom(const ID4& name, const char*);
    Atom(const ID4& name, const GnuID&);
    Atom(const ID4& name, const IP&);

    bool isNull() const;
    size_t size() const;

    int getInt() const;
    short getShort() const;
    std::string getBytes() const;
    void getBytes(void*, size_t) const;
    std::string getString() const;
    IP getAddress() const;
    GnuID getGnuID() const;

    void write(Stream&) const;
    std::string serialize() const;

    ID4 m_name;
    std::string m_data;
    std::vector<Atom> children;
};

#endif
