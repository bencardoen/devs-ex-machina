/*
 * networktest.cpp
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#include <gtest/gtest.h>
#include "timestamp.h"
#include "network.h"
#include "objectfactory.h"
#include <unordered_set>
#include <thread>
#include <vector>

using namespace n_network;

TEST(Time, FactoryFunctions)
{
	auto first = makeTimeStamp();
	auto second = makeTimeStamp();
	auto aftersecond = makeCausalTimeStamp(second);
	auto third = first;
	EXPECT_TRUE(first.getTime() == third.getTime());
	EXPECT_TRUE(second.getCausality() < aftersecond.getCausality());
}

TEST(Time, HashingOperators)
{
	const size_t TESTSIZE = 1000;
	for (size_t i = 0; i < TESTSIZE; ++i) {
		TimeStamp lhs = TimeStamp(i, 0);
		TimeStamp rhs = TimeStamp(i, 1);
		TimeStamp equalleft = TimeStamp(i, 0);
		auto hashleft = std::hash<TimeStamp>()(lhs);
		auto hashright = std::hash<TimeStamp>()(rhs);
		auto hashequal = std::hash<TimeStamp>()(equalleft);
		EXPECT_TRUE(lhs < rhs);
		EXPECT_FALSE(lhs >= rhs);
		EXPECT_FALSE(lhs == rhs);
		EXPECT_TRUE(lhs != rhs);
		EXPECT_FALSE(hashleft == hashright);
		EXPECT_TRUE(lhs == equalleft);
		EXPECT_TRUE(hashleft == hashequal);
	}
	std::unordered_set<TimeStamp> father_time;
	for (size_t i = 0; i < TESTSIZE; ++i) {
		father_time.emplace(i, 0);
		father_time.emplace(i, 1);
	}
	EXPECT_TRUE(father_time.size() == 2 * TESTSIZE);
	father_time.clear();
	double a = 0;
	double b = a + std::numeric_limits<double>::epsilon() * 100;
	bool x = nearly_equal<double>(a, b);
	EXPECT_TRUE(x);

	TimeStamp lesstime = Time<size_t, size_t>(2, 0);
	TimeStamp moretime = Time<size_t, size_t>(3, 0);
	EXPECT_TRUE(lesstime < moretime);
	EXPECT_TRUE(lesstime <= moretime);
	EXPECT_TRUE(moretime > lesstime);
	EXPECT_TRUE(moretime >= lesstime);
	EXPECT_TRUE(lesstime != moretime);
	EXPECT_FALSE(lesstime == moretime);
}

void push(size_t pushcount,size_t coreid, n_network::Network<2>& net, size_t cores)
{
	for(size_t i =0; i<pushcount; ++i){
		for(size_t j = 0; j<cores; ++j){
			if(j==coreid)
				continue;
			t_msgptr msg= n_tools::createObject<Message>("", j);
			net.acceptMessage(msg);
		}
	}
}

void pull(size_t pushcount,size_t coreid, n_network::Network<2>& net, size_t cores){
	std::vector<t_msgptr> received;
	while(received.size() != pushcount*(cores-1)){
		auto messages = net.getMessages(coreid);
		received.insert(received.begin(), messages.begin(), messages.end());
	}
}

TEST(Network, driver)
{
	constexpr size_t cores = 2;
	n_network::Network<cores> n;
	std::vector<std::thread> workers;
	for(size_t i = 0; i<cores; ++i){
		workers.push_back(std::thread(push, 10, i, std::ref(n), cores));
		workers.push_back(std::thread(pull, 10, i, std::ref(n), cores));
	}
	for(auto& t : workers){
		t.join();
	}
}
