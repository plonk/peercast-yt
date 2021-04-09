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

// IPアドレスはパターン文字列に対してマッチされる。パターン文字列は
// IPv4 アドレス、ホスト名、または . で始まる FQDN のサフィックスであ
// る。IPv4 アドレスで値が 255 のフィールドはワイルドカードとして扱われる。

// 例:
// "127.0.0.1" → 127.0.0.1
// "127.255.255.255" → 127.*.*.*
// "localhost" → "localhost" のIPv4アドレス。
// ".jp" → 逆引きホスト名が ".jp" で終わる。

// アドレスは 0.0.0.0 は「リセット状態」として扱われ、また空文字列のセッ
// トもフィルターをリセットする。

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

    enum Type
    {
        T_IP,
        T_HOSTNAME,
        T_SUFFIX,
        T_IPV6,
    };

    ServFilter() { init(); }
    void    init()
    {
        type = T_IP;
        flags = 0;
        host.init();
    }
    bool    writeVariable(Stream &, const String &) override;
    bool    matches(int fl, const Host& h) const;

    void setPattern(const char* str);
    std::string getPattern();

    bool isGlobal();
    bool isSet();
    Type type;
    unsigned int flags;
private:
    Host host;
    std::string pattern;
};
