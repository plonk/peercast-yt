// ------------------------------------------------
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

#ifndef _FLAG_H
#define _FLAG_H

#include <vector>
#include <map>
#include <functional>
#include <atomic>
#include <string>
#include "varwriter.h"

class Flag
{
public:
    Flag(const std::string& name,
         const std::string& desc,
         bool defaultValue);
    Flag(const Flag& other);

    operator bool () const { return currentValue.load(); }
    Flag& operator = (bool v) { currentValue.store(v); return *this; }

    const std::string name, desc;
    const bool defaultValue;
    std::atomic_bool currentValue;
};

class FlagRegistory : public VariableWriter
{
public:
    FlagRegistory(std::vector<Flag>&& flags);

    Flag& get(const std::string&);

    void forEachFlag(std::function<void(Flag&)> func);

    amf0::Value getState() override;

    std::vector<Flag> m_flags;
    std::map<std::string,int> m_indices;
};

#endif
