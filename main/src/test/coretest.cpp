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
#include "multicore.h"
#include "trafficlight.h"
#include "trafficlightc.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_model;
using namespace n_tools;
using namespace n_examples;

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
	std::string portname_stub = "model_port";
	t_msgptr mymessage = createObject<Message>("toBen", (0), portname_stub, portname_stub);
	t_atomicmodelptr modelfrom = createObject<TrafficLight>("Amodel");
	t_atomicmodelptr modelto = createObject<TrafficLight>("toBen");
	EXPECT_EQ(modelfrom->getName(), "Amodel");
	c.addModel(modelfrom);
	EXPECT_EQ(c.getModel("Amodel"), modelfrom);
	c.addModel(modelto);
	EXPECT_EQ(c.getModel("toBen"), modelto);
	EXPECT_FALSE(mymessage->getDestinationCore() == 0);
	c.init();
	//c.printSchedulerState();
	EXPECT_TRUE(c.getTime() == t_timestamp(60));
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
	t_coreptr c = createObject<Core>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
	//TODO Matthijs : Creating the tracers should be done by the user, not the Controller
	// Add Models
	t_atomicmodelptr modelfrom = createObject<TrafficLight>("Amodel");
	t_atomicmodelptr modelto = createObject<TrafficLight>("toBen");
	c->addModel(modelfrom);
	c->addModel(modelto);

	// Initialize (loads models by ta() into scheduler
	c->init();
	// Set termination conditions (optional), both are checked (time first, then function)
	auto finaltime = c->getTerminationTime();

	EXPECT_EQ(finaltime , t_timestamp::infinity());
	c->setTerminationTime(t_timestamp(200, 0));
	finaltime = c->getTerminationTime();
	EXPECT_EQ(finaltime , t_timestamp(200,0));

	t_timestamp coretimebefore = c->getTime();
	// Switch 'on' Core.
	c->setLive(true);
	EXPECT_TRUE(c->terminated() == false);
	EXPECT_TRUE(c->isLive()== true);

	// Run simulation.
	c->runSmallStep();
	t_timestamp coretimeafter = c->getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c->terminated() == false);
	EXPECT_TRUE(c->isLive()== true);
}

class termfun: public n_model::TerminationFunctor{
public:
	virtual
	bool evaluateModel(const t_atomicmodelptr& model)const override{
		return (model->getName()=="Amodel");
	}
};


TEST(Core, terminationfunction){
	RecordProperty("description", "Core simulation steps with term function.");
	t_coreptr c = createObject<Core>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
	// Add Models
	t_atomicmodelptr modelfrom = createObject<TrafficLight>("Amodel");
	t_atomicmodelptr modelto = createObject<TrafficLight>("toBen");
	c->addModel(modelfrom);
	c->addModel(modelto);

	// Initialize (loads models by ta() into scheduler
	c->init();
	// Set termination conditions (optional), both are checked (time first, then function)
	auto finaltime = c->getTerminationTime();
	EXPECT_EQ(finaltime , t_timestamp::infinity());
	c->setTerminationFunction(createObject<termfun>());

	t_timestamp coretimebefore = c->getTime();
	// Switch 'on' Core.
	c->setLive(true);
	EXPECT_TRUE(c->terminated() == false);
	EXPECT_TRUE(c->isLive()== true);

	// Run simulation.
	c->runSmallStep();
	t_timestamp coretimeafter = c->getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c->terminated() == true);
	EXPECT_TRUE(c->isLive()== false);
	c->removeModel("Amodel");
}

void core_worker(const t_coreptr& core){
	core->setLive(true);
	core->init();

	while(core->isLive()){
		core->runSmallStep();
	}
}

TEST(Core, multicoresafe){
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	t_coreptr coreone = createObject<n_model::Multicore>(network, 1, loctable);
	coreone->setTracers(tracers);
	t_coreptr coretwo = createObject<n_model::Multicore>(network, 0, loctable);
	coretwo->setTracers(tracers);
	std::unordered_map<std::string, std::vector<t_msgptr>> mailstubone;
	std::unordered_map<std::string, std::vector<t_msgptr>> mailstubtwo;
	coreone->getMessages(mailstubone);
	coretwo->getMessages(mailstubtwo);
	std::vector<t_coreptr> coreptrs;
	coreptrs.push_back( coreone);
	coreptrs.push_back( coretwo);
	auto tcmodel = createObject<TrafficLight>("mylight", 0);
	auto tc2model = createObject<TrafficLight>("myotherlight", 0);
	coreone->addModel(tcmodel);
	coreone->setTerminationTime(t_timestamp(20000,0));
	coretwo->addModel(tc2model);
	coretwo->setTerminationTime(t_timestamp(20000,0));
	//const size_t cores = std::thread::hardware_concurrency();
	const size_t threadcount = 2;
	if(threadcount <= 1){
		LOG_WARNING("Skipping test, no threads!");
		return;
	}
	std::vector<std::thread> workers;
	for(size_t i = 0; i<threadcount; ++i){
		workers.push_back(std::thread(core_worker, std::cref(coreptrs[i])));
	}
	for(auto& worker : workers){
		worker.join();
	}
	coreone->signalTracersFlush();
	coretwo->signalTracersFlush();
	EXPECT_TRUE(coreone->getTime() >= coreone->getTerminationTime());
	EXPECT_TRUE(coretwo->getTime() >= coretwo->getTerminationTime());
}

