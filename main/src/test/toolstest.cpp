#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <queue>
#include <algorithm>
#include <random>
#include "tools/schedulerfactory.h"
#include <thread>
#include <atomic>
#include <chrono>
#include "tools/globallog.h"
#include "tools/coutredirect.h"
#include "tools/sharedvector.h"
#include "tools/listscheduler.h"
#include "tools/stlscheduler.h"
#include "tools/flags.h"
#include "model/modelentry.h"
#include "network/messageentry.h"

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
		std::lock_guard<std::mutex> m_lock(queuelock);	// This lock is required for threadsafety only on virtualbox with windows as bare metal os.
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
		scheduler = SchedulerFactory<t_TypeUsed>::makeScheduler(Storage::FIBONACCI, true);
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
	EXPECT_EQ(scheduler->size(), 3u);
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
	EXPECT_EQ(scheduler->size(), 3u);
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
	EXPECT_EQ(scheduler->size(), 10u);
	std::vector<t_TypeUsed> rem;
	scheduler->unschedule_until(rem, 5);
	EXPECT_EQ(rem.size(), 6u);
	EXPECT_EQ(scheduler->size(), 4u);
	for (size_t i = 0; i < 4; ++i) {
		scheduler->top();
		scheduler->pop();
	}
	EXPECT_EQ(scheduler->size(), 0u);
	EXPECT_EQ(scheduler->empty(), true);
}

/**
 *  Specifically designed to trap concurency errors. Keeping writer/readers even over max supported threads by hw
 *  introduces maximum stress on locking code, without starving. Note that VM size can be extreme for this testcase (TB's is not unusual).
 *  @note (in reality there is 1 thread allocated extra (main = thread 0), but no cpu that I know of has odd threadcount.
 */
