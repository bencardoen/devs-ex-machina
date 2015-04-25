/*
 * tracerstest.cpp
 *
 *  Created on: Jan 7, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "gtest/gtest.h"
#include <sstream>
#include <vector>
#include "tracers.h"
#include "tracemessage.h"
#include "policies.h"
#include "verbosetracer.h"
#include "compare.h"
#include "macros.h"
#include "coutredirect.h"


using namespace n_tracers;

///**
// * @brief adapter class for the scheduler interface.
// *
// * I noticed that Ben's scheduler prototype doesn't adhere to the std container interface.
// * I assume this may get fixed, but otherwise, this is a quick and dirty way to allow my
// * prototype to use Ben's implementation and allow the use of std containers for testing purposes.
// * However, this is something we have to talk about on the next meeting.
// * @note `std::queue` and `std::stack` use `push()` instead of `push_back()`
// */
//template<class Container>
//class ContainerAdapter: public Container{
//public:
//	void push(const typename Container::value_type& value){
//		Container::push_back(value);
//	}
//};

//used for testing whether the returned reference of getTracer works
struct testVal {
		testVal(unsigned int i = 0) :
				i(i) {
		}
		unsigned int i;
};

//the move constructor is deleted so the copy constructor is used.
struct copyTest {
		copyTest(char d = 'a') :
				e(d) {
		}
		copyTest(const copyTest& other) :
				e(other.e) {
		}
		//copyTest(copyTest&& other) = delete;
		char e;
};

//the move constructor is deleted so the copy constructor is used.
struct copyMoveTest {
		copyMoveTest(int d = 42) :
				f(d) {
		}
		copyMoveTest(const copyMoveTest& other) :
				f(other.f + 1) {
		}
		copyMoveTest(copyMoveTest&& other) :
				f(other.f - 1) {
		}
		;	//implicit
		int f;
};

template<typename ... args> void testSize() {
	EXPECT_EQ(Tracers<args...>().getSize(), sizeof...(args));
	EXPECT_EQ(Tracers<args...>().hasTracers(), bool(sizeof...(args)));
}
TEST(tracing, getSizeHasTracers) {
	testSize<>();
	testSize<int>();	//as long as we don't access any tracing methods, these work just fine
	testSize<int, double>();
	testSize<int, double, char>();
	//tests what happens when you slice the object
	EXPECT_EQ(((Tracers<double, char> )Tracers<int, double, char>()).getSize(), 2);
	EXPECT_EQ(((Tracers<char> )Tracers<int, double, char>()).getSize(), 1);
	EXPECT_EQ(((Tracers<> )Tracers<int, double, char>()).getSize(), 0);
	EXPECT_EQ(((Tracers<double, char> )Tracers<int, double, char>()).hasTracers(), true);
	EXPECT_EQ(((Tracers<char> )Tracers<int, double, char>()).hasTracers(), true);
	EXPECT_EQ(((Tracers<> )Tracers<int, double, char>()).hasTracers(), false);
}

TEST(tracing, tracerCopyMove) {
	Tracers<copyTest> test2(copyTest('y'));
	EXPECT_EQ(test2.getByID<0>().e, 'y');
	Tracers<copyMoveTest> test3(copyMoveTest(10));
//	EXPECT_EQ(test3.getByID<0>().f, 9);	//value decremented if move constructor used
	EXPECT_EQ(test3.getByID<0>().f, 11);	//value incremented if copy constructor used
}

