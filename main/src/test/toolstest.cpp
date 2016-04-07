/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve, Tim Tuijn
 */

#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <list>
#include <array>
#include <queue>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include "boost/pool/object_pool.hpp"
#include "boost/pool/singleton_pool.hpp"
#include "scheduler/heapscheduler.h"
#include "scheduler/schedulerfactory.h"
#include "scheduler/listscheduler.h"
#include "scheduler/stlscheduler.h"
#include "tools/globallog.h"
#include "tools/coutredirect.h"
#include "tools/sharedvector.h"
#include "tools/gviz.h"
#include "tools/flags.h"
#include "tools/misc.h"
#include "model/modelentry.h"
#include "network/messageentry.h"
#include "network/message.h"
#include "pools/pools.h"
#include "pools/cfpool.h"
#include "model/laentry.h"
#include "tools/frandom.h"

using std::cout;
using std::endl;
using std::atomic;
using std::thread;
using namespace n_tools;
using namespace n_scheduler;
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
	const size_t num_threads = std::thread::hardware_concurrency() > 8u ? 8u : std::thread::hardware_concurrency();
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

TEST(Vizwriter, creation){
        std::vector<GVizWriter*> ptrs(std::thread::hardware_concurrency(), nullptr);
        auto tfun = [&ptrs](size_t tid)->void{
                n_tools::GVizWriter* writer = GVizWriter::getWriter("a test");
                EXPECT_FALSE(writer==nullptr);
                ptrs[tid]=writer;
        };
        std::vector<std::thread> threads;
        for(size_t i = 0; i<std::thread::hardware_concurrency(); ++i){
                threads.push_back(std::thread(tfun, i));
        }
        for(auto& t : threads)
                t.join();
        // Let main ask a copy of the ptr.
        n_tools::GVizWriter* writer = GVizWriter::getWriter("a test");
        for(auto ptr : ptrs)
                EXPECT_EQ(ptr, writer);
        delete writer;
}

struct HeapTestComparator
{
	bool operator()(int a, int b){
		//we want a min heap, so we must test whether a > b and not a < b
		return (a > b);
	}
} heaptestcomparator;

TEST(HeapTest, heap_operations){
	std::vector<int> vec(20);
	std::iota(vec.begin(), vec.end(), 0);
#define HEAP_TEST_ISHEAP std::is_heap(vec.begin(), vec.end(), heaptestcomparator)
#define HEAP_TEST_UPDATE(x) n_tools::fix_heap(vec.begin(), vec.end(), x, heaptestcomparator)
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0] = -1;
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin());
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0] = 2;
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin());
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[4] = -1;
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+4);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[3] = 17;
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+3);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[9] = 20;
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+9);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[12] = 18;
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+12);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	std::vector<int> result = {-1, 1, 2, 7, 2, 5, 6, 15, 8, 19, 10, 11, 18, 13, 14, 17, 16, 17, 18, 20};
	EXPECT_EQ(vec.size(), result.size());
	for(std::size_t i = 0; i < vec.size(); ++i){
		EXPECT_EQ(vec[i], result[i]);
	}
#undef HEAP_TEST_ISHEAP
#undef HEAP_TEST_UPDATE
}

struct HeapTestUpdateVal
{
	std::size_t m_index;
	int m_value;
	constexpr HeapTestUpdateVal(int value = 0, std::size_t index = 0):
		m_index(index), m_value(value)
	{}

	HeapTestUpdateVal& operator++()
	{
	  ++m_value;
	  return *this;
	}
};

struct HeapTestComparator2
{
	bool operator()(const HeapTestUpdateVal& a, const HeapTestUpdateVal& b){
		//we want a min heap, so we must test whether a > b and not a < b
		return (a.m_value > b.m_value);
	}
} heaptestcomparator2;
struct HeapTestUpdator
{
	void operator()(HeapTestUpdateVal& item, std::size_t distance){
		//we want a min heap, so we must test whether a > b and not a < b
		LOG_DEBUG("updating value ", item.m_value, ',', item.m_index, " to index ", distance);
		item.m_index = distance;
	}
} heaptestupdator;

