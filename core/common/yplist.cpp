#include "yplist.h"

#include <functional>

YPList* g_ypList;

namespace {
    template <typename T, typename U>
    std::vector<U> map(std::vector<T>& vec, std::function<U(T&)> func)
    {
        std::vector<U> result;
        for (auto& elt : vec) {
            result.push_back(func(elt));
        }
        return result;
    }
}

YP::YP(const std::string& aName, const std::string& aFeedUrl, const std::string& aRootHost)
    : name(aName)
    , feedUrl(aFeedUrl)
    , rootHost(aRootHost)
{
}

amf0::Value YP::getState()
{
    return amf0::Value::object(
        {
            { "name", name },
            { "feedUrl", feedUrl },
            { "rootHost", rootHost },
        });
}

YPList::YPList()
    : m_list(
        {
            // { name, feedUrl, rootHost }
            { "SP", "http://bayonet.ddo.jp/sp/index.txt", "bayonet.ddo.jp:7146" },
            { "Heisei", "http://yp.pcgw.pgw.jp/index.txt", "yp.pcgw.pgw.jp:7146" },
            { "P@", "https://p-at.net/index.txt", "root.p-at.net" },
            { "YPv6", "http://ypv6.pecastation.org/index.txt", "ypv6.pecastation.org" },
            { "Event YP", "http://eventyp.xrea.jp/index.txt", "" },
        })
{
}

amf0::Value YPList::getState()
{
    std::function<amf0::Value(YP&)> stateOf = [](YP& yp) { return yp.getState(); };

    return amf0::Value::object(
        {
            { "list", map(m_list, stateOf) } 
        });
}