TEST(tracing, getByID) {
	//should not compile
//	Tracers<> emptyTester;
//	EXPECT_EQ(emptyTester.getByID<0>(), nullptr);

	auto tester = Tracers<char, int, double, float, testVal>();
	EXPECT_EQ(tester.getByID<0>(), '\0');
	EXPECT_EQ(tester.getByID<1>(), 0);
	EXPECT_EQ(tester.getByID<2>(), 0.0);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 0u);
	//the following line should result in a compiler error
	//auto val = tester.getTracer<5>();

	tester.getByID<2>() = 1.5;
	EXPECT_EQ(tester.getByID<0>(), '\0');
	EXPECT_EQ(tester.getByID<1>(), 0);
	EXPECT_EQ(tester.getByID<2>(), 1.5);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 0u);
	++tester.getByID<1>();
	EXPECT_EQ(tester.getByID<0>(), '\0');
	EXPECT_EQ(tester.getByID<1>(), 1);
	EXPECT_EQ(tester.getByID<2>(), 1.5);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 0u);
	tester.getTracer() = 'a';
	EXPECT_EQ(tester.getByID<0>(), 'a');
	EXPECT_EQ(tester.getByID<1>(), 1);
	EXPECT_EQ(tester.getByID<2>(), 1.5);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 0u);
	tester.getByID<4>().i = 42u;
	EXPECT_EQ(tester.getByID<0>(), 'a');
	EXPECT_EQ(tester.getByID<1>(), 1);
	EXPECT_EQ(tester.getByID<2>(), 1.5);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 42u);
	tester.getByID<4>() = testVal(3u);
	EXPECT_EQ(tester.getByID<0>(), 'a');
	EXPECT_EQ(tester.getByID<1>(), 1);
	EXPECT_EQ(tester.getByID<2>(), 1.5);
	EXPECT_EQ(tester.getByID<3>(), 0.0f);
	EXPECT_EQ(tester.getByID<4>().i, 3u);
}

struct TraceCall
{
	t_timestamp m_lastTime;
	unsigned int m_timesCalled;
	std::size_t m_lastID;
	TraceCall()
		: m_timesCalled(0u), m_lastID(-1u)
	{
	}

	void call(t_timestamp t, std::size_t coreid)
	{
		m_lastTime = t;
		++m_timesCalled;
		m_lastID = coreid;
	}
};

struct TraceCallTest
{
	bool m_stopped;
	TraceCall m_value[4];
	TraceCallTest(): m_stopped(false){

	}
	void stopTracer(){
		m_stopped = true;
	}
	void tracesInit(const t_atomicmodelptr&, t_timestamp time)
	{
		if(!m_stopped) m_value[0].call(time, 0u);
	}
	void tracesInternal(const t_atomicmodelptr&, std::size_t coreid)
	{
		if(!m_stopped) m_value[1].call(t_timestamp(), coreid);
	}
	void tracesExternal(const t_atomicmodelptr&, std::size_t coreid)
	{
		if(!m_stopped) m_value[2].call(t_timestamp(), coreid);
	}
	void tracesConfluent(const t_atomicmodelptr&, std::size_t coreid)
	{
		if(!m_stopped) m_value[3].call(t_timestamp(), coreid);
	}
};
#define TEST_DATA_PART(i, a0, a1, a2, b0, b1, b2, c0, c1, c2, d0, d1, d2) \
	EXPECT_EQ(tester.getByID<i>().m_value[0].m_timesCalled, a0);\
	EXPECT_EQ(tester.getByID<i>().m_value[0].m_lastTime, a1);\
	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastID, a2);\
	EXPECT_EQ(tester.getByID<i>().m_value[1].m_timesCalled, b0);\
	EXPECT_EQ(tester.getByID<i>().m_value[1].m_lastTime, b1);\
	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastID, b2);\
	EXPECT_EQ(tester.getByID<i>().m_value[2].m_timesCalled, c0);\
	EXPECT_EQ(tester.getByID<i>().m_value[2].m_lastTime, c1);\
	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastID, c2);\
	EXPECT_EQ(tester.getByID<i>().m_value[3].m_timesCalled, d0);\
	EXPECT_EQ(tester.getByID<i>().m_value[3].m_lastTime, d1);\
	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastID, d2);

#define TEST_DATA(a0, a1, a2, b0, b1, b2, c0, c1, c2, d0, d1, d2) \
	TEST_DATA_PART(0, a0, a1, a2, b0, b1, b2, c0, c1, c2, d0, d1, d2) \
	TEST_DATA_PART(1, a0, a1, a2, b0, b1, b2, c0, c1, c2, d0, d1, d2)

