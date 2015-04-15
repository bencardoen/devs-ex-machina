/*
 * messagetest.cpp
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#include "message.h"
#include "messageentry.h"
#include "scheduler.h"
#include "schedulerfactory.h"
#include "unordered_map"
#include "objectfactory.h"
#include "globallog.h"
#include <queue>

using namespace n_tools;
using namespace n_network;

TEST(Message, TestThreadRaceConditions){
	/**
	 * std::string has a copy-on-write implementation that can trigger race conditions in threaded code.
	 * Test if the message class can avoid these by forcing the compiler to copy strings and not cow.
	 */
	if(std::thread::hardware_concurrency() <=1)
		LOG_WARNING("Thread test skipped, OS report no threads avaiable");
	auto msg = createObject<Message>("TargetModel", t_timestamp(0,1), "TargetPort", "SourcePort", " cargo ");
	msg->setDestinationCore(42);
	msg->setSourceCore(1);
	std::string expected_out = "Message from SourcePort to TargetPort @TimeStamp ::0 causal ::1 to model TargetModel from kernel 1 to kernel 42 payload  cargo  color : WHITE";
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
	/**
	 * Test if Message, MessageEntry 's operators behave as expected.
	 */
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
	std::size_t msghashbefore = std::hash<Message>()(*msgbefore);
	std::size_t msghashafter = std::hash<Message>()(*msgafter);
	EXPECT_TRUE(msghashbefore != msghashafter);
	MessageEntry mbefore(msgbefore);
	MessageEntry mafter(msgafter);
	EXPECT_TRUE(mbefore != mafter);
	EXPECT_TRUE(mbefore > mafter);
	EXPECT_TRUE(mbefore >= mafter);
	EXPECT_FALSE(mbefore < mafter);
	EXPECT_FALSE(mbefore <= mafter);
	EXPECT_TRUE(std::hash<Message>()(*msgbefore) == std::hash<MessageEntry>()(mbefore));
	// Copy constructors
	MessageEntry copy(mbefore);
	EXPECT_TRUE(std::hash<Message>()(*msgbefore) == std::hash<MessageEntry>()(copy));
	MessageEntry assigned = mbefore;
	EXPECT_TRUE(assigned == mbefore);
	EXPECT_TRUE(copy == mbefore);
	EXPECT_TRUE(std::hash<Message>()(*msgbefore) == std::hash<MessageEntry>()(assigned));
	std::unordered_set<MessageEntry> myset;
	myset.insert(mbefore);
	EXPECT_EQ(myset.insert(copy).second, false);
	EXPECT_EQ(myset.insert(assigned).second, false);
	EXPECT_EQ(myset.insert(msgafter).second, true);
	auto scheduler = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	EXPECT_FALSE(scheduler->isLockable());
	for(size_t i = 0; i<100; ++i){
		std::shared_ptr<Message> msg = createObject<Message>("TargetModel", t_timestamp(i,0), "TargetPort", "SourcePort", " cargo ");
		MessageEntry entry(msg);
		scheduler->push_back(entry);
		EXPECT_EQ(scheduler->size(), i+1);
		EXPECT_TRUE(scheduler->contains(entry));
	}
	std::shared_ptr<Message> antimessage = createObject<Message>("TargetModel", t_timestamp(55,0), "TargetPort", "SourcePort", " cargo ");
	scheduler->erase(MessageEntry(antimessage));
	EXPECT_FALSE(scheduler->contains(MessageEntry(antimessage)));
	//scheduler->printScheduler();
	std::vector<MessageEntry> popped;
	std::shared_ptr<Message> token = createObject<Message>("", t_timestamp(55,0), "", "", "");
	MessageEntry tokentime(token);
	scheduler->unschedule_until(popped, tokentime);
	EXPECT_EQ(popped.size(), 55);
	EXPECT_EQ(scheduler->size(), 44);
	scheduler->clear();
	EXPECT_EQ(scheduler->size(), 0);
}

TEST(Message, Antimessage){
	auto scheduler = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	std::shared_ptr<Message> msg = createObject<Message>("TargetModel", t_timestamp(55,0), "TargetPort", "SourcePort", " cargo ");
	msg->setDestinationCore(1);
	msg->setSourceCore(0);
	t_msgptr antimessage = n_tools::createObject<Message>(msg->getDestinationModel(), msg->getTimeStamp(), msg->getDestinationPort(), msg->getSourcePort(), msg->getPayload());
	antimessage->setDestinationCore(0);
	antimessage->setSourceCore(1);
	scheduler->push_back(MessageEntry(msg));
	EXPECT_TRUE(scheduler->contains(MessageEntry(msg)));
	scheduler->erase(MessageEntry(antimessage));
	EXPECT_FALSE(scheduler->contains(MessageEntry(msg)));
	EXPECT_FALSE(scheduler->contains(MessageEntry(antimessage)));
}
