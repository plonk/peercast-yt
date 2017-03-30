#include "uri.h"

#include <boost/network/protocol/http/client.hpp>

using namespace boost::network::uri;

URI::URI(const std::string& uriString)
    : m_uri(new uri(uriString))
{
}

URI::~URI()
{
    delete m_uri;
}

bool        URI::isValid()
{
    return m_uri->is_valid();
}

std::string URI::scheme()
{
    return m_uri->scheme();
}

std::string URI::host()
{
    return m_uri->host();
}

int         URI::port()
{
    if (m_uri->port() == "")
        return defaultPort(scheme());
    else {
        int p;
        sscanf(m_uri->port().c_str(), "%d", &p);
        return p;
    }
}

std::string URI::path()
{
    if (m_uri->path() == "")
        return "/";
    else
        return m_uri->path();
}

std::string URI::fragment()
{
    return m_uri->fragment();
}

int URI::defaultPort(const std::string& scheme)
{
    if (scheme == "http")
        return 80;
    else
        return 0;
}