TEST(tracing, traceCall) {
	//using multiple copies to test whether they all get called
	auto tester = Tracers<TraceCallTest, TraceCallTest>();
	for (unsigned int i = 0u; i < 4u; ++i) {
		EXPECT_EQ(tester.getByID<0>().m_value[i].m_timesCalled, 0u);
		EXPECT_EQ(tester.getByID<0>().m_value[i].m_lastTime, t_timestamp());
		EXPECT_EQ(tester.getByID<1>().m_value[i].m_timesCalled, 0u);
		EXPECT_EQ(tester.getByID<1>().m_value[i].m_lastTime, t_timestamp());
	}
	t_atomicmodelptr ptr;
	tester.tracesInit(ptr, t_timestamp(1u, 0u));
	TEST_DATA(1u, t_timestamp(1u, 0u), 0u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u)

	tester.tracesInit(ptr, t_timestamp(0u, 1u));
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());

	tester.tracesInternal(ptr, 1u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  1u, t_timestamp(),  1u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());
	tester.tracesInternal(ptr, 0u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  0u, t_timestamp(), -1u,
		  0u, t_timestamp(), -1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());

	tester.tracesExternal(ptr, 1u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  1u, t_timestamp(), 1u,
		  0u, t_timestamp(), -1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());
	tester.tracesExternal(ptr, 0u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  2u, t_timestamp(), 0u,
		  0u, t_timestamp(), -1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 0u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());

	tester.tracesConfluent(ptr, 1u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  2u, t_timestamp(), 0u,
		  1u, t_timestamp(), 1u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 1u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());
	tester.tracesConfluent(ptr, 0u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  2u, t_timestamp(), 0u,
		  2u, t_timestamp(), 0u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());
	tester.stopTracers();
	tester.tracesInit(ptr, t_timestamp(2u, 1u));
	tester.tracesInternal(ptr, 0u);
	tester.tracesExternal(ptr, 0u);
	tester.tracesConfluent(ptr, 0u);
	TEST_DATA(2u, t_timestamp(0u, 1u), 0u,
		  2u, t_timestamp(),  0u,
		  2u, t_timestamp(), 0u,
		  2u, t_timestamp(), 0u)
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[0].m_lastTime, t_timestamp(0u, 1u));
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[1].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[2].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<0>().m_value[3].m_lastTime, t_timestamp());
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_timesCalled, 2u);
//	EXPECT_EQ(tester.getByID<1>().m_value[3].m_lastTime, t_timestamp());
}

#undef TEST_DATA
#undef TEST_DATA_PART

void testFunc(int* ref, int newVal) {
	*ref = newVal;
}
TEST(tracing, tracerMessage) {
	//test the testFunc function
	int intvar = 0;
	EXPECT_EQ(intvar, 0);
	testFunc(&intvar, 5);
	EXPECT_EQ(intvar, 5);
	TraceMessage::t_messagefunc boundFunc = std::bind(&testFunc, &intvar, 42);
	EXPECT_EQ(intvar, 5);
	boundFunc();
	EXPECT_EQ(intvar, 42);

	//test the message
	TraceMessage::t_messagefunc boundFunc2 = std::bind(&testFunc, &intvar, 18);
	TraceMessage msg = TraceMessage(t_timestamp(12u, 42u), boundFunc2, 0u);
	EXPECT_EQ(intvar, 42);
	msg.execute();
	EXPECT_EQ(intvar, 18);
	EXPECT_TRUE(TraceMessage(t_timestamp(12u, 41u), boundFunc2, 0u) < TraceMessage(t_timestamp(12u, 42u), boundFunc2, 0u));
	EXPECT_TRUE(TraceMessage(t_timestamp(12u, 42u), boundFunc2, 0u) > TraceMessage(t_timestamp(12u, 41u), boundFunc2, 0u));
	EXPECT_TRUE(TraceMessage(t_timestamp(11u, 42u), boundFunc2, 0u) < TraceMessage(t_timestamp(12u, 42u), boundFunc2, 0u));
	EXPECT_TRUE(TraceMessage(t_timestamp(12u, 42u), boundFunc2, 0u) > TraceMessage(t_timestamp(11u, 42u), boundFunc2, 0u));
}

