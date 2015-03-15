#include "gtest/gtest.h"
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <queue>
#include <algorithm>
#include <random>
#include "schedulerfactory.h"
#include <thread>
#include <atomic>
#include <chrono>

using std::cout;
using std::endl;
using std::atomic;
using std::thread;
using namespace n_tools;
typedef ExampleItem t_TypeUsed;

/**
 * @brief pusher Threadtask, push totalsize item on scheduler, mark finish with writer_done.
 */
void pusher(std::mutex& queuelock, std::atomic<int>& writer_done, const int totalsize,
        const SchedulerFactory<t_TypeUsed>::t_Scheduler& scheduler, const int id)
{
	std::atomic<int> count(0);
	std::vector<int> rands;
	const int base = id * totalsize;
	for (int j = 0; j < totalsize; ++j) {
		rands.push_back(base + j);
	}
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	shuffle(rands.begin(), rands.end(), std::default_random_engine(seed));
	for (int i = 0; i < totalsize; ++i) {
		std::lock_guard<std::mutex> m_lock(queuelock);
		t_TypeUsed q(rands[i]);
		scheduler->push_back(q); // scheduler is locked on single operations.
		count.fetch_add(1);
	}
	writer_done.fetch_add(1);
	EXPECT_EQ(count, totalsize);
}

/**
 * @brief popper Pop until all pushers are done & scheduler is empty.
 */
void popper(std::mutex& queuelock, const std::atomic<int>& writer_done, int pushcount,
        const SchedulerFactory<t_TypeUsed>::t_Scheduler& scheduler)
{
	while (true) {
		{   /// Use RAII to ensure locking.
			std::lock_guard<std::mutex> lock_sched(queuelock);
			if (scheduler->empty()) {
				if (writer_done == pushcount) {
					return; // mutex is unlocked by RAII
				}
			} else {
				scheduler->pop();
			}
		}
	}
}

class SchedulerTest: public ::testing::Test
{
public:
	SchedulerTest()
	{
		scheduler = SchedulerFactory<t_TypeUsed>::makeScheduler(Storage::BINOMIAL);
	}

	void SetUp()
	{
		scheduler->clear();
		ASSERT_EQ(scheduler->empty(), true);
	}

	void TearDown()
	{
		scheduler->clear();
	}

	~SchedulerTest()
	{
		// cleanup any pending stuff, but no exceptions allowed
	}

	SchedulerFactory<t_TypeUsed>::t_Scheduler scheduler;
};

/**
 * Core operations on the scheduler (unthreaded):
 * 	tests : size(), push.pop(), contains().
 */
TEST_F(SchedulerTest, basic_push)
{
	scheduler->push_back(1);
	EXPECT_EQ(scheduler->contains(1), true);
	scheduler->push_back(3);
	EXPECT_EQ(scheduler->contains(3), true);
	scheduler->push_back(2);
	EXPECT_EQ(scheduler->contains(2), true);
	EXPECT_EQ(scheduler->size(), 3);
	EXPECT_EQ(scheduler->contains(42), false);
}

/**
 * Verifies that ordering is respected (heap order),
 */
TEST_F(SchedulerTest, basic_push_order)
{
	scheduler->push_back(1);
	scheduler->push_back(3);
	scheduler->push_back(2);
	EXPECT_EQ(scheduler->size(), 3);
	size_t pops = scheduler->size();
	for (size_t i = 1; i < pops + 1; ++i) {
		t_TypeUsed q = scheduler->pop();
		EXPECT_EQ(q, i);
		EXPECT_EQ(scheduler->contains(q), false);
	}
}

TEST_F(SchedulerTest, mutability)
{
	for (size_t i = 0; i < 10; ++i) {
		scheduler->push_back(i);
		EXPECT_EQ(scheduler->contains(i), true);
		EXPECT_EQ(scheduler->erase(i), true);
		EXPECT_EQ(scheduler->contains(i), false);
	}
}

TEST_F(SchedulerTest, basic_unschedule_until)
{
	for (size_t i = 0; i < 10; ++i) {
		scheduler->push_back(i);
	}
	EXPECT_EQ(scheduler->size(), 10);
	std::vector<t_TypeUsed> rem;
	scheduler->unschedule_until(rem, 5);
	EXPECT_EQ(rem.size(), 6);
	EXPECT_EQ(scheduler->size(), 4);
	for (size_t i = 0; i < 4; ++i) {
		scheduler->top();
		scheduler->pop();
	}
	EXPECT_EQ(scheduler->size(), 0);
	EXPECT_EQ(scheduler->empty(), true);
}

/**
 *  Specifically designed to trap conccurency errors. Keeping writer/readers even over max supported threads by hw
 *  introduces maximum stress on locking code, without starving. Note that VM size can be extreme for this testcase (TB's is not unusual).
 *  @note (in reality there is 1 thread allocated extra (main = thread 0), but no cpu that I know of has odd threadcount.
 */
