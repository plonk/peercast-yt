// ------------------------------------------------
// File : servfilter.h (derived from servmgr.h)
// Author: giles
// Desc:
//      IPアドレスマッチング。
//
// (c) 2002 peercast.org
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

#include "varwriter.h"
#include "host.h"

class ServFilter : public VariableWriter
{
public:
    enum
    {
        F_PRIVATE  = 0x01,
        F_BAN      = 0x02,
        F_NETWORK  = 0x04,
        F_DIRECT   = 0x08
    };

    ServFilter() { init(); }
    void    init()
    {
        flags = 0;
        host.init();
    }
    bool    writeVariable(Stream &, const String &) override;
    bool    matches(int fl, const Host& h) const;

    Host host;
    unsigned int flags;
};
