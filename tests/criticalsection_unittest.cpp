#include <gtest/gtest.h>

#include "critsec.h"

class CriticalSectionFixture : public ::testing::Test {
public:
    CriticalSectionFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~CriticalSectionFixture()
    {
    }
};

TEST_F(CriticalSectionFixture, smokeTest)
{
    WLock lock;

    ASSERT_NO_THROW(
        {
            CriticalSection cs(lock);
        });
}

TEST_F(CriticalSectionFixture, recursive)
{
    WLock lock;

    // デッドロックしない。
    ASSERT_NO_THROW(
        {
            CriticalSection cs(lock);
            {
                CriticalSection cs(lock);
            }
        });
}
