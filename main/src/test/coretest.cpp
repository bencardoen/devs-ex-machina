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
#include "core.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_model;
using namespace n_tools;

TEST(ModelEntry, Scheduling){
	RecordProperty("description", "Tests if models can be scheduled with identical timestamps, and timestamps differing only in causality.");
	auto scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
	EXPECT_TRUE(scheduler->empty());
	std::stringstream s;
	for(size_t i = 0; i<100; ++i){
		s << i;
		std::string name = s.str();
		s.str(std::string(""));
		scheduler->push_back(ModelEntry(name, t_timestamp(i, 0)));
		EXPECT_EQ(scheduler->size(), i+1);
	}
	std::vector<ModelEntry> imminent;
	ModelEntry token ("", t_timestamp(50, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 50);
	token = ModelEntry("", t_timestamp(100,0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 0);

	// Test if scheduling models at same time is a problem
	ModelEntry origin ("Abc", t_timestamp(0));
	ModelEntry duplicate ("Bca", t_timestamp(0));
	ModelEntry third ("Cab", t_timestamp(0, 1));
	scheduler->push_back(origin);
	scheduler->push_back(duplicate);
	EXPECT_EQ(scheduler->size(), 2);
	scheduler->push_back(third);
	EXPECT_EQ(scheduler->size(), 3);
	ModelEntry found = scheduler->pop();
	EXPECT_EQ(found.getName() , "Abc");
	EXPECT_EQ(scheduler->pop().getName(), "Bca");
	EXPECT_EQ(scheduler->pop().getName(), "Cab");
	EXPECT_EQ(scheduler->size(), 0);
}

TEST(ModelScheduling, BasicOperations){
	RecordProperty("description", "Verify that std::hash, std::less and related operators are well defined and execute as expected.");
	ModelEntry me("alone", t_timestamp(0,0));
	ModelEntry you("home", t_timestamp(0,0));
	EXPECT_FALSE(me == you);
	std::unordered_set<ModelEntry> set;
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(),2);
	set.clear();
	EXPECT_EQ(set.size(),0);
	// This is evil, and is never guaranteed to work.
	// A model entry with the same name is equal no matter what time is set.
	// The alternative allows insertion multiple times (an error).
	// It's written as a test to detect if/when somebody clobbers the logic of the operators in devious ways.
	me = ModelEntry("alone", t_timestamp(1,0));
	you = ModelEntry("alone", t_timestamp(1,1));
	EXPECT_TRUE(me == you);
	EXPECT_TRUE(me > you); // Note this is so the max-heap property works as a min heap.
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 1);
}

TEST(Core, CoreFlow){
	RecordProperty("description", "Verify that Core can (re)schedule models, model lookup is working and core can advance in time.");
	using n_network::Message;
	Core c; // single core.
	EXPECT_EQ(c.getCoreID(), 0);
	t_msgptr mymessage = createObject<Message>("toBen", (0));
	t_atomicmodelptr modelfrom = createObject<modelstub>("Amodel");
	t_atomicmodelptr modelto = createObject<modelstub>("toBen");
	EXPECT_EQ(modelfrom->getName(), "Amodel");
	c.addModel(modelfrom);
	EXPECT_EQ(c.getModel("Amodel"), modelfrom);
	c.addModel(modelto);
	EXPECT_EQ(c.getModel("toBen"), modelto);
	EXPECT_FALSE(mymessage->getDestinationCore() == 0);
	EXPECT_TRUE(c.isMessageLocal(mymessage));
	EXPECT_TRUE(mymessage->getDestinationCore() == 0);
	c.init();
	//c.printSchedulerState();
	EXPECT_TRUE(c.getTime() == t_timestamp(10));
	auto imminent  = c.getImminent();
	EXPECT_EQ(imminent.size(), 2);
	//for(const auto& el : imminent)	std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	imminent = c.getImminent();
	//for(const auto& el : imminent)		std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	//c.printSchedulerState();
	EXPECT_EQ(imminent.size(), 2);
}

// TODO Matthijs : this is how a Core expects to be run.
TEST(Core, smallStep){
	RecordProperty("description", "Core simulation steps and termination conditions");
	Core c;
	// Add Models
	t_atomicmodelptr modelfrom = createObject<modelstub>("Amodel");
	t_atomicmodelptr modelto = createObject<modelstub>("toBen");
	c.addModel(modelfrom);
	c.addModel(modelto);

	// Initialize (loads models by ta() into scheduler
	c.init();
	// Set termination conditions (optional), both are checked (time first, then function)
	auto finaltime = c.getTerminationTime();

	EXPECT_EQ(finaltime , t_timestamp::infinity());
	c.setTerminationTime(t_timestamp(24, 0));
	finaltime = c.getTerminationTime();
	EXPECT_EQ(finaltime , t_timestamp(24,0));

	t_timestamp coretimebefore = c.getTime();
	// Switch 'on' Core.
	c.setLive(true);
	EXPECT_TRUE(c.terminated() == false);
	EXPECT_TRUE(c.isLive()== true);

	// Run simulation.
	c.runSmallStep();
	t_timestamp coretimeafter = c.getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c.terminated() == false);
	EXPECT_TRUE(c.isLive()== true);

	c.runSmallStep();
	EXPECT_TRUE(c.terminated() == true);
	EXPECT_TRUE(c.isLive()== false);
}

TEST(Core, terminationfunction){
	RecordProperty("description", "Core simulation steps and termination conditions");
	Core c;
	// Add Models
	t_atomicmodelptr modelfrom = createObject<modelstub>("Amodel");
	t_atomicmodelptr modelto = createObject<modelstub>("toBen");
	c.addModel(modelfrom);
	c.addModel(modelto);

	// Initialize (loads models by ta() into scheduler
	c.init();
	// Set termination conditions (optional), both are checked (time first, then function)
	auto finaltime = c.getTerminationTime();
	auto termfun = [](const t_atomicmodelptr& model)->bool{
		if(model->getName() == "Amodel")
			return true;
		return false;
	};

	EXPECT_EQ(finaltime , t_timestamp::infinity());
	c.setTerminationFunction(termfun);

	t_timestamp coretimebefore = c.getTime();
	// Switch 'on' Core.
	c.setLive(true);
	EXPECT_TRUE(c.terminated() == false);
	EXPECT_TRUE(c.isLive()== true);

	// Run simulation.
	c.runSmallStep();
	t_timestamp coretimeafter = c.getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c.terminated() == true);
	EXPECT_TRUE(c.isLive()== false);

}

