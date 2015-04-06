/*
 * messagetest.cpp
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#include "message.h"
#include "objectfactory.h"
#include "globallog.h"
#include <queue>

using namespace n_tools;
using namespace n_network;

TEST(Message, TestThreadRaceConditions){
	if(std::thread::hardware_concurrency() <=1)
		LOG_WARNING("Thread test skipped, OS report no threads avaiable");
	auto msg = createObject<Message>("TargetModel", t_timestamp(0,1), "TargetPort", "SourcePort", " cargo ");
	msg->setDestinationCore(42);
	std::string expected_out = "Message from SourcePort to TargetPort @TimeStamp ::0 causal ::1 to model TargetModel @core_nr 42 payload  cargo ";
	EXPECT_EQ(msg->toString(), expected_out);
	std::vector<std::thread> workers;
	// Try to trigger races.
	auto worker = [&]()->void{
		std::string dest = msg->getDestinationModel();
		dest[0] = 'c';
		dest = msg->getDestinationPort();
		dest[0] = 'c';
		dest = msg->getSourcePort();
		dest[0] = 'c';
	};
	for(size_t i = 0; i<4; ++i){
		workers.push_back(std::thread(worker));
	}
	for(auto& t : workers)
		t.join();
	EXPECT_TRUE(msg->getDestinationModel()=="TargetModel");
	EXPECT_TRUE(msg->getSourcePort()=="SourcePort");
	EXPECT_TRUE(msg->getDestinationPort()=="TargetPort");
}

TEST(Message, operators){
	std::shared_ptr<Message> msgbefore = createObject<Message>("TargetModel", t_timestamp(1,0), "TargetPort", "SourcePort", " cargo ");
	std::shared_ptr<Message> msgafter = createObject<Message>("TargetModel", t_timestamp(1,1), "TargetPort", "SourcePort", " cargo ");
	compare_msgptr functor;
	EXPECT_FALSE(functor(msgbefore, msgafter));
	EXPECT_TRUE(functor(msgafter, msgbefore));
	EXPECT_FALSE(functor(msgbefore, msgbefore));
	std::priority_queue<t_msgptr, std::deque<t_msgptr>, compare_msgptr> myqueue;
	myqueue.push(msgbefore);
	myqueue.push(msgafter);
	EXPECT_TRUE(myqueue.size()==2);
	EXPECT_FALSE(myqueue.top() == msgafter);
	myqueue.push(msgbefore);
	EXPECT_TRUE(myqueue.size()==3);
	EXPECT_TRUE(myqueue.top() == msgbefore);
	auto first = myqueue.top();
	myqueue.pop();
	EXPECT_TRUE(first == msgbefore);
	auto tied = myqueue.top();
	myqueue.pop();
	EXPECT_TRUE(tied == msgbefore);
	auto last = myqueue.top();
	myqueue.pop();
	EXPECT_TRUE(last == msgafter);
}
