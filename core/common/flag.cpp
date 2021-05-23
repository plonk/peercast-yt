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

#include "flag.h"

Flag::Flag(const std::string& aName,
           const std::string& aDesc,
           bool aDefaultValue)
    : name(aName)
    , desc(aDesc)
    , defaultValue(aDefaultValue)
    , currentValue(aDefaultValue)
{
}

Flag::Flag(const Flag& other)
    : desc(other.desc)
    , name(other.name)
    , defaultValue(other.defaultValue)
    , currentValue(other.currentValue.load())
{
}

bool Flag::load() const
{
    return currentValue.load();
}

void Flag::store(bool value)
{
    currentValue.store(value);
}

FlagRegistory::FlagRegistory(std::vector<Flag>&& flags)
    : m_flags(flags)
{
    for (int i = 0; i < m_flags.size(); ++i) {
        m_indices[m_flags[i].name] = i;
    }
}

Flag& FlagRegistory::get(const std::string& name)
{
    return m_flags.at(m_indices.at(name));
}

void FlagRegistory::forEachFlag(std::function<void(Flag&)> func)
{
    for (auto& flag : m_flags) {
        func(flag);
    }
}
