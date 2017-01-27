#include <gtest/gtest.h>
#include "common.h"

TEST(HostTest, loopbackIP) {
    Host host;

    host.fromStrIP("127.0.0.1", 0);
    ASSERT_TRUE( host.loopbackIP() );

    // 127 で始まるクラスAのネットワーク全部がループバックとして機能す
    // るが、loopbackIP は 127.0.0.1 以外には FALSE を返す。
    host.fromStrIP("127.99.99.99", 0);
    ASSERT_FALSE( host.loopbackIP() );
}

TEST(HostTest, isMemberOf) {
    Host host, pattern;

    host.fromStrIP("192.168.0.1", -1);
    pattern.fromStrIP("192.168.0.1", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.0.2", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.0.255", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.255.255", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.255.1", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("0.0.0.0", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));

    host.fromStrIP("0.0.0.0", -1);
    pattern.fromStrIP("0.0.0.0", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));
}
