/*
 * tracerstest.cpp
 *
 *  Created on: Jan 7, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */


#include "gtest/gtest.h"
#include <sstream>
#include <vector>
#include "tracers/tracers.h"
//#include "../../src/tracing/tracemessage.h"
//#include "../../src/tracing/tracers/policies.h"
//#include "../../src/tracing/tracers/verbosetracer.h"

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
struct testVal{
	testVal(unsigned int i = 0):i(i){}
	unsigned int i;
};

//tests whether the constructor copies its arguments or not
//the copy constructor is deleted so the move constructor is used.
struct moveTest{
	moveTest(double d = 5.5):d(d){}
	moveTest(const moveTest& other) = delete;
	moveTest(moveTest&& other): d(std::move(other.d)) {}
	double d;
};

//the move constructor is deleted so the copy constructor is used.
struct copyTest{
	copyTest(char d = 'a'):e(d){}
	copyTest(const copyTest& other): e(other.e){}
	//copyTest(copyTest&& other) = delete;
	char e;
};

//the move constructor is deleted so the copy constructor is used.
struct copyMoveTest{
	copyMoveTest(int d = 42):f(d){}
	copyMoveTest(const copyMoveTest& other): f(other.f + 1){}
	copyMoveTest(copyMoveTest&& other): f(other.f - 1){};	//implicit
	int f;
};

template<typename... args> void testSize(){
	EXPECT_EQ(Tracers<args...>().getSize(), sizeof...(args));
}
TEST(tracing, templateSize_test){
	testSize<>();
	testSize<int>();	//as long as we don't access any tracing methods, these work just fine
	testSize<int, double>();
	testSize<int, double, char>();
}


TEST(tracing, templateCopyMove_test){
//	TracersTemplated<moveTest> test1(moveTest(1.1));
//	EXPECT_EQ(test1.getByID<0>().d, 1.1);
	Tracers<copyTest> test2(copyTest('y'));
	EXPECT_EQ(test2.getByID<0>().e, 'y');
	Tracers<copyMoveTest> test3(copyMoveTest(10));
//	EXPECT_EQ(test3.getByID<0>().f, 9);	//value decremented if move constructor used
	EXPECT_EQ(test3.getByID<0>().f, 11);	//value incremented if copy constructor used

//	int i = 20;
//	auto f = [i] () {return copyMoveTest(i);};
//	TracersTemplated<copyMoveTest> test4(f());
//	EXPECT_EQ(test4.getTracer<0>().f, 19);	//value subtracted with 1 if move constructor used

}

TEST(tracing, templateGet_test){
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
/*
struct TraceCallTest{
	TraceCallTest():timesCalled(0), lastTime(-1), value(0){}
	void traceInternal(Time time, int* newVal){
		value = *newVal;
		++timesCalled;
		lastTime = time;
	}
	unsigned int timesCalled;
	Time lastTime;
	int value;
};

TEST(tracing, templateTraceCall_test){
	int useVal = 5;
	//using multiple copies to test whether they all get called
	//Notice that I abused the template parameter for the scheduler
	auto tester = Tracers<TraceCallTest, TraceCallTest>();
	EXPECT_EQ(tester.getTracer<0>().timesCalled, 0u);
	EXPECT_EQ(tester.getTracer<1>().timesCalled, 0u);
	EXPECT_EQ(tester.getTracer<0>().value, 0);
	EXPECT_EQ(tester.getTracer<1>().value, 0);
	EXPECT_EQ(tester.getTracer<0>().lastTime, -1);
	EXPECT_EQ(tester.getTracer<1>().lastTime, -1);
	tester.traceInternal(10, &useVal);
	EXPECT_EQ(tester.getTracer<0>().timesCalled, 1u);
	EXPECT_EQ(tester.getTracer<1>().timesCalled, 1u);
	EXPECT_EQ(tester.getTracer<0>().value, useVal);
	EXPECT_EQ(tester.getTracer<1>().value, useVal);
	EXPECT_EQ(tester.getTracer<0>().lastTime, 10);
	EXPECT_EQ(tester.getTracer<1>().lastTime, 10);
	useVal = 42;
	tester.traceInternal(5, &useVal);
	EXPECT_EQ(tester.getTracer<0>().timesCalled, 2u);
	EXPECT_EQ(tester.getTracer<1>().timesCalled, 2u);
	EXPECT_EQ(tester.getTracer<0>().value, useVal);
	EXPECT_EQ(tester.getTracer<1>().value, useVal);
	EXPECT_EQ(tester.getTracer<0>().lastTime, 5);
	EXPECT_EQ(tester.getTracer<1>().lastTime, 5);
}


TEST(tracing, templateVerboseTracer_test){
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