TEST_F(SchedulerTest, Concurrency_evenwritersreaders)
{
	const int totalsize = 100; // On an i5 quad core , 50000 elems requires approx 1 min.
	const int threadcount = std::thread::hardware_concurrency();  // 1 = main
	const int pushcount = threadcount / 2;  //e.g. 4 -1 = 3 , 3/2 = 1 pusher
	std::atomic<int> writer_done(0);
	std::mutex pqueue_mutex;
	std::vector<thread> threads(threadcount);
	for (auto i = 0; i < threadcount; ++i) {
		if (i < pushcount) {
			threads[i] = std::thread(pusher, std::ref(pqueue_mutex), std::ref(writer_done), totalsize, std::cref(scheduler), i);
		} else {
			threads[i] = std::thread(popper, std::ref(pqueue_mutex), std::cref(writer_done), pushcount,
			        std::cref(scheduler));
		}
	};

	for (auto& t : threads)
		t.join();
	EXPECT_EQ(scheduler->size(), 0);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

/**
 *  Starve the readers with a single writer.
 */
TEST_F(SchedulerTest, Concurrency_1writerkreaders)
{
	const int totalsize = 100;
	const int threadcount = std::thread::hardware_concurrency();  // 1 = main
	const int pushcount = 1;
	std::atomic<int> writer_done(0);
	std::mutex pqueue_mutex;
	std::vector<thread> threads(threadcount);
	for (auto i = 0; i < threadcount; ++i) {
		if (i < pushcount) {
			threads[i] = std::thread(pusher, std::ref(pqueue_mutex), std::ref(writer_done), totalsize, std::cref(scheduler), i);
		} else {
			threads[i] = std::thread(popper, std::ref(pqueue_mutex), std::cref(writer_done), pushcount,
			        std::cref(scheduler));
		}
	};

	for (auto& t : threads)
		t.join();
	EXPECT_EQ(scheduler->size(), 0);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

/**
 * What happens if nr threads by far exceeds physical capability ?
 */
TEST_F(SchedulerTest, Concurrency_threadoverload)
{
	const int totalsize = 50;
	const int threadcount = std::thread::hardware_concurrency() * 2;
	const int pushcount = threadcount / 2;
	std::atomic<int> writer_done(0);
	std::mutex pqueue_mutex;
	std::vector<thread> threads(threadcount);
	for (auto i = 0; i < threadcount; ++i) {
		if (i < pushcount) {
			threads[i] = std::thread(pusher, std::ref(pqueue_mutex), std::ref(writer_done), totalsize, std::cref(scheduler), i);
		} else {
			threads[i] = std::thread(popper, std::ref(pqueue_mutex), std::cref(writer_done), pushcount,
			        std::cref(scheduler));
		}
	};

	for (auto& t : threads)
		t.join();
	EXPECT_EQ(scheduler->size(), 0);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

class UnSyncedSchedulerTest: public ::testing::Test
{
public:
	UnSyncedSchedulerTest()
	{
		scheduler = SchedulerFactory<t_TypeUsed>::makeScheduler(Storage::BINOMIAL, true);
	}

	void SetUp()
	{
		scheduler->clear();
		ASSERT_EQ(scheduler->empty(), true);
	}

	void TearDown()
	{
		scheduler->clear();
	}

	~UnSyncedSchedulerTest()
	{
		// cleanup any pending stuff, but no exceptions allowed
	}

	SchedulerFactory<t_TypeUsed>::t_Scheduler scheduler;
};

/**
 * Core operations on the scheduler (unthreaded):
 * 	tests : size(), push.pop(), contains().
 */
TEST_F(UnSyncedSchedulerTest, basic_push)
{
	scheduler->push_back(1);
	EXPECT_EQ(scheduler->contains(1), true);
	scheduler->push_back(3);
	EXPECT_EQ(scheduler->contains(3), true);
	scheduler->push_back(2);
	EXPECT_EQ(scheduler->contains(2), true);
	EXPECT_EQ(scheduler->size(), 3);
	EXPECT_EQ(scheduler->contains(42), false);
}

/**
 * Verifies that ordering is respected (heap order),
 */
TEST_F(UnSyncedSchedulerTest, basic_push_order)
{
	scheduler->push_back(1);
	scheduler->push_back(3);
	scheduler->push_back(2);
	EXPECT_EQ(scheduler->size(), 3);
	size_t pops = scheduler->size();
	for (size_t i = 1; i < pops + 1; ++i) {
		t_TypeUsed q = scheduler->pop();
		EXPECT_EQ(q, i);
		EXPECT_EQ(scheduler->contains(q), false);
	}
}

TEST_F(UnSyncedSchedulerTest, mutability)
{
	for (size_t i = 0; i < 10; ++i) {
		scheduler->push_back(i);
		EXPECT_EQ(scheduler->contains(i), true);
		EXPECT_EQ(scheduler->erase(i), true);
		EXPECT_EQ(scheduler->contains(i), false);
	}
}

TEST_F(UnSyncedSchedulerTest, basic_unschedule_until)
{
	for (size_t i = 0; i < 10; ++i) {
		scheduler->push_back(i);
	}
	EXPECT_EQ(scheduler->size(), 10);
	std::vector<t_TypeUsed> rem;
	scheduler->unschedule_until(rem, 5);
	EXPECT_EQ(rem.size(), 6);
	EXPECT_EQ(scheduler->size(), 4);
	for (size_t i = 0; i < 4; ++i) {
		scheduler->top();
		scheduler->pop();
	}
	EXPECT_EQ(scheduler->size(), 0);
	EXPECT_EQ(scheduler->empty(), true);
}
