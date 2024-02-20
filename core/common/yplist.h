#ifndef _YPLIST_H
#define _YPLIST_H

#include <string>
#include <vector>

#include "varwriter.h"
#include "amf0.h"

class YP : public VariableWriter
{
public:
    YP(const std::string&, const std::string&, const std::string&);

    amf0::Value getState() override;

    std::string name;
    std::string feedUrl;
    std::string rootHost;
};

class YPList : public VariableWriter
{
public:
    YPList();

    amf0::Value getState() override;

private:
    std::vector<YP> m_list;
};

extern YPList* g_ypList;

#endif