TEST_F(SchedulerTest, Concurrency_evenwritersreaders)
{
	const int totalsize = 500; // On an i5 quad core , 50000 elems requires approx 1 min.
	const int threadcount = std::thread::hardware_concurrency();  // 1 = main
	if(std::thread::hardware_concurrency() <= 1){
		LOG_WARNING("Skipping threaded test, no support for threads on implementation !!");
		return;
	}
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
	EXPECT_EQ(scheduler->size(), 0u);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

/**
 *  Starve the readers with a single writer.
 */
TEST_F(SchedulerTest, Concurrency_1writerkreaders)
{
	const int totalsize = 500;
	const int threadcount = std::thread::hardware_concurrency();  // 1 = main
	if(std::thread::hardware_concurrency() <= 1){
		LOG_WARNING("Skipping threaded test, no support for threads on implementation !!");
		return;
	}
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
	EXPECT_EQ(scheduler->size(), 0u);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

/**
 * What happens if nr threads by far exceeds physical capability ?
 */
TEST_F(SchedulerTest, Concurrency_threadoverload)
{
	const int totalsize = 500;
	const int threadcount = std::thread::hardware_concurrency() * 2;
	if(std::thread::hardware_concurrency() <= 1){
		LOG_WARNING("Skipping threaded test, no support for threads on implementation !!");
		return;
	}
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
	EXPECT_EQ(scheduler->size(), 0u);
	EXPECT_EQ(scheduler->empty(), true);
	EXPECT_EQ(writer_done, pushcount);
}

class UnSyncedSchedulerTest: public ::testing::Test
{
public:
	UnSyncedSchedulerTest()
	{
		scheduler = SchedulerFactory<t_TypeUsed>::makeScheduler(Storage::LIST, false);
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
	EXPECT_EQ(scheduler->size(), 3u);
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
	EXPECT_EQ(scheduler->size(), 3u);
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
	EXPECT_EQ(scheduler->size(), 10u);
	std::vector<t_TypeUsed> rem;
	scheduler->unschedule_until(rem, 5);
	EXPECT_EQ(rem.size(), 6u);
	EXPECT_EQ(scheduler->size(), 4u);
	for (size_t i = 0; i < 4; ++i) {
		scheduler->top();
		scheduler->pop();
	}
	EXPECT_EQ(scheduler->size(), 0u);
	EXPECT_EQ(scheduler->empty(), true);
}

TEST(CoutRedirectTest, main_test){
	std::stringstream ssr1;
	std::stringstream ssr2;
	std::stringstream ssr3;
	{
		n_tools::CoutRedirect redirect(ssr1.rdbuf());
		std::cout << "This is written to ssr1.";
	}
	{
		n_tools::CoutRedirect redirect(ssr2.rdbuf());
		std::cout << "This is written to ssr2.";
	}
	{
		n_tools::CoutRedirect redirect(ssr1.rdbuf());
		std::cout << "This is MOAR text written to ssr1!";
		{
			n_tools::CoutRedirect redirect2(ssr3.rdbuf());
			std::cout << "This is written to ssr3, nested within ssr1.";
		}
		std::cout << "This is EVEN MOAR text written to ssr1!";
	}
	EXPECT_EQ(ssr1.str(), "This is written to ssr1.This is MOAR text written to ssr1!This is EVEN MOAR text written to ssr1!");
	EXPECT_EQ(ssr2.str(), "This is written to ssr2.");
	EXPECT_EQ(ssr3.str(), "This is written to ssr3, nested within ssr1.");
}


TEST(Flags, basic){
	std::size_t FREE = 1;
	std::size_t ISWAITING = 2;
	std::size_t IDLE = 4;
	std::size_t STOP = 8;
	std::size_t value = 0;
	EXPECT_TRUE(!n_tools::flag_is_set(value, FREE));
	EXPECT_TRUE(!n_tools::flag_is_set(value, ISWAITING));
	EXPECT_TRUE(!n_tools::flag_is_set(value, IDLE));
	EXPECT_TRUE(!n_tools::flag_is_set(value, STOP));
	n_tools::set_flag(value, FREE);
	n_tools::set_flag(value, ISWAITING);
	n_tools::set_flag(value, IDLE);
	n_tools::set_flag(value, STOP);
	EXPECT_TRUE(n_tools::flag_is_set(value, FREE));
	EXPECT_TRUE(n_tools::flag_is_set(value, ISWAITING));
	EXPECT_TRUE(n_tools::flag_is_set(value, IDLE));
	EXPECT_TRUE(n_tools::flag_is_set(value, STOP));
	n_tools::unset_flag(value, FREE);
	n_tools::unset_flag(value, ISWAITING);
	n_tools::unset_flag(value, IDLE);
	n_tools::unset_flag(value, STOP);
	EXPECT_TRUE(!n_tools::flag_is_set(value, FREE));
	EXPECT_TRUE(!n_tools::flag_is_set(value, ISWAITING));
	EXPECT_TRUE(!n_tools::flag_is_set(value, IDLE));
	EXPECT_TRUE(!n_tools::flag_is_set(value, STOP));
}

TEST(SharedVector, concurrency){
	/**
	 * Try to block as much as possible on the shared vector to trigger races/deadlock.
	 */
	size_t accesses = 100000;
	const size_t num_threads = std::thread::hardware_concurrency();
	accesses *= num_threads; // avoid ugly division below.
	if(num_threads < 2){
		LOG_INFO("Refusing to test threads without threading support.");
		return;
	}
	n_tools::SharedVector<size_t> protected_vector(num_threads, 0);
	std::vector<std::thread> threads;
	auto worker = [&]()->void{
		for(size_t i = 0; i<accesses; ++i){
			const size_t index = i % num_threads;
			protected_vector.lockEntry(index);
			const size_t value = protected_vector.get(index);
			protected_vector.set(index, value+1);
			protected_vector.unlockEntry(index);
		}
	};
	for(size_t j = 0; j<num_threads; ++j){
		threads.emplace_back(worker);
	}
	for(auto& thr : threads){
		thr.join();
	}
	for(size_t index = 0; index<protected_vector.size(); ++index){
		EXPECT_EQ(protected_vector.get(index), accesses);
	}
        
}

TEST(VectorScheduler, basic_ops){
        using n_model::ModelEntry;
        constexpr size_t limit = 1000;
        using n_network::t_timestamp;
        typedef boost::heap::fibonacci_heap<ModelEntry> heapchoice;
        VectorScheduler<heapchoice, ModelEntry> vscheduler;
        std::vector<ModelEntry> scheduled;
        for(size_t i = 0; i<limit; ++i){
                ModelEntry entry(i, t_timestamp(i, 0u));
                vscheduler.push_back(entry);
                scheduled.push_back(entry);
                EXPECT_TRUE(vscheduler.contains(entry));
                EXPECT_EQ(vscheduler.size(), i+1);
        }
        vscheduler.testInvariant();
        for(size_t i = 0; i<limit; ++i){
                ModelEntry entry(i, t_timestamp(424242422, 9999u));
                EXPECT_TRUE(vscheduler.contains(entry));
        }
        vscheduler.testInvariant();
        for(const auto& entry : scheduled){
                EXPECT_TRUE(vscheduler.contains(entry));
                if(size_t(entry)%2==0){
                        vscheduler.erase(entry);
                        EXPECT_TRUE(!vscheduler.contains(entry));
                }
        }
        
        vscheduler.testInvariant();
        EXPECT_EQ(vscheduler.size(), limit/2);
        size_t oldsize = vscheduler.size();
        std::vector<ModelEntry> popped;
        ModelEntry last(9999999, t_timestamp(limit/2, 0));
        vscheduler.unschedule_until(popped, last);
        EXPECT_EQ(vscheduler.size(), oldsize-popped.size());
}

TEST(STLScheduler, basic_ops){
        struct Entry{
                
                size_t m_time;
                void * m_placeholder;
                explicit constexpr Entry(size_t time, void* v=nullptr):m_time(time),m_placeholder(v){;}
                bool operator<(const Entry& rhs)const{return m_time > rhs.m_time;}
        };
        
        using n_model::ModelEntry;
        constexpr size_t limit = 100000;
        using n_network::t_timestamp;
        STLScheduler<Entry> msgqueue;
        for(size_t i = 0; i< limit; ++i){
                msgqueue.push_back(Entry(limit-1-i));
        }
        for(size_t i = 0; i< limit; ++i){
                EXPECT_EQ(msgqueue.pop().m_time, i);
        }
}


struct modelstub{
        n_network::t_timestamp time;
        size_t  id;
        modelstub(n_network::t_timestamp t, size_t d):time(t),id(d){;}
        n_network::t_timestamp getTimeNext()const{return time;}
        size_t getLocalID()const{return id;}
};


/**
 * Prototype for a scheduler that never releases items, and 
 * fits model scheduling better than the current as a test before implementing the new one.
 */
template<typename Item>
struct mockscheduler{
        std::deque<Item> m_storage;
        void
        init(const std::deque<Item>& m_models){
                m_storage=m_models;
        }
        
        void
        pop_until(const Item& mark, std::vector<size_t>& ids){
            std::sort(m_storage.rbegin(), m_storage.rend());
            for(auto iter = m_storage.begin(); iter != m_storage.end(); ++iter){
                    if(*iter < mark)
                            break;
                    ids.emplace_back(size_t (*iter));
            }
        }
};


TEST(IntrusiveScheduler, basic_ops){
        using n_network::t_timestamp;
        constexpr size_t limit = 1000;
        using n_model::IntrusiveEntry;
        using n_model::ModelEntry;
        std::deque<modelstub*> models;
        std::deque<IntrusiveEntry<modelstub>> entries;
        for(size_t i = 0; i<limit; ++i){
                modelstub* ptr= new modelstub(t_timestamp(i, i), i);
                models.push_back(ptr);
                entries.push_back(IntrusiveEntry<modelstub>(ptr));
        }
        mockscheduler<IntrusiveEntry<modelstub>> sched;
        sched.init(entries);
        std::vector<size_t> ids;
        modelstub m(t_timestamp(limit+1), limit+1);
        IntrusiveEntry<modelstub> ie(&m);
        sched.pop_until(ie, ids);
        EXPECT_EQ(ids.size(), limit);
        for(auto mptr: models)
                delete mptr;
}