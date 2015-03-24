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
#include <chrono>

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
		t_timestamp lhs = t_timestamp(i, 0);
		t_timestamp rhs = t_timestamp(i, 1);
		t_timestamp equalleft = t_timestamp(i, 0);
		auto hashleft = std::hash<t_timestamp>()(lhs);
		auto hashright = std::hash<t_timestamp>()(rhs);
		auto hashequal = std::hash<t_timestamp>()(equalleft);
		EXPECT_TRUE(lhs < rhs);
		EXPECT_FALSE(lhs >= rhs);
		EXPECT_FALSE(lhs == rhs);
		EXPECT_TRUE(lhs != rhs);
		EXPECT_FALSE(hashleft == hashright);
		EXPECT_TRUE(lhs == equalleft);
		EXPECT_TRUE(hashleft == hashequal);
	}
	std::unordered_set<t_timestamp> father_time;
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

	t_timestamp lesstime = Time<size_t, size_t>(2, 0);
	t_timestamp moretime = Time<size_t, size_t>(3, 0);
	EXPECT_TRUE(lesstime < moretime);
	EXPECT_TRUE(lesstime <= moretime);
	EXPECT_TRUE(moretime > lesstime);
	EXPECT_TRUE(moretime >= lesstime);
	EXPECT_TRUE(lesstime != moretime);
	EXPECT_FALSE(lesstime == moretime);
}

void push(size_t pushcount, size_t coreid, n_network::Network& net, size_t cores)
{
	for (size_t i = 0; i < pushcount; ++i) {
		for (size_t j = 0; j < cores; ++j) {
			if (j == coreid)
				continue;
			t_msgptr msg = n_tools::createObject<Message>("Q", t_timestamp(i, 0));
			EXPECT_TRUE(msg->getDestinationCore() != j);
			msg->setDestinationCore(j);
			EXPECT_TRUE(msg->getDestinationCore() == j);
			net.acceptMessage(msg);
		}
	}
}

void pull(size_t pushcount, size_t coreid, n_network::Network& net, size_t cores)
{
	std::vector<t_msgptr> received;
	while (received.size() != pushcount * (cores - 1)) {
		if (net.havePendingMessages(coreid)) {
			auto messages = net.getMessages(coreid);
			received.insert(received.begin(), messages.begin(), messages.end());
		}
	}
}

TEST(Network, threadsafety)
{
	constexpr size_t cores = 4;
	constexpr size_t msgcount = 10000;
	n_network::Network n(cores);
	std::vector<std::thread> workers;
	for (size_t i = 0; i < cores; ++i) {
		workers.push_back(std::thread(push, msgcount, i, std::ref(n), cores));
		workers.push_back(std::thread(pull, msgcount, i, std::ref(n), cores));
	}
	for (auto& t : workers) {
		t.join();
	}
}

void benchNetworkSpeed()
{
	// Each thread pushes msgcount * cores-1 messages, pulls msgcount * cores-1messages.
	constexpr size_t cores = 4;
	constexpr size_t msgcount = 200;
	n_network::Network n(cores);
	std::vector<std::thread> workers;
	std::chrono::time_point<std::chrono::system_clock> start, end;
	start = std::chrono::system_clock::now();
	for (size_t i = 0; i < cores; ++i) {
		workers.push_back(std::thread(push, msgcount, i, std::ref(n), cores));
		workers.push_back(std::thread(pull, msgcount, i, std::ref(n), cores));
	}
	for (auto& t : workers) {
		t.join();
	}
	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end-start;
	//std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	std::size_t totalcount = (msgcount * (cores-1))*cores;
//	std::cout << "Sending / Receiving of  " << totalcount << " messages finished at\t" << std::ctime(&end_time)
	//<< "elapsed time: " << elapsed_seconds.count() << "s\n";

	LOG_INFO("NETWORK: Network element with up to ", cores, " queues simulated with  ", cores*2, " threads.");
	LOG_INFO("NETWORK: Logging == ", LOGGING);
	LOG_INFO("NETWORK: Processing speed: ", totalcount/ (elapsed_seconds.count()), "msg / s");
}

TEST(Network, speed)
{
	benchNetworkSpeed();
}
