/*
 * messagetest.cpp
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#include "network/message.h"
#include "network/messageentry.h"
#include "tools/scheduler.h"
#include "tools/schedulerfactory.h"
#include <unordered_map>
#include "tools/objectfactory.h"
#include "tools/globallog.h"
#include <gtest/gtest.h>
#include <queue>
#include <thread>

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
		workers.emplace_back(worker);
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
	auto scheduler = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false);
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
	EXPECT_EQ(popped.size(), 55u);
	EXPECT_EQ(scheduler->size(), 44u);
	popped.clear();
	token.reset();
	token = createObject<Message>("", t_timestamp::infinity(), "", "", "");
	MessageEntry endtime(token);
	scheduler->unschedule_until(popped, token);
	EXPECT_EQ(scheduler->size(), 0u);
}

TEST(Message, Antimessage){
	auto scheduler = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false);
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

TEST(Message, Smoketest){
	//// Try to break scheduler.
	auto scheduler = n_tools::SchedulerFactory<MessageEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false);
	for(size_t i = 0; i<1000; ++i){
		std::shared_ptr<Message> msg = createObject<Message>("TargetModel", t_timestamp(0,i), "TargetPort", "SourcePort", " cargo ");
		msg->setDestinationCore(1);
		msg->setSourceCore(0);
		t_msgptr antimessage = n_tools::createObject<Message>(msg->getDestinationModel(), msg->getTimeStamp(), msg->getDestinationPort(), msg->getSourcePort(), msg->getPayload());
		antimessage->setDestinationCore(0);
		antimessage->setSourceCore(1);
		EXPECT_FALSE(scheduler->contains(msg));
		size_t oldsize = scheduler->size();
		scheduler->push_back(MessageEntry(msg));
		EXPECT_TRUE(oldsize == scheduler->size()-1);
		EXPECT_TRUE(scheduler->contains(MessageEntry(msg)));
		EXPECT_TRUE(scheduler->contains(MessageEntry(antimessage)));
		scheduler->erase(MessageEntry(antimessage));
		EXPECT_FALSE(scheduler->contains(MessageEntry(msg)));
		EXPECT_FALSE(scheduler->contains(MessageEntry(antimessage)));
	}
}

struct MyStruct
{
	int i;
	char c;
	double d;
};
std::ostream& operator<<(std::ostream& o, const MyStruct& m){
	o << m.i << ' ' << m.c << ' ' << m.d;
	return o;
}

TEST(Message, ContentTest){
	t_msgptr msgStr = n_tools::createObject<Message>("model", 1, "dest", "source", "payload");
	EXPECT_EQ(msgStr->getDestinationPort(), "dest");
	EXPECT_EQ(msgStr->getSourcePort(), "source");
	EXPECT_EQ(msgStr->getPayload(), "payload");
	EXPECT_EQ(msgStr->getDestinationModel(), "model");
	std::string str = n_network::getMsgPayload<std::string>(msgStr);
	EXPECT_EQ(str, "payload");

	t_msgptr msgDouble = n_tools::createObject<SpecializedMessage<double>>("model", 1, "dest", "source", 3.14);
	EXPECT_EQ(msgDouble->getDestinationPort(), "dest");
	EXPECT_EQ(msgDouble->getSourcePort(), "source");
	EXPECT_EQ(msgDouble->getDestinationModel(), "model");
	const double& doub = n_network::getMsgPayload<double>(msgDouble);
	EXPECT_EQ(doub, 3.14);

	MyStruct data = {-2, 't', 42.24};
	MyStruct control = data;
	t_msgptr msgMyStruct = n_tools::createObject<SpecializedMessage<MyStruct>>("model", 1, "dest", "source", data);
	EXPECT_EQ(msgMyStruct->getDestinationPort(), "dest");
	EXPECT_EQ(msgMyStruct->getSourcePort(), "source");
	EXPECT_EQ(msgMyStruct->getDestinationModel(), "model");
	const MyStruct& res = n_network::getMsgPayload<MyStruct>(msgMyStruct);
	data.i++;
	EXPECT_EQ(res.i, control.i);
	EXPECT_EQ(res.c, control.c);
	EXPECT_EQ(res.d, control.d);
	EXPECT_NE(data.i, control.i);
}
