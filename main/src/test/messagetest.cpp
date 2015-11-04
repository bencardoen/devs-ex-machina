/*
 * messagetest.cpp
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#include "network/message.h"
#include "network/messageentry.h"
#include "scheduler/scheduler.h"
#include "scheduler/schedulerfactory.h"
#include <unordered_map>
#include <unordered_set>
#include "tools/objectfactory.h"
#include "tools/globallog.h"
#include "network/mid.h"
#include <gtest/gtest.h>
#include <queue>
#include <thread>

using namespace n_tools;
using namespace n_network;

TEST(Message, operators){
        {
                /**
                 * Test if Message, MessageEntry 's operators behave as expected.
                 */
                t_msgptr msgbefore = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp(1,0), 3u, 2u);
                t_msgptr msgafter = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp(1,1), 3u, 2u);

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
                delete msgbefore;
                delete msgafter;
        }
        
	auto scheduler = n_scheduler::SchedulerFactory<MessageEntry>::makeScheduler(n_scheduler::Storage::FIBONACCI, false);
	
        t_msgptr am;
        /// Create N objects, test push/contains
	for(size_t i = 0; i<100; ++i){
		t_msgptr msg = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp(i,0), 3u, 2u);
		MessageEntry entry(msg);
                if(i==55)
                        am=msg;
		scheduler->push_back(entry);
		EXPECT_EQ(scheduler->size(), i+1);
		EXPECT_TRUE(scheduler->contains(entry));
	}
        
	{
                /// Check hash / operator< antimessaging.
                //t_msgptr antimessage = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp(55,0), 3u, 2u);
                scheduler->erase(MessageEntry(am));
                EXPECT_FALSE(scheduler->contains(MessageEntry(am)));
                delete am;
        }
        
	//scheduler->printScheduler();
        /// N-1 objects left, test unschedule_until && operator<
	std::vector<MessageEntry> popped;
	t_msgptr token = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp(55,0), 0u, 0u);
	MessageEntry tokentime(token);
	scheduler->unschedule_until(popped, tokentime);
	EXPECT_EQ(popped.size(), 55u);
	EXPECT_EQ(scheduler->size(), 44u);
        
        for(auto entry : popped)
                delete entry.getMessage();
	popped.clear();
        delete token;
        
	token = createRawObject<Message>(n_model::uuid(1, 0), n_model::uuid(42, 0), t_timestamp::infinity(), 0u, 0u);
	scheduler->unschedule_until(popped, token);
        for(auto entry : popped)
                delete entry.getMessage();
        delete token;
	EXPECT_EQ(scheduler->size(), 0u);
}

TEST(Message, Antimessage){
	auto scheduler = n_scheduler::SchedulerFactory<MessageEntry>::makeScheduler(n_scheduler::Storage::FIBONACCI, false);
	t_shared_msgptr msg = createObject<Message>(n_model::uuid(0, 0), n_model::uuid(1, 0), t_timestamp(55,0), 3u, 2u);
	t_shared_msgptr antimessage = n_tools::createObject<Message>(n_model::uuid(0, 0), n_model::uuid(1, 0), msg->getTimeStamp(), msg->getDestinationPort(), msg->getSourcePort());
	scheduler->push_back(MessageEntry(msg.get()));
	EXPECT_TRUE(scheduler->contains(MessageEntry(msg.get())));
	scheduler->erase(MessageEntry(antimessage.get()));
	EXPECT_FALSE(scheduler->contains(MessageEntry(msg.get())));
	EXPECT_FALSE(scheduler->contains(MessageEntry(antimessage.get())));
}