template<class P>
class PolicyTester: public P {
	public:
		template<typename ... Args>
		void printTest(const Args&... args) {
			this->print(args...);
		}
};

#define TESTFOLDERTRACE TESTFOLDER"tracer/"

TEST(tracing, policies) {
	{
		PolicyTester<FileWriter> pFile;
		EXPECT_EQ(pFile.isInitialized(), false);
		EXPECT_NO_THROW(pFile.initialize(TESTFOLDERTRACE"filewrite_1.txt"));
		EXPECT_EQ(pFile.isInitialized(), true);
		pFile.printTest("This is an integer: ", 5, '\n');
		pFile.stopTracer();
		pFile.printTest("This is text!\n");
		pFile.startTracer(true);
		pFile.printTest("This is ", "MOAR", " text!");
		pFile.stopTracer();
		EXPECT_EQ(n_misc::filecmp(TESTFOLDERTRACE"filewrite_1.txt", TESTFOLDERTRACE"filewrite_corr_1.txt"), 0);
	}	//filewrite_1.txt is surely released here.
	//TODO test for files that cannot be opened for writing
	//test for reopening a finished file

	//make sure the buffer is flushed
	std::stringstream newCoutTarget;
	{
		n_tools::CoutRedirect redirect(newCoutTarget);

		PolicyTester<CoutWriter> pCout;
		pCout.printTest("This is an integer: ", 5, '\n');
		pCout.stopTracer();
		pCout.printTest("This is text!\n");
		pCout.startTracer(true);
		pCout.printTest("This is ", "MOAR", " text!");
		pCout.stopTracer();
	}
	EXPECT_EQ(newCoutTarget.str(), "This is an integer: 5\nThis is MOAR text!");

}

class TestState: public n_model::State {
	public:
		TestState(std::size_t time = 0, std::size_t causality = 0) :
				State("This is a test state") {
			m_timeLast = t_timestamp(time, causality);
		}

		virtual std::string toXML() {
			return toString();
		}
		;
		virtual std::string toJSON() {
			return toString();
		}
		;
		virtual std::string toCell() {
			return toString();
		}
		;
};
class TestModel: public n_model::AtomicModel {
	public:
		TestModel() :
				AtomicModel("TestModel", 1) {
			this->addInPort("portIn");
			this->addInPort("portIn2");
			this->addOutPort("MyVerySpecialOutput");
			this->setState(std::make_shared<TestState>());
		}

		virtual void extTransition(const std::vector<n_network::t_msgptr> &) {
		}
		;
		virtual void intTransition() {
		}
		;
//	virtual void confTransition(const std::vector<n_network::t_msgptr> & message);
		virtual std::vector<n_network::t_msgptr> output() const {
			return std::vector<n_network::t_msgptr>();
		}
		virtual t_timestamp timeAdvance() const {
			return t_timestamp(getState()->m_timeLast.getTime() + 1, 1);
		}
};
//very simple macro for testing a single tracer class
//call it like this:
//	SINGLETRACERTEST(TESTFOLDERTRACE, ClassYouWantToTest)
#define SINGLETRACERTEST(outputFolder, tracerclass)\
TEST(tracing, tracer##tracerclass){\
	{\
		tracerclass<FileWriter> tracer;\
		n_model::t_atomicmodelptr model = std::make_shared<TestModel>();\
		tracer.initialize(outputFolder STRINGIFY( tracerclass ) "_out.txt");\
		tracer.tracesInit(model, t_timestamp(12, 1));\
		tracer.tracesInternal(model, 0u);\
		model->setState(std::make_shared<TestState>(13));\
		model->setTime(13); \
		tracer.tracesExternal(model, 0u);\
		model->setState(std::make_shared<TestState>(14));\
		model->setTime(14); \
		tracer.tracesExternal(model, 0u);\
		model->setState(std::make_shared<TestState>(14, 1));\
		tracer.tracesConfluent(model, 0u);\
		model->setState(std::make_shared<TestState>(15));\
		model->setTime(15); \
		tracer.tracesInit(model, t_timestamp(16, 1));\
		model->setState(std::make_shared<TestState>(16, 2));\
		model->setTime(16); \
		tracer.tracesInternal(model, 0u);\
		model->setState(std::make_shared<TestState>(16, 3));\
		tracer.tracesExternal(model, 0u);\
		model->setState(std::make_shared<TestState>(17));\
		model->setTime(17); \
		tracer.tracesExternal(model, 0u);\
		model->setState(std::make_shared<TestState>(18));\
		model->setTime(18); \
		tracer.tracesConfluent(model, 0u);\
		n_tracers::traceUntil(t_timestamp(400, 0));\
		n_tracers::clearAll();\
		n_tracers::waitForTracer();\
	}\
	EXPECT_EQ(n_misc::filecmp(outputFolder STRINGIFY( tracerclass ) "_out.txt", outputFolder STRINGIFY(tracerclass) "_out.corr"), 0);\
}

