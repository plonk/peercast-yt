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

HostGraph::HostGraph(std::shared_ptr<Channel> ch, ChanHitList *hitList, int ipVersion)
{
    if (ch == nullptr)
        throw std::invalid_argument("ch");

    if (hitList == nullptr)
        throw std::invalid_argument("hitList");

    // 自分を追加する。
    {
        Host uphost;
        ChanHit self;
        bool isTracker = ch->isBroadcasting();

        if (!isTracker)
            uphost = ch->sourceHost.host;

        self.initLocal(ch->localListeners(),
                       ch->localRelays(),
                       ch->info.numSkips,
                       ch->info.getUptime(),
                       ch->isPlaying(),
                       ch->rawData.getOldestPos(),
                       ch->rawData.getLatestPos(),
                       ch->canAddRelay(),
                       uphost,
                       (ipVersion == 6));
        self.tracker = isTracker;

        m_hit[endpoint(&self)] = self;
        m_children[self.uphost].push_back(endpoint(&self));
    }

    for (ChanHit *p = hitList->hit;
         p;
         p = p->next)
    {
        Host h = endpoint(p);

        LOG_DEBUG("HostGraph: endpoint = %s", h.IPtoStr().cstr());

        if (!h.ip)
            continue;

        m_hit[h] = *p;
        m_hit[h].next = nullptr;

        m_children[p->uphost].push_back(h);
    }

    // 上流の項目が見付からないノードはIPの同じポート違いのノー
    // ドか、ルートにする。
    for (auto& pair : m_hit) {
        auto& host = pair.first;
        auto& hit = pair.second;

        if (hit.uphost == Host())
            continue;

        if (m_hit.count(hit.uphost) == 0) {
            auto it = std::find_if(m_hit.begin(), m_hit.end(),
                                   [&](std::pair<Host,ChanHit> p) {
                                       return (p.first.ip == hit.uphost.ip);
                                   });
            if (it != m_hit.end()) {
                LOG_TRACE("HostGraph: %s adopts %s (uphost %s)",
                          it->first.str().c_str(),
                          endpoint(&hit).str().c_str(),
                          hit.uphost.str().c_str());
                m_children[it->first].push_back(endpoint(&hit));
            } else {
                m_children[Host()].push_back(endpoint(&hit));
            }
        }
    }
}

Host HostGraph::endpoint(ChanHit *hit)
{
    if (hit->rhost[0].ip)
        return hit->rhost[0];
    else
        return hit->rhost[1];
}

json HostGraph::toRelayTree(Host& endpoint, const std::vector<Host> path)
{
    ChanHit& hit = m_hit[endpoint];
    json::array_t children;

    for (Host& child : m_children[endpoint])
    {
        if (find(path.begin(), path.end(), child) == path.end())
        {
            std::vector<Host> p = path;
            p.push_back(endpoint);
            children.push_back(toRelayTree(child, p));
        }
        else
        {
            LOG_DEBUG("toRelayTree: circularity detected. skipping %s", ((std::string) child).c_str());
        }
    }

    return {
        { "sessionId", (std::string) hit.sessionID },
        { "address", endpoint.IPtoStr().cstr() },
        { "port", endpoint.port },
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

    for (Host& root : m_children[Host()])
    {
        result.push_back(toRelayTree(root, std::vector<Host>()));
    }
    return result;
}
