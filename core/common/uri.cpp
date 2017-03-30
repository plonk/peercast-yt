#include "uri.h"

#include "LUrlParser.h"

using namespace LUrlParser;

URI::URI(const std::string& uriString)
    : m_uri(new clParseURL())
{
    *m_uri = clParseURL::ParseURL(uriString);
}

URI::~URI()
{
    delete m_uri;
}

bool        URI::isValid()
{
    return m_uri->IsValid();
}

std::string URI::scheme()
{
    return m_uri->m_Scheme;
}

std::string URI::user_info()
{
    return m_uri->m_UserName + ":" + m_uri->m_Password;
}

std::string URI::host()
{
    return m_uri->m_Host;
}

int         URI::port()
{
    int p;
    if (m_uri->GetPort(&p)) {
        return p;
    } else return -1;
}

std::string URI::path()
{
    return m_uri->m_Path;
}

std::string URI::query()
{
    return m_uri->m_Query;
}

std::string URI::fragment()
{
    return m_uri->m_Fragment;
}

int URI::defaultPort(const std::string& scheme)
{
    if (scheme == "http")
        return 80;
    else
        return 0;
}