SINGLETRACERTEST(TESTFOLDERTRACE, VerboseTracer)

TEST(tracing, messageManagement){
	{
		VerboseTracer<FileWriter> tracer;
		n_model::t_atomicmodelptr model = std::make_shared<TestModel>();
		tracer.initialize(TESTFOLDERTRACE "msgmngmnt.txt");
		for(std::size_t i = 0; i < 300; ++i){
			model->setTime(t_timestamp(i+1));
			if(!(i%3))
				tracer.tracesConfluent(model, i%4);
			else if(!((i+1)%3))
				tracer.tracesInternal(model, i%4);
			else if(!((i+2)%3))
				tracer.tracesExternal(model, i%4);
			if(!(i%11) && i){
				n_tracers::revertTo(t_timestamp(i-11u), (i%5u)? ((i%5)-1u) : std::numeric_limits<std::size_t>::max());
				LOG_DEBUG("TracerTest: revertTo trie ", i);
			}
		}
		n_tracers::traceUntil(t_timestamp::infinity());
		n_tracers::clearAll();
		n_tracers::waitForTracer();
	}
	EXPECT_EQ(n_misc::filecmp(TESTFOLDERTRACE "msgmngmnt.txt", TESTFOLDERTRACE "msgmngmnt.corr"), 0);\

}
/*
 TEST(tracing, verboseTracer){
 VerboseTracer<FileWriter> tracer;
 ContainerAdapter<std::vector<TraceMessage*>> messages;

 EXPECT_EQ(tracer.isInitialized(), false);
 EXPECT_NO_THROW(tracer.initialize("test/files/verboseOut1.txt"));
 EXPECT_EQ(tracer.isInitialized(), true);
 tracer.traceInternal(2u, &messages);
 EXPECT_EQ(messages.size(), 1u);
 EXPECT_EQ(messages[0]->getScheduledTime(), 2u);
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_0.txt"), 0);
 messages[0]->execute();
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_1.txt"), 0);
 tracer.traceInternal(4u, &messages);
 EXPECT_EQ(messages.size(), 2u);
 EXPECT_EQ(messages[0]->getScheduledTime(), 2u);
 EXPECT_EQ(messages[1]->getScheduledTime(), 4u);
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_1.txt"), 0);
 messages[1]->execute();
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_2.txt"), 0);

 //test appending
 VerboseTracer<FileWriter> tracer2;
 EXPECT_EQ(tracer2.isInitialized(), false);
 EXPECT_NO_THROW(tracer2.initialize("test/files/verboseOut1.txt", true));
 EXPECT_EQ(tracer2.isInitialized(), true);
 tracer2.traceInternal(42u, &messages);
 EXPECT_EQ(messages.size(), 3u);
 EXPECT_EQ(messages[0]->getScheduledTime(), 2u);
 EXPECT_EQ(messages[1]->getScheduledTime(), 4u);
 EXPECT_EQ(messages[2]->getScheduledTime(), 42u);
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_2.txt"), 0);
 messages[2]->execute();
 EXPECT_EQ(filecmp("test/files/verboseOut1.txt", "test/files/corr_verboseOut1_3.txt"), 0);

 //delete the created messages.
 //This is another thing we'll have to talk about: who and how do the messages gat deleted. `std::shared_ptr`?
 for(TraceMessage* m:messages)
 delete m;
 }
 */