TEST(HeapTest, heap_operations_update){
	std::vector<HeapTestUpdateVal> vec(20);
	std::iota(vec.begin(), vec.end(), 0);
#define HEAP_TEST_ISHEAP std::is_heap(vec.begin(), vec.end(), heaptestcomparator2)
#define HEAP_TEST_UPDATE(x) n_tools::fix_heap(vec.begin(), vec.end(), x, heaptestcomparator2, heaptestupdator)
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0] = -1;
	LOG_DEBUG("vec[0] = -1");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin());
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0] = 2;
	LOG_DEBUG("vec[0] = 2");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin());
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[4] = -1;
	LOG_DEBUG("vec[4] = -1");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+4);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[3] = 17;
	LOG_DEBUG("vec[3] = 17");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+3);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[9] = 20;
	LOG_DEBUG("vec[9] = 20");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(vec.begin()+9);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[12] = 18;
	LOG_DEBUG("vec[12] = 18");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	LOG_DEBUG("vec[12] = 18, check before update");
	HEAP_TEST_UPDATE(vec.begin()+12);
	LOG_DEBUG("vec[12] = 18, after update");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	LOG_DEBUG("vec[12] = 18, after final check");
	std::vector<int> result = {-1, 1, 2, 7, 2, 5, 6, 15, 8, 19, 10, 11, 18, 13, 14, 17, 16, 17, 18, 20};
	std::vector<std::size_t> indexresult = {0, 1, 0, 3, 4, 0, 0, 7, 0, 9, 0, 0, 0, 0, 0, 15, 0, 0, 0, 19};
	EXPECT_EQ(vec.size(), result.size());
	EXPECT_EQ(vec.size(), indexresult.size());
	for(std::size_t i = 0; i < vec.size(); ++i){
		LOG_DEBUG("Checking item ", i);
		EXPECT_EQ(vec[i].m_value, result[i]);
		EXPECT_EQ(vec[i].m_index, indexresult[i]);
	}
	LOG_DEBUG("Done");
#undef HEAP_TEST_ISHEAP
#undef HEAP_TEST_UPDATE
}

struct HeapSchedulerVal
{
	const int m_startvalue;
	int m_value;
	constexpr HeapSchedulerVal(int value = 0):
		m_startvalue(value), m_value(value)
	{}
};

struct HeapSchedulerComparator
{
	bool operator()(HeapSchedulerVal* a, HeapSchedulerVal* b) const{
		//we want a min heap, so we must test whether a > b and not a < b
		return (a->m_value > b->m_value);
	}
};

TEST(HeapTest, heap_scheduler){
	n_scheduler::HeapScheduler<HeapSchedulerVal, HeapSchedulerComparator> vec(5);
	for(std::size_t i = 0; i < 20; ++i){
		vec.push_back(new HeapSchedulerVal(i));
		EXPECT_EQ(vec.dirty(), i >= 5);
	}
#define HEAP_TEST_ISHEAP vec.isHeap()
#define HEAP_TEST_UPDATE(x) vec.update(x)
	EXPECT_TRUE(vec.dirty());
	vec.updateAll();
	EXPECT_FALSE(vec.dirty());
	EXPECT_EQ(vec.size(), 20u);
	for(std::size_t i = 0; i < vec.size(); ++i){
		LOG_DEBUG("testing item ", i);
		EXPECT_EQ(vec.heapAt(i)->m_value, int(i));
		EXPECT_EQ(vec.heapAt(i)->m_startvalue, int(i));
	}
	LOG_DEBUG("prior to testing for the first time.");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0]->m_value = -1;
	LOG_DEBUG("vec[0] = -1");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(0);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[0]->m_value = 2;
	LOG_DEBUG("vec[0] = 2");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(0);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[4]->m_value = -1;
	LOG_DEBUG("vec[4] = -1");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(4);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[3]->m_value = 17;
	LOG_DEBUG("vec[3] = 17");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(3);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[9]->m_value = 20;
	LOG_DEBUG("vec[9] = 20");
	EXPECT_FALSE(HEAP_TEST_ISHEAP);
	HEAP_TEST_UPDATE(9);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	vec[12]->m_value = 18;
	LOG_DEBUG("vec[12] = 18, ", vec[12].m_index);
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	LOG_DEBUG("vec[12] = 18, check before update");
	HEAP_TEST_UPDATE(12);
	LOG_DEBUG("vec[12] = 18, after update");
	EXPECT_TRUE(HEAP_TEST_ISHEAP);
	LOG_DEBUG("vec[12] = 18, after final check");
	std::vector<int> result = {-1, 1, 2, 7, 2, 5, 6, 15, 8, 19, 10, 11, 18, 13, 14, 17, 16, 17, 18, 20};
	std::vector<int> indexresult = {4, 1, 2, 7, 0, 5, 6, 15, 8, 19, 10, 11, 12, 13, 14, 3, 16, 17, 18, 9};
	EXPECT_EQ(vec.size(), result.size());
	for(std::size_t i = 0; i < vec.size(); ++i){
		EXPECT_EQ(vec.heapAt(i)->m_value, result[i]);
		EXPECT_EQ(vec.heapAt(i)->m_startvalue, indexresult[i]);
	}
	for(std::size_t i = 0; i < vec.size(); ++i)
		delete vec[i];
