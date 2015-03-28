/*
 * messagetest.cpp
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */
#include "message.h"
#include "objectfactory.h"
#include "globallog.h"

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
