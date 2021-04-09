#include <gtest/gtest.h>
#include "sstream.h"
#include "atom.h"

class AtomStreamFixture : public ::testing::Test {
public:
    AtomStreamFixture()
    {
        
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~AtomStreamFixture()
    {
    }
};

TEST_F(AtomStreamFixture, writeInt)
{
    StringStream s;
    AtomStream a(s);

    a.writeInt("ip", 127<<24|1);
    ASSERT_EQ(std::string({ 'i','p',0,0, 4,0,0,0, 1,0,0,127 }),
              s.str());
}

TEST_F(AtomStreamFixture, writeAddress4)
{
    StringStream s;
    AtomStream a(s);
    IP ip(127<<24|1);
    
    a.writeAddress("ip", ip);
    ASSERT_EQ(std::string({ 'i','p',0,0, 4,0,0,0, 1,0,0,127 }),
              s.str());
}

TEST_F(AtomStreamFixture, writeAddress16)
{
    StringStream s;
    AtomStream a(s);
    in6_addr addr = { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } };
    IP ip(addr);
    
    a.writeAddress("ip", ip);
    ASSERT_EQ(std::string({ 'i','p',0,0, 16,0,0,0, 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }),
              s.str());
}

TEST_F(AtomStreamFixture, readInt)
{
    StringStream s(std::string({ 'i','p',0,0, 4,0,0,0, 1,0,0,127 }));
    AtomStream a(s);

    int nchildren, size;
    ID4 id = a.read(nchildren, size);
    ASSERT_EQ(0, nchildren);
    ASSERT_EQ(4, size);
    ASSERT_EQ(127<<24|1, a.readInt());
}

TEST_F(AtomStreamFixture, readAddress4)
{
    StringStream s(std::string({ 'i','p',0,0, 4,0,0,0, 1,0,0,127 }));
    AtomStream a(s);

    int nchildren, size;
    ID4 id = a.read(nchildren, size);
    ASSERT_EQ(0, nchildren);
    ASSERT_EQ(4, size);
    ASSERT_EQ(IP(127<<24|1), a.readAddress());
}

TEST_F(AtomStreamFixture, readAddress16)
{
    StringStream s(std::string({ 'i','p',0,0, 16,0,0,0, 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }));
    AtomStream a(s);
    in6_addr addr = { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } };
    IP ip(addr);
    
    int nchildren, size;
    ID4 id = a.read(nchildren, size);
    ASSERT_EQ(0, nchildren);
    ASSERT_EQ(16, size);
    ASSERT_EQ(ip, a.readAddress());
}
