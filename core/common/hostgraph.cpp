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

#include "hostgraph.h"

#include "host.h"
#include "chanhit.h"

using json = nlohmann::json;

const HostGraph::ID HostGraph::kNullID = { Host(), Host() };

HostGraph::HostGraph(const ChanHit& self, ChanHitList *hitList)
{
    if (hitList == nullptr)
        throw std::invalid_argument("hitList");

    m_hit[id(self)] = self;

    for (auto p = hitList->hit;
         p;
         p = p->next)
    {
        LOG_DEBUG("HostGraph: %s", p->rhost[0].str().c_str());

        m_hit[id(*p)] = *p;
        m_hit[id(*p)].next = nullptr;
    }

    for (auto& pair : m_hit) {
        auto& id0 = pair.first;
        auto& hit = pair.second;
        bool found = false;

        // tracker
        if (hit.uphost == Host()) {
            m_children[kNullID].push_back(id0);
            found = true;
        }

        // wan relay (fetch)
        if (!found) {
            for (const auto& entry : m_hit) {
                auto& id1 = entry.first;
                if (id1.first == hit.uphost) {
                    found = true;
                    m_children[id1].push_back(id0);
                    break;
                }
            }
        }

        // wan relay (push)
        if (!found) {
            for (const auto& entry : m_hit) {
                auto& id1 = entry.first;
                if (id1.first.ip == hit.uphost.ip) {
                    found = true;
                    m_children[id1].push_back(id0);
                    break;
                }
            }
        }

        // lan relay (fetch)
        if (!found) {
            for (const auto& entry : m_hit) {
                auto& id1 = entry.first;
                if (id1.first.ip == hit.rhost[0].ip &&
                    id1.second == hit.uphost) {
                    found = true;
                    m_children[id1].push_back(id0);
                    break;
                }
            }
        }

        if (!found) {
            m_children[kNullID].push_back(id0);
        }
    }
}

std::pair<Host, Host> HostGraph::id(const ChanHit& hit)
{
    return { hit.rhost[0], hit.rhost[1] };
}

json HostGraph::toRelayTree(ID& endpoint, const std::vector<ID> path)
{
    ChanHit& hit = m_hit[endpoint];
    json::array_t children;

    for (ID& child : m_children[endpoint])
    {
        if (find(path.begin(), path.end(), child) == path.end())
        {
            std::vector<ID> p = path;
            p.push_back(endpoint);
            children.push_back(toRelayTree(child, p));
        }
        else
        {
            LOG_WARN("toRelayTree: circularity detected.");
        }
    }

    return {
        { "sessionId", (std::string) hit.sessionID },
        { "address", hit.rhost[0].ip.str() },
        { "port", hit.rhost[0].port }, // ペカステに合わせて 0 を入れないようにすべき？
        { "isFirewalled", hit.firewalled },
        { "localRelays", hit.numRelays },
        { "localDirects", hit.numListeners },
        { "isTracker", hit.tracker },
        { "isRelayFull", !hit.relay },
        { "isDirectFull", !hit.direct },
        { "isReceiving", hit.recv },
        { "isControlFull", !hit.cin },
        { "version", hit.version },
        { "versionString", hit.versionString() },
        { "children", children }
    };
}

json::array_t HostGraph::getRelayTree()
{
    json::array_t result;

    for (ID& root : m_children[kNullID])
    {
        result.push_back(toRelayTree(root, {root}));
    }
    return result;
}
