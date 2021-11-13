#include <gtest/gtest.h>
#include "waitablequeue.h"
#include <thread>
#include <functional>

class WaitableQueueFixture : public ::testing::Test {
public:
    WaitableQueueFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~WaitableQueueFixture()
    {
    }
};

TEST_F(WaitableQueueFixture, simplestCase)
{
    WaitableQueue<int> q;
    q.enqueue(1);
    ASSERT_EQ(q.dequeue(), 1);
}

TEST_F(WaitableQueueFixture, firstInFirstOut)
{
    WaitableQueue<int> q;
    q.enqueue(1);
    q.enqueue(2);
    q.enqueue(3);
    ASSERT_EQ(q.dequeue(), 1);
    ASSERT_EQ(q.dequeue(), 2);
    ASSERT_EQ(q.dequeue(), 3);
}

TEST_F(WaitableQueueFixture, dequeueBlocksProperly)
{
    using namespace std;
    WaitableQueue<int> q;

    auto producer = thread([&]()
                           {
                               this_thread::sleep_for(chrono::milliseconds(100));
                               for (int i = 1; i <= 10; i++)
                                   q.enqueue(i);
                           });
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        sum += q.dequeue();
    }
    producer.join();
    ASSERT_EQ(sum, 55);
}

TEST_F(WaitableQueueFixture, multipleProducers)
{
    using namespace std;
    WaitableQueue<int> q;

    auto p1 = thread([&]()
                     {
                         for (int i = 0; i < 10000; i++) {
                             q.enqueue(1);
                         }
                     });
    auto p2 = thread([&]()
                     {
                         for (int i = 0; i < 10000; i++)
                             q.enqueue(2);
                     });
    auto p3 = thread([&]()
                     {
                         for (int i = 0; i < 10000; i++)
                             q.enqueue(3);
                     });
    int sum = 0;
    for (int i = 0; i < 30000; i++) {
        sum += q.dequeue();
    }
    p1.join(); p2.join(); p3.join();
    ASSERT_EQ(sum, 10000 + 20000 + 30000);
}

TEST_F(WaitableQueueFixture, multipleConsumers)
{
    using namespace std;
    WaitableQueue<int> q;

    for (int i = 0; i < 10000; i++)
        q.enqueue(1);
    for (int i = 0; i < 10000; i++)
        q.enqueue(2);
    for (int i = 0; i < 10000; i++)
        q.enqueue(3);

    int total = 0;
    mutex m;

    function<void(void)> f = [&]()
                             {
                                 int sum = 0;
                                 for (int i = 0; i < 10000; i++)
                                     sum += q.dequeue();
                                 m.lock();
                                 total += sum;
                                 m.unlock();
                             };
    auto p1 = thread(f);
    auto p2 = thread(f);
    auto p3 = thread(f);
    p1.join(); p2.join(); p3.join();
    ASSERT_EQ(total, 60000);
}