TEST(Message, Smoketest){
	//// Try to break scheduler.
	auto scheduler = n_scheduler::SchedulerFactory<MessageEntry>::makeScheduler(n_scheduler::Storage::FIBONACCI, false);

	for(size_t i = 0; i<1000; ++i){
		t_msgptr msg = createRawObject<Message>(n_model::uuid(0, 0), n_model::uuid(1, 0), t_timestamp(0,i), 3u, 2u);
		t_msgptr antimessage = n_tools::createRawObject<Message>(n_model::uuid(0, 0), n_model::uuid(1, 0), msg->getTimeStamp(), msg->getDestinationPort(), msg->getSourcePort());
		EXPECT_FALSE(scheduler->contains(MessageEntry(msg)));
		size_t oldsize = scheduler->size();
		scheduler->push_back(MessageEntry(msg));
		EXPECT_TRUE(oldsize == scheduler->size()-1);
		EXPECT_TRUE(scheduler->contains(MessageEntry(msg)));
		EXPECT_TRUE(scheduler->contains(MessageEntry(antimessage)));
		scheduler->erase(MessageEntry(antimessage));
		EXPECT_FALSE(scheduler->contains(MessageEntry(msg)));
		EXPECT_FALSE(scheduler->contains(MessageEntry(antimessage)));
                delete msg;
                delete antimessage;
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
	t_shared_msgptr msgStr = n_tools::createObject<SpecializedMessage<std::string>>(n_model::uuid(1, 0), n_model::uuid(42, 0), 1, 3u, 2u, "payload");
        EXPECT_FALSE(msgStr->flagIsSet(Status::DELETE));
	EXPECT_EQ(msgStr->getDestinationPort(), 3u);
	EXPECT_EQ(msgStr->getSourcePort(), 2u);
	EXPECT_EQ(msgStr->getPayload(), "payload");
	EXPECT_EQ(msgStr->getDestinationCore(), 42u);
	std::string str = n_network::getMsgPayload<std::string>(msgStr.get());
	EXPECT_EQ(str, "payload");

	t_shared_msgptr msgDouble = n_tools::createObject<SpecializedMessage<double>>(n_model::uuid(1, 0), n_model::uuid(42, 0), 1, 3u, 2u, 3.14);
	EXPECT_EQ(msgDouble->getDestinationPort(), 3u);
	EXPECT_EQ(msgDouble->getSourcePort(), 2u);
	EXPECT_EQ(msgDouble->getDestinationCore(), 42u);
	const double& doub = n_network::getMsgPayload<double>(msgDouble.get());
	EXPECT_EQ(doub, 3.14);

	MyStruct data = {-2, 't', 42.24};
	MyStruct control = data;
	t_shared_msgptr msgMyStruct = n_tools::createObject<SpecializedMessage<MyStruct>>(n_model::uuid(1, 0), n_model::uuid(42, 0), 1, 3u, 2u, data);
	EXPECT_EQ(msgMyStruct->getDestinationPort(), 3u);
	EXPECT_EQ(msgMyStruct->getSourcePort(), 2u);
	EXPECT_EQ(msgDouble->getDestinationCore(), 42u);
	const MyStruct& res = n_network::getMsgPayload<MyStruct>(msgMyStruct.get());
	data.i++;
	EXPECT_EQ(res.i, control.i);
	EXPECT_EQ(res.c, control.c);
	EXPECT_EQ(res.d, control.d);
	EXPECT_NE(data.i, control.i);
}

TEST(Message, PackedID){
	mid z;
        EXPECT_EQ(z.coreid(), 0u);
        EXPECT_EQ(z.portid(), 0u);
        EXPECT_EQ(z.modelid(), 0u);
        size_t p = 255;
        size_t c = 255;
        size_t m = 12345;
        mid myid(255, 255, 12345);
        EXPECT_EQ(myid.coreid(), c);
        EXPECT_EQ(myid.portid(), p);
        EXPECT_EQ(myid.modelid(), m);
        {
                mid t;
                for(size_t i = 0; i <= n_const::port_max; ++i){
                        t.setportid(i);
                        EXPECT_EQ(t.coreid(), 0u);
                        EXPECT_EQ(t.modelid(), 0u);
                        EXPECT_EQ(t.portid(), i);
                }
                
                for(size_t i = 0; i<= n_const::core_max; ++i){
                        t.setcoreid(i);
                        EXPECT_EQ(t.coreid(), i);
                        EXPECT_EQ(t.modelid(), 0u);
                        EXPECT_EQ(t.portid(), n_const::port_max);
                }
                
                for(size_t i = 1; i<= n_const::model_max; i*=2){ // 2^48 is a ~bit~ much for a test.
                        t.setmodelid(i);
                        EXPECT_EQ(t.coreid(), n_const::core_max);
                        EXPECT_EQ(t.modelid(), i);
                        EXPECT_EQ(t.portid(), n_const::port_max);
                }
        }
}

TEST(Message, Integrity){
	t_msgptr msg = n_tools::createRawObject<SpecializedMessage<double>>(n_model::uuid(n_const::core_max, n_const::model_max), n_model::uuid(254, 2096), t_timestamp(1,1), n_const::port_max, 249, std::numeric_limits<double>::max());
	const double& d = n_network::getMsgPayload<double>(msg);
	EXPECT_EQ(d, std::numeric_limits<double>::max());
        EXPECT_EQ(msg->getColor(), MessageColor::WHITE);
        msg->paint(MessageColor::RED);
        EXPECT_FALSE(msg->isAntiMessage());
        msg->setAntiMessage(true);
        EXPECT_TRUE(msg->isAntiMessage());
        EXPECT_EQ(msg->getColor(), MessageColor::RED);
        EXPECT_FALSE(msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::DELETE);
        EXPECT_FALSE(!msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::PROCESSED);
        EXPECT_FALSE(!msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(!msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::PENDING);
        EXPECT_FALSE(!msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(!msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(!msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::DELETE, false);
        EXPECT_FALSE(msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(!msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(!msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::PROCESSED, false);
        EXPECT_FALSE(msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(!msg->flagIsSet(Status::PENDING));
        msg->setFlag(Status::PENDING, false);
        EXPECT_FALSE(msg->flagIsSet(Status::DELETE));
        EXPECT_FALSE(msg->flagIsSet(Status::PROCESSED));
        EXPECT_FALSE(msg->flagIsSet(Status::PENDING));
        EXPECT_EQ(msg->getSourcePort(),249u);
        EXPECT_EQ(msg->getSourceCore(),n_const::core_max);
        EXPECT_EQ(msg->getSourceModel(),n_const::model_max);
        EXPECT_EQ(msg->getDestinationPort(),n_const::port_max);
        EXPECT_EQ(msg->getDestinationCore(),254u);
        EXPECT_EQ(msg->getDestinationModel(),2096u);
        delete msg;
}