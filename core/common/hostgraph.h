// -*- mode: c++ -*-
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

#ifndef _HOSTGRAPH_H
#define _HOSTGRAPH_H

#include "json.hpp"

class Host;
class ChanHit;
class ChanHitList;

#include <channel.h>
#include <chanhit.h>

class HostGraph
{
    using json = nlohmann::json;

public:
    HostGraph(std::shared_ptr<Channel> ch, ChanHitList *hitList, int ipVersion);

    json::array_t getRelayTree();

    Host endpoint(ChanHit *hit);
    json toRelayTree(Host& endpoint, const std::vector<Host> path);

    std::map<Host, ChanHit> m_hit;
    std::map<Host, std::vector<Host> > m_children;
};

#endif