#undef HEAP_TEST_ISHEAP
#undef HEAP_TEST_UPDATE
}

TEST(NumericTest, sgnFunc){
#define DOTEST(suffix) \
	EXPECT_EQ(1, n_tools::sgn(1##suffix)); \
	EXPECT_EQ(1, n_tools::sgn(2##suffix)); \
	EXPECT_EQ(1, n_tools::sgn(10##suffix)); \
	EXPECT_EQ(-1, n_tools::sgn(-1##suffix)); \
	EXPECT_EQ(-1, n_tools::sgn(-2##suffix)); \
	EXPECT_EQ(-1, n_tools::sgn(-10##suffix)); \
	EXPECT_EQ(0, n_tools::sgn(0##suffix))

	DOTEST(0);	//integer
	DOTEST(.0);	//double
	DOTEST(.0f);	//float
	DOTEST(l);	//long int
	DOTEST(ll);	//long long int
#undef DOTEST
}

TEST(NumericTest, log2Func){
	EXPECT_EQ(0, n_tools::intlog2(1));
	EXPECT_EQ(1, n_tools::intlog2(2));
	EXPECT_EQ(1, n_tools::intlog2(3));
	EXPECT_EQ(2, n_tools::intlog2(4));
	EXPECT_EQ(2, n_tools::intlog2(5));
	EXPECT_EQ(2, n_tools::intlog2(6));
#define DOTESTPART(k, type) \
	EXPECT_EQ(k-1, n_tools::intlog2((type)((1 << k) -1))); \
	EXPECT_EQ(k, n_tools::intlog2((type)(1 << k))); \
	EXPECT_EQ(k, n_tools::intlog2((type)((1 << k) + 1)));
#define DOTEST(type) \
	DOTESTPART(3, type)\
	DOTESTPART(4, type)\
	DOTESTPART(5, type)\
	DOTESTPART(6, type)\
	DOTESTPART(7, type)\
	DOTESTPART(8, type)\
	DOTESTPART(9, type)\
	DOTESTPART(10, type)\
	DOTESTPART(11, type)\
	DOTESTPART(12, type)\
	DOTESTPART(13, type)\
	DOTESTPART(14, type)\
	DOTESTPART(15, type)\
	DOTESTPART(16, type)\
	DOTESTPART(17, type)\
	DOTESTPART(18, type)\
	DOTESTPART(19, type)\
	DOTESTPART(20, type)\
	DOTESTPART(21, type)

	DOTEST(int)			//uses default one
	DOTEST(unsigned int)
	DOTEST(unsigned long)
	DOTEST(unsigned long long)

#undef DOTEST
#undef DOTESTPART
}

TEST(Pool, MessageBasics){
        using n_network::Message;
        using n_model::uuid;
        using n_network::t_timestamp;
        boost::object_pool<Message> pl;
        Message* rawmem = pl.malloc();
        Message * msgconstructed = new (rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
        EXPECT_EQ(msgconstructed->getSourceCore(), 1u);
        pl.free(rawmem);
}




#define testsize 40000 //inconnect fta w200 , represent 4e4 * 64byte = ~2.4 MB
#define rounds 10 // -t rounds*100


void timePool(n_pools::PoolInterface<n_network::Message>* pl){
        using n_network::Message;
        using n_model::uuid;
        using n_network::t_timestamp;
        std::vector<Message*> mptrs(testsize);
        for(size_t j = 0; j<rounds;++j){
                for(size_t i = 0; i<testsize; ++i){
                        Message* rmem = pl->allocate();
                        Message * msgconstructed = new (rmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        EXPECT_EQ(msgconstructed->getSourceCore(), 1u);
                        msgconstructed->setAntiMessage(true);
                        mptrs[i]=msgconstructed;
                }
                
                for(auto p : mptrs){
                        pl->deallocate(p);
                }
                
        }
}

TEST(New, Timing)
{
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::Pool<n_network::Message, std::false_type>(testsize);
        timePool(pl);
        delete pl;
}


TEST(BoostPool, Timing){
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::Pool<n_network::Message, boost::pool<>>(testsize);
        timePool(pl);
        delete pl;
}

TEST(SlabPool, Timing){
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::SlabPool<n_network::Message>(testsize);
        timePool(pl);
        delete pl;
}

TEST(StackPool, Timing){
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::StackPool<n_network::Message>(testsize);
        timePool(pl);
        delete pl;
}

TEST(DSlabPool, Timing){
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::DynamicSlabPool<n_network::Message>(testsize);
        timePool(pl);
        delete pl;
}

TEST(CFPool, Timing){
        n_pools::PoolInterface<n_network::Message>* pl= new n_pools::CFPool<n_network::Message>(testsize);
        timePool(pl);
        delete pl;
}

TEST(Pool, DynamicSlabPool){
        using n_network::Message;
        using n_network::SpecializedMessage;
        using n_model::uuid;
        using n_network::t_timestamp;
        size_t psize=2;
        size_t tsize=11;        // trigger at least 2 resize operations.
        size_t rsize=2;         
        n_pools::DynamicSlabPool<SpecializedMessage<std::string>> pl(psize);
        std::vector<SpecializedMessage<std::string>*> mptrs(tsize);
        for(size_t j = 0; j<rsize;++j){
                for(size_t i = 0; i<tsize; ++i){
                        Message* rawmem = pl.allocate();
                        SpecializedMessage<std::string>* msgconstructed = new(rawmem) SpecializedMessage<std::string>( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6, "abc");
                        EXPECT_EQ(msgconstructed->getSourceCore(), 1u);
                        mptrs[i]=msgconstructed;
                }
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
        }
        EXPECT_EQ(pl.size(), 1ull << (size_t((log2(tsize)+1))));
        EXPECT_EQ(pl.allocated(), 0u);
}

TEST(Pool, StackPool){
        using n_network::Message;
        using n_model::uuid;
        using n_network::t_timestamp;
        size_t psize=4;
        size_t tsize=9;
        size_t rsize=3;
        n_pools::StackPool<Message> pl(psize);
        std::vector<Message*> mptrs(tsize);
        for(size_t j = 0; j<rsize;++j){
                for(size_t i = 0; i<tsize; ++i){
                        Message* rawmem = pl.allocate();
                        Message * msgconstructed = new(rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        EXPECT_EQ(msgconstructed->getSourceCore(), 1u);
                        mptrs[i]=msgconstructed;
                }
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
        }
        EXPECT_EQ(pl.size(), 1ull << (size_t((log2(tsize)+1))));
        EXPECT_EQ(pl.allocated(), 0u);
}

// Compilation fails if struct is defined inside a testcase.
struct mystr{
                int i; 
                double j;
                constexpr explicit mystr(int pi, double pj):i(pi), j(pj){;}
        };

TEST(Factory, PoolCalls){
        mystr * ptrtostr = n_tools::createPooledObject<mystr>(1, 31.4);
        EXPECT_EQ(ptrtostr->i, 1);
        n_tools::destroyPooledObject<mystr>(ptrtostr);
}

TEST(Bits, FBS)
{
        size_t i = 1;
        size_t j = 1;
        size_t rng = sizeof(i)*8;
        while(i){
                EXPECT_EQ(n_tools::firstbitset<sizeof(i)>(i), rng-j);
                i = i<<1;
                ++j;
        }
                
}

TEST(Threading, DetectMainNoSyscall)
{
        n_pools::setMain();
        std::vector<std::thread> ts;
        for(size_t i = 0; i < 4; ++i)
        {
                ts.push_back(std::thread(
                                [&]()->void{
                                        EXPECT_FALSE(n_pools::isMain());
                                        }
                                        )
                        );
        }
        for(auto& t : ts)
                t.join();
        EXPECT_TRUE(n_pools::isMain());
}

TEST(Pool, CFPool){
        using n_network::Message;
        using n_model::uuid;
        using n_network::t_timestamp;
        std::set<Message*> ptrs;
        size_t psize=2;
        size_t tsize=129;
        size_t rsize=2;
        n_pools::CFPool<Message> pl(psize); 
        std::vector<Message*> mptrs;
        for(size_t j = 0; j<rsize;++j){
                for(size_t i = 0; i<tsize; ++i){
                        Message* rawmem = pl.allocate();
                        Message * msgconstructed = new(rawmem) Message( uuid(1,1), uuid(2,2), t_timestamp(3,4), 5, 6);
                        EXPECT_EQ(msgconstructed->getSourceCore(), 1u);
                        mptrs.push_back(msgconstructed);
                        if (!ptrs.insert(msgconstructed).second){
                                std::cerr << "Double ptr " << msgconstructed << std::endl;
                                throw 42;
                        }
                }
                for(auto p : mptrs){
                        pl.deallocate(p);
                }
                ptrs.clear();
                mptrs.clear();
        }
        pl.log_pool();
        EXPECT_EQ(pl.size(), 1ull << (size_t((log2(tsize)+1))));
        EXPECT_EQ(pl.allocated(), 0u);
}

TEST(Tools, p2){
        size_t val = 1;
        for(size_t i = 0; i<63; ++i){
                val = val << 1ull;
                EXPECT_TRUE(n_tools::is_power_2(val));
                if(val!=2){
                        EXPECT_FALSE(n_tools::is_power_2(val-1));
                }
        }
}
struct __attribute__((aligned(64))) teststruct{
        size_t val;
        char c;
        size_t v2;
        virtual void f(){;}
        virtual ~teststruct(){;}
};

// If the alignment is incorrect, the vtable will have an invalid offset which
// asan will detect (usually slightly before nasal demons arrive).
struct __attribute__((aligned(64))) derivedts : public teststruct{
        size_t pval;
        virtual void f()override{;}
};

TEST(Tools, alignptr)
{
        derivedts* ts = (derivedts*)std::malloc(3*sizeof(derivedts));        // Alignment should be ~8 ymmv
        derivedts* alignedts = n_pools::align_ptr(ts);
        alignedts = new (alignedts) derivedts;
        // A few accesses to check alignment asan warnings.
        alignedts->val = 42;
        alignedts->c = 'a';
        alignedts->v2 = 41;
        teststruct* bts = (teststruct*) alignedts;
        bts->f();
        uintptr_t low = (uintptr_t)alignedts;
        ++alignedts;
        uintptr_t high = (uintptr_t)alignedts;
        EXPECT_EQ(high-low, 64u);       // ++ should have move 64.
        EXPECT_EQ(low%64, 0u);          // original address should be 0 mod alignment.
        std::free(ts);
}

TEST(LA, Vsched)
{
        constexpr size_t tsize = 10;
        using n_model::LaEntry;
        using n_network::t_timestamp;
        typedef boost::heap::fibonacci_heap<LaEntry> heapchoice;
        VectorScheduler<heapchoice, LaEntry> vscheduler;
        for(size_t i = 0; i<tsize; ++i){
                LaEntry la(i, t_timestamp(i,i));
                vscheduler.push_back(la);
                EXPECT_TRUE(vscheduler.contains(la));
        }
        LaEntry first = vscheduler.top();
        EXPECT_EQ(first.getTime(), t_timestamp(0,0));
        LaEntry mod_first(first.getID(), t_timestamp(42,42));
        EXPECT_TRUE(vscheduler.contains(mod_first));
        vscheduler.update(mod_first);
        EXPECT_EQ(vscheduler.top().getTime(), t_timestamp(1,1));
}

TEST(RNG, Iface){
    n_tools::n_frandom::t_fastrng str2;
    std::uniform_int_distribution<> uid(1,10);
    str2.seed(0);
    auto answer = uid(str2);
    str2.seed(0);
    auto question = uid(str2);
    EXPECT_EQ(answer, question);
}

TEST(RNG, Rand){
    n_tools::n_frandom::t_fastrng str2;
    std::uniform_int_distribution<> uid(1,10);
    str2.seed(0);
    auto answer = uid(str2);
    str2.seed(0);
    auto question = uid(str2);
    EXPECT_EQ(answer, question);
    size_t r = 0;
    for(int i = 0; i<1000; ++i){
        r = str2.operator ()();
        EXPECT_TRUE(r != 0);    // x64 shifter cannot ever produce a zero, if it does the sequence will converge to zero.
    }
}

TEST(RNG, Bench){
    // This test can fail, we're looking at threadsafety more than anything else here, -fthreadsanitize should pick
    // up a race on 2e10 calls. 
    n_tools::n_frandom::benchrngs();
}
