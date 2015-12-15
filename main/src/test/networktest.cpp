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
#include "network/timestamp.h"
#include "network/network.h"
#include "tools/objectfactory.h"
#include <unordered_set>
#include <thread>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <chrono>

using namespace n_network;

TEST(Network, detectIdle){
	n_network::Network n(4);
	EXPECT_EQ(n.empty(), true);
	t_msgptr msg = n_tools::createRawObject<Message>(n_model::uuid(0, 0), n_model::uuid(3, 0), t_timestamp(1, 0), 0, 0);
	n.acceptMessage(msg);
	EXPECT_EQ(n.empty(), false);
        delete msg;
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

	t_timestamp lesstime = t_timestamp(2, 0);
	t_timestamp moretime = t_timestamp(3, 0);
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
			t_msgptr msg = n_tools::createRawObject<Message>(n_model::uuid(0, 0), n_model::uuid(j, 0), t_timestamp(i, 0), 0, 0);
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
        for(auto msg : received)
                delete msg;
}

void pushVector(size_t pushcount, size_t vectorcount, size_t coreid, n_network::Network& net, size_t cores)
{
	std::vector<t_msgptr> msgs;
	msgs.reserve(vectorcount);
	for (size_t i = 0; i < pushcount; ++i) {
		for (size_t j = 0; j < cores; ++j) {
			if (j == coreid)
				continue;
			for(size_t k = 0; k < vectorcount; ++k) {
				t_msgptr msg = n_tools::createRawObject<Message>(n_model::uuid(0, 0), n_model::uuid(j, 0), t_timestamp(i, 0), 0, 0);
				EXPECT_TRUE(msg->getDestinationCore() == j);
				msgs.push_back(msg);
			}
			net.giveMessages(j, msgs);
			msgs.clear();
		}
	}
}

TEST(Network, threadsafety)
{
	const size_t cores = std::min(std::thread::hardware_concurrency(), 8u);
	if(cores <= 1){
		LOG_WARNING("No threads available for threaded test.");
		return;
	}
	constexpr size_t msgcount = 500;
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

TEST(Network, vectorthreadsafety)
{
	const size_t cores = std::min(std::thread::hardware_concurrency(), 8u);
	if(cores <= 1){
		LOG_WARNING("No threads available for threaded test.");
		return;
	}
	constexpr size_t msgcount = 500;
	constexpr size_t vecsize = 3;
	n_network::Network n(cores);
	std::vector<std::thread> workers;
	for (size_t i = 0; i < cores; ++i) {
		workers.push_back(std::thread(pushVector, msgcount, vecsize, i, std::ref(n), cores));
		workers.push_back(std::thread(pull, msgcount*vecsize, i, std::ref(n), cores));
	}
	for (auto& t : workers) {
		t.join();
	}
}

TEST(Network, mixedthreadsafety)
{
	const size_t cores = std::min(std::thread::hardware_concurrency(), 8u);
	if(cores <= 1){
		LOG_WARNING("No threads available for threaded test.");
		return;
	}
	constexpr size_t msgcount = 500;
	constexpr size_t vecsize = 3;
	n_network::Network n(cores);
	std::vector<std::thread> workers;
	for (size_t i = 0; i < cores; ++i) {
		workers.push_back(std::thread(pushVector, msgcount, vecsize, i, std::ref(n), cores));
		workers.push_back(std::thread(push, msgcount, i, std::ref(n), cores));
		workers.push_back(std::thread(pull, msgcount*(vecsize+1), i, std::ref(n), cores));
	}
	for (auto& t : workers) {
		t.join();
	}
}

void benchNetworkSpeed()
{
	// Each thread pushes msgcount * cores-1 messages, pulls msgcount * cores-1messages.
	constexpr size_t cores = 4;
	if(cores <= 1){
		LOG_WARNING("No threads available for threaded test.");
		return;
	}
	constexpr size_t msgcount = 20000;
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
#if (LOG_LEVEL!=0)
	std::chrono::duration<double> elapsed_seconds = end-start;
	//std::time_t end_time = std::chrono::system_clock::to_time_t(end);
	std::size_t totalcount = (msgcount * (cores-1))*cores;
//	std::cout << "Sending / Receiving of  " << totalcount << " messages finished at\t" << std::ctime(&end_time)
	//<< "elapsed time: " << elapsed_seconds.count() << "s\n";

	LOG_INFO("NETWORK: Network element with up to ", cores, " queues simulated with  ", cores*2, " threads.");
	LOG_INFO("NETWORK: Logging == ", LOGGING);
	LOG_INFO("NETWORK: Processing speed: ", totalcount/ (elapsed_seconds.count()), "msg / s");
#endif
}

TEST(Time, FloatingPoint){
        constexpr size_t testsizefp=1000;
        constexpr size_t testsizecs=100;
        constexpr double stepsize = 1e-9;       // MASSIVE collissions.
        typedef Time<double, size_t> fptime;
        std::unordered_set<fptime> times;
        std::vector<fptime> times_backup;
        // Test hashing operator
        double fpvalue = 0;
        for(size_t i = 0; i<testsizefp;++i){
                size_t causal = 0;
                for(size_t j = 0; j< testsizecs; ++j){
                        const fptime testvalue(fpvalue, causal);
                        bool insert = times.insert(testvalue).second;
                        times_backup.push_back(testvalue);
                        EXPECT_TRUE(insert);
                        fpvalue += stepsize;
                        ++causal;
                }
        }
        EXPECT_EQ(times.size(), testsizefp*testsizecs);
        for(const auto& timeval : times_backup){
                EXPECT_TRUE(times.find(timeval)!=times.end());
        }
        // Test equality.
        auto newend = std::unique(times_backup.begin(), times_backup.end());
        // If any duplicates are found, newend is no longer equal to end.
        EXPECT_EQ(newend, times_backup.end());
        fptime left(1.0, 0);
        // Difference is less than defined epsilon.
        double tval = 1.0+2e-13;
        fptime right (tval, 0);
        EXPECT_TRUE(left == right);
        EXPECT_TRUE(left <= right);
        EXPECT_FALSE(left > right);
        EXPECT_FALSE(left < right);
        EXPECT_FALSE(left != right);
        EXPECT_TRUE(left >= right);
        // Try cancellation effects.
        EXPECT_TRUE(fptime(0,0) == left-right);
        EXPECT_FALSE(left == (left-right));
        
        right = fptime(1.0+2.1e-12, 0); // Larger than epsilon
        EXPECT_FALSE(left == right);
        EXPECT_TRUE(left <= right);
        EXPECT_FALSE(left > right);
        EXPECT_TRUE(left < right);
        EXPECT_TRUE(left != right);
        EXPECT_FALSE(left >= right);
        // Try cancellation effects.
        EXPECT_FALSE(fptime(0,0) == left-right);
        EXPECT_FALSE(left == (left-right));
}
