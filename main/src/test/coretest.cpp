/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen, Stijn Manhaeve
 */

#include <gtest/gtest.h>
#include "network/timestamp.h"
#include "tools/objectfactory.h"
#include "examples/trafficlight_classic/trafficlight.h"
#include "examples/abstract_conservative/modelc.h"
#include "examples/trafficlight_coupled/trafficlightc.h"
#include "examples/trafficlight_coupled/policemanc.h"
#include "control/controller.h"
#include "model/conservativecore.h"
#include "control/simpleallocator.h"
#include "tracers/tracers.h"
#include "tools/coutredirect.h"
#include "examples/trafficlight_coupled/trafficsystemc.h"
#include "control/controllerconfig.h"
#include "control/controller.h"
#include "test/compare.h"
#include "examples/deadlock/pingset.h"
#include <sstream>
#include <unordered_set>
#include <thread>
#include <sstream>
#include <chrono>

using namespace n_model;
using namespace n_tools;
using namespace n_control;
using namespace n_examples;

typedef n_examples::TrafficLight ATOMIC_TRAFFICLIGHT;
typedef n_examples_coupled::TrafficLight COUPLED_TRAFFICLIGHT;
typedef n_examples_coupled::Policeman COUPLED_POLICEMAN;

TEST(ModelEntry, Scheduling)
{
	RecordProperty("description",
	        "Tests if models can be scheduled with identical timestamps, and timestamps differing only in causality.");
	auto scheduler = n_scheduler::SchedulerFactory<ModelEntry>::makeScheduler(n_scheduler::Storage::FIBONACCI, false);
	EXPECT_TRUE(scheduler->empty());
	std::stringstream s;
	for (size_t i = 0; i < 100; ++i) {
		scheduler->push_back(ModelEntry(i, t_timestamp(i, 0)));
		EXPECT_EQ(scheduler->size(), i + 1);
	}
	std::vector<ModelEntry> imminent;
	ModelEntry token(0, t_timestamp(50, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 50u);
	token = ModelEntry(0, t_timestamp(100, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 0u);

	// Test if scheduling models at same time is a problem
	ModelEntry origin(1, t_timestamp(0));
	ModelEntry duplicate(2, t_timestamp(0));
	ModelEntry third(3, t_timestamp(0, 1));
	scheduler->push_back(origin);
	scheduler->push_back(duplicate);
	EXPECT_EQ(scheduler->size(), 2u);
	scheduler->push_back(third);
	EXPECT_EQ(scheduler->size(), 3u);
	ModelEntry found = scheduler->pop();
	EXPECT_EQ(found.getID(), 1u);
	EXPECT_EQ(scheduler->pop().getID(), 2u);
	EXPECT_EQ(scheduler->pop().getID(), 3u);
	EXPECT_EQ(scheduler->size(), 0u);
}

TEST(ModelScheduling, BasicOperations)
{
	RecordProperty("description",
	        "Verify that std::hash, std::less and related operators are well defined and execute as expected.");
	ModelEntry me(1, t_timestamp(0, 0));
	ModelEntry you(2, t_timestamp(0, 0));
	EXPECT_FALSE(me == you);
	std::unordered_set<ModelEntry> set;
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 2u);
	set.clear();
	EXPECT_EQ(set.size(), 0u);
	
	me = ModelEntry(1, t_timestamp(1, 0));
	you = ModelEntry(1, t_timestamp(1, 1));
	EXPECT_TRUE(me == you);
	EXPECT_TRUE(me > you); // Note this is so the max-heap property works as a min heap.
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 1u);
}

TEST(Core, CoreFlow)
{
	RecordProperty("description",
	        "Verify that Core can (re)schedule models, model lookup is working and core can advance in time.");
	using n_network::Message;
	Core c; // single core.
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c.setTracers(tracers);
	EXPECT_EQ(c.getCoreID(), 0u);
	std::size_t portname_stub = 1;
	t_atomicmodelptr modelfrom = createObject<ATOMIC_TRAFFICLIGHT>("Amodel");
	t_atomicmodelptr modelto = createObject<ATOMIC_TRAFFICLIGHT>("toBen");
	EXPECT_EQ(modelfrom->getName(), "Amodel");
	c.addModel(modelfrom);
	EXPECT_EQ(c.getModel("Amodel"), modelfrom);
	c.addModel(modelto);
	EXPECT_EQ(c.getModel("toBen"), modelto);
	c.init();
	t_msgptr mymessage = createRawObject<Message>(modelfrom->getUUID(), modelto->getUUID(), (0), portname_stub, portname_stub);
	EXPECT_EQ(mymessage->getDestinationCore(), 0u);
	EXPECT_EQ(mymessage->getSourceCore(), 0u);
	//c.printSchedulerState();
	c.syncTime();
	EXPECT_EQ(c.getTime().getTime() , t_timestamp(60, 0).getTime());
        std::vector<t_raw_atomic> imminent;
	c.getImminent(imminent);
	EXPECT_EQ(imminent.size(), 2u);
	c.rescheduleImminent();
	c.syncTime();
        imminent.clear();
	c.getImminent(imminent);
	c.rescheduleImminent();
	//c.printSchedulerState();
	c.syncTime();
	EXPECT_EQ(imminent.size(), 2u);
	delete mymessage;
}

TEST(DynamicCore, smallStep)
{
	t_coreptr c = createObject<DynamicCore>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
        std::string mname = "Amodel";
        std::string nname = "toBen";
	t_atomicmodelptr modelfrom = createObject<ATOMIC_TRAFFICLIGHT>(mname);
	t_atomicmodelptr modelto = createObject<ATOMIC_TRAFFICLIGHT>(nname);
        modelfrom->setCorenumber(0);
        modelto->setCorenumber(0);
	c->addModel(modelfrom);
	c->addModel(modelto);
	c->init();
	auto finaltime = c->getTerminationTime();
	EXPECT_TRUE(isInfinity(finaltime));
	c->setTerminationTime(t_timestamp(200, 0));
	finaltime = c->getTerminationTime();
	EXPECT_EQ(finaltime, t_timestamp(200, 0));
	std::vector<t_raw_atomic> imms;
	c->getLastImminents(imms);
	EXPECT_EQ(imms.size(), 0u);
	c->setLive(true);
	c->syncTime();
	while(c->isLive()){
		c->runSmallStep();
		c->getLastImminents(imms);
		EXPECT_EQ(imms.size(), 2u);
		if(imms.size()==2){
			EXPECT_EQ(imms[0],modelfrom.get());
			EXPECT_EQ(imms[1], modelto.get());
		}
	        c->validateModels();
	}
	// This is not how to run a core, but a check of safety blocks.
	c->setLive(true);
	c->removeModelDS(modelfrom->getLocalID());
        c->validateModels();
	EXPECT_EQ(c->containsModel("Amodel"), false);
	c->removeModelDS(modelto->getLocalID());
        c->validateModels();
	EXPECT_EQ(c->containsModel("toBen"), false);
	c->printSchedulerState();
	c->runSmallStep();
	c->printSchedulerState();
	c->getLastImminents(imms);
	EXPECT_EQ(imms.size(), 0u);
}

class termfun: public n_model::TerminationFunctor
{
public:
	virtual
	bool evaluateModel(const t_atomicmodelptr& model) const override
	{
		return (model->getName() == "Amodel");
	}
};

TEST(Core, terminationfunction)
{
	RecordProperty("description", "Core simulation steps with term function.");
	t_coreptr c = createObject<Core>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
	// Add Models
	t_atomicmodelptr modelfrom = createObject<ATOMIC_TRAFFICLIGHT>("Amodel");
	t_atomicmodelptr modelto = createObject<ATOMIC_TRAFFICLIGHT>("toBen");
	c->addModel(modelfrom);
	c->addModel(modelto);

	// Initialize (loads models by ta() into scheduler
	c->init();
	// Set termination conditions (optional), both are checked (time first, then function)
	auto finaltime = c->getTerminationTime();
	EXPECT_TRUE(isInfinity(finaltime));
	c->setTerminationFunction(createObject<termfun>());

	t_timestamp coretimebefore = c->getTime();
	// Switch 'on' Core.
	c->setLive(true);
	EXPECT_TRUE(c->isLive() == true);

	// Run simulation.
	c->runSmallStep();
	t_timestamp coretimeafter = c->getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c->isLive() == false);
	c->removeModel(modelfrom->getLocalID());
}

TEST(Optimisticcore, revert){
        // Valgrind clear
	RecordProperty("description", "Revert/timewarp basic tests.");
	using namespace n_network;
	t_networkptr network = createObject<Network>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	t_coreptr coreone = createObject<n_model::Optimisticcore>(network, 0, 2);
	coreone->setTracers(tracers);
	auto tcmodel = createObject<COUPLED_TRAFFICLIGHT>("mylight", 0);
	coreone->addModel(tcmodel);
	EXPECT_TRUE(coreone->containsModel("mylight"));
	t_timestamp endtime(2000,0);
	coreone->setTerminationTime(endtime);
	coreone->init();
	coreone->initThread();
	coreone->syncTime();
	EXPECT_EQ(coreone->getTime().getTime(), 58u);
	
	coreone->setLive(true);
	coreone->runSmallStep();
	EXPECT_EQ(coreone->getTime().getTime(), 108u);
	
	// Trigger revert
	// Setup message state to test all paths
	
	t_timestamp gvt(62,0);
	t_timestamp aftergvt(63,0);
	
	t_msgptr msgaftergvt = createRawObject<SpecializedMessage<std::string>>(n_model::uuid(0, 42), n_model::uuid(1, 38), aftergvt, 3u, 2u,"a_test");
        LOG_DEBUG("Creating ", msgaftergvt);

        n_tlocal::setRevert(false);
	coreone->setGVT(gvt);
	coreone->revert(gvt);		// We were @110, went back to 62
        n_tlocal::setRevert(false);

	EXPECT_EQ(coreone->getTime(), 62u);
	EXPECT_EQ(coreone->getTime(), coreone->getGVT());
	coreone->setTime(t_timestamp(67,0));	// need to cheat here, else we won't get the result we're aiming for.
	
	msgaftergvt->setAntiMessage(true);
	coreone->receiveMessage(msgaftergvt);		// this (no longer ) triggers a new revert
        coreone->revert(msgaftergvt->getTimeStamp());
        n_tlocal::setRevert(false);
        
	EXPECT_EQ(coreone->getTime().getTime(), 63u);
	coreone->runSmallStep();			// does nothing, check that empty transitioning works. (next = 108, time = 62)

        coreone->setLive(false);
	coreone->shutDown();
	EXPECT_EQ(coreone->getTime().getTime(), 108u);
        // Message is not sent by any core, it's not processed, it's not queued so delete here (since it won't be otherwise)
        delete msgaftergvt;
}


TEST(Optimisticcore, revertidle){
        // Valgrind clear.
	RecordProperty("description", "Revert: test if a core can go from idle/terminated back to working.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
        c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
        c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	//c2->printSchedulerState();

	tracers->startTrace();

	c1->runSmallStep();
	c1->logCoreState();
	c2->runSmallStep();
	c2->logCoreState();

	c1->runSmallStep();
	c1->logCoreState();
	c2->runSmallStep();
	c2->logCoreState();

	c1->runSmallStep();
	c1->logCoreState();		// Cop is terminated @time 500, idle=true, live=false
	c2->runSmallStep();
	c2->logCoreState();		// Trafficlight is at 118::8, queued message from cop @200, @300

	// Test that a Core can go from idle to live again without corrupting.
        n_tlocal::setRevert(false);
	c1->revert(t_timestamp(201,42));
        n_tlocal::setRevert(false);
	// Expecting time = 201,42, revert of model 1x antimessage @300.
	EXPECT_EQ(c1->getTime().getTime(), 201u);
	c1->logCoreState();
	c1->runSmallStep();	// no imminents, no messages, adv time to 300::1	// 1 only if single test is run
	EXPECT_EQ(c1->getTime().getTime(), 300u);
	c1->logCoreState();
	c1->runSmallStep();	// Imminent = cop : 300,1 , internal transition, time 300:1->500:1, go to terminated, idle
	c1->logCoreState();
	EXPECT_TRUE( !c1->isLive());
	c1->runSmallStep();	// idle, does nothing.
        
        c2->runSmallStep();     // give tlight chance to delete amsgs.

	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

        c1->setLive(false);
        c1->shutDown();
        c2->setLive(false);
        c2->shutDown();

	EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
			!=
		std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());

	};

}

TEST(Optimisticcore, revertedgecases){
	RecordProperty("description", "Revert : test revert in scenario.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
    c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	//c2->printSchedulerState();

	tracers->startTrace();

	// c1: has policeman
	// c2: has trafficlight

	c2->runSmallStep();
	c2->runSmallStep();
	c2->runSmallStep();
	c2->runSmallStep();
	c2->runSmallStep();
	c2->logCoreState();	// C2 is at 228, previous state 178:2
	c1->runSmallStep();	// C1 is now at 200
	c1->logCoreState();
	c1->runSmallStep();	// C1 : sends message to C2, triggering revert.
	c1->logCoreState();
        n_tlocal::setRevert(false);
	/// Revert is triggered, light is rescheduled @228, but then wiped (correctly).
        n_tlocal::setRevert(false);
	c2->runSmallStep();	// C2 : time is 200:1 (reverted), no scheduled models @200
	c2->logCoreState();
	/// c2 remains in zombiestate
	c2->runSmallStep();
	c2->logCoreState();
	EXPECT_EQ(c2->getZombieRounds(), 2u);
	c1->runSmallStep();	// C1 time:300->500, send message to light, state idle
	c1->logCoreState();
	EXPECT_EQ(c1->getTime().getTime(), 500u);
	c2->runSmallStep();	// C2 receives msg, does nothing (nothing to do @200), advances to 300
	c2->logCoreState();
	EXPECT_EQ(c2->getTime().getTime(), 300u);
	EXPECT_EQ(c2->getZombieRounds(), 0u);
	c2->runSmallStep();	// C2 finally awakens, light receives message, core terminates @360.
	c2->logCoreState();

	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

    c1->setLive(false);
    c1->shutDown();
    c2->setLive(false);
    c2->shutDown();

	EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
			!=
		std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());

	};

}

TEST(Optimisticcore, revertoffbyone){
	RecordProperty("description", "Test revert in beginstate.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	t_atomicmodelptr police = std::dynamic_pointer_cast<AtomicModel_impl>(m->getComponents()[0]);
	t_atomicmodelptr light = std::dynamic_pointer_cast<AtomicModel_impl>(m->getComponents()[1]);
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
    c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	//c2->printSchedulerState();

	tracers->startTrace();

	// c1: has policeman
	// c2: has trafficlight
        
        

	c2->runSmallStep();
	c2->logCoreState();
	c2->runSmallStep();
	c2->logCoreState();
        n_tlocal::setRevert(false);
	c2->revert(t_timestamp(57,0));	// Go back to first state, try to crash.
        n_tlocal::setRevert(false);
	EXPECT_EQ(c2->getTime().getTime(), 57u);
	c2->runSmallStep();		// first scheduled is 58:2, time == 57:0, does nothing but increase time.
	EXPECT_EQ(c2->getTime().getTime(), 58u);
	c2->runSmallStep();		// Fires light @ 58:2, advances to 108.
	EXPECT_EQ(c2->getTime().getTime(), 108u);
	/// Next simulate what happens if light gets a confluent transition, combined with a revert.
	/// 108::0 < 108::2, forces revert.
	t_msgptr msg = createRawObject<SpecializedMessage<std::string>>(police->getUUID(), light->getUUID(), t_timestamp(108, 0), 0u, 0u, "toManual");
	msg->paint(MessageColor::WHITE);
	network->acceptMessage(msg);
	c2->runSmallStep();
	c2->logCoreState();
	EXPECT_EQ(c2->getTime().getTime(), 109u);// Model switched to oo, so time will advance with epsilon for one round.
	EXPECT_EQ(c2->getZombieRounds(), 1u);	// so zombie round = 1;
	c2->runSmallStep();
	c2->logCoreState();
	EXPECT_EQ(c2->getTime().getTime(), 109u);// Model switched to oo, so time will advance with epsilon for one round.
	EXPECT_EQ(c2->getZombieRounds(), 2u);	// so zombie round = 2;


	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

    c1->setLive(false);
    c1->shutDown();
    c2->setLive(false);
    c2->shutDown();

	EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
			!=
		std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());

        delete msg;
	}
}


TEST(Optimisticcore, revertstress){
        // Valgrind clear
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	t_atomicmodelptr police = std::dynamic_pointer_cast<AtomicModel_impl>(m->getComponents()[0]);
	t_atomicmodelptr light = std::dynamic_pointer_cast<AtomicModel_impl>(m->getComponents()[1]);
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
    c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	tracers->startTrace();
	// c1: has policeman
	// c2: has trafficlight

	c2->runSmallStep();
	c2->logCoreState();
	c2->runSmallStep();
	c2->logCoreState();
        n_tlocal::setRevert(false);
	c2->revert(t_timestamp(57,0));	// Go back to first state, try to crash.
        n_tlocal::setRevert(false);
	EXPECT_EQ(c2->getTime().getTime(), 57u);
	c2->runSmallStep();		// first scheduled is 58:2, time == 57:0, does nothing but increase time.
	EXPECT_EQ(c2->getTime().getTime(), 58u);
	c2->runSmallStep();		// Fires light @ 58:2, advances to 108.
	EXPECT_EQ(c2->getTime().getTime(), 108u);
	/// Next simulate what happens if light gets a confluent transition, combined with a double revert.
	t_msgptr msg = createRawObject<SpecializedMessage<std::string>>(police->getUUID(), light->getUUID(), t_timestamp(101, 0), 0u,0u, "toManual");
	msg->paint(MessageColor::WHITE);
	network->acceptMessage(msg);
	t_msgptr msglater = createRawObject<SpecializedMessage<std::string>>(police->getUUID(), light->getUUID(), t_timestamp(100, 0), 0u,0u, "toManual");
	msglater->paint(MessageColor::WHITE);
	network->acceptMessage(msglater);
	c2->runSmallStep();
	c2->logCoreState();
	EXPECT_EQ(c2->getTime().getTime(), 101u);// Model switched to oo, so time will not advance.
	EXPECT_EQ(c2->getZombieRounds(), 0u);	// but still have another message to handle @ 101.


	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

    c1->setLive(false);
    c1->shutDown();
    c2->setLive(false);
    c2->shutDown();

	EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
			!=
		std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());

        delete msglater;
        delete msg;
	}
}

TEST(Optimisticcore, revert_antimessaging){    
        // Valgrind clear
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();

	c2->setTracers(tracers);
	c2->init();
    c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	tracers->startTrace();
	// c1: has policeman
	// c2: has trafficlight
	c1->runSmallStep();
	c1->runSmallStep();
	c1->printSchedulerState();
	EXPECT_TRUE(c1->getTime().getTime()==300);	// Core 1 has sent 1 message, trafficlight is still at 0.
        n_tlocal::setRevert(false);
	c1->revert(t_timestamp(32,0));
        n_tlocal::setRevert(false);
	c1->runSmallStep();	// Time goes from 32 -> 200 (first scheduled).
	EXPECT_TRUE(c1->getTime().getTime()==200);
	EXPECT_TRUE(c2->existTransientMessage());
	c2->runSmallStep();	// Trafficlight requests messages, sees message + antimessage
	// It first queues the message, then sees the next (anti) , and annihilates it.
	EXPECT_TRUE(c2->getTime().getTime()==58);
	EXPECT_FALSE(c2->existTransientMessage());


	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

        c1->setLive(false);
        c1->shutDown();
        c2->setLive(false);
        c2->shutDown();

	EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
			!=
		std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());

	}
}


TEST(Optimisticcore, GVT){
	RecordProperty("description", "Manually run GVT.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, 2);
	coreMap.push_back(c1);
	coreMap.push_back(c2);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
    c2->initThread();
	c2->setTerminationTime(endTime);
	c2->setLive(true);
	c2->logCoreState();
	tracers->startTrace();
	// c1: has policeman
	// c2: has trafficlight

	c2->runSmallStep();
	c2->logCoreState();
	c2->runSmallStep();
	c2->logCoreState();	// C2 @108
	c1->runSmallStep();	// C1
	c1->logCoreState(); 	// C1 @200, GVT = 108.
	std::atomic<bool> rungvt(true);
	n_control::runGVT(ctrl, rungvt);
	EXPECT_EQ(c1->getGVT().getTime(), 108u);
	EXPECT_EQ(c1->getGVT(), c2->getGVT());
	EXPECT_EQ(c1->getColor(), MessageColor::WHITE);
	EXPECT_EQ(c2->getColor(), MessageColor::WHITE);
			EXPECT_TRUE(std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[0])->getCorenumber()
				!=
			std::static_pointer_cast<AtomicModel_impl>(m->getComponents()[1])->getCorenumber());


		    c1->setLive(false);
		    c1->shutDown();
		    c2->setLive(false);
		    c2->shutDown();
	}
}


TEST(Conservativecore, Abstract){
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	using namespace n_examples_abstract_c;
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::vector<t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);

        t_eotvector eotvector = createObject<SharedAtomic<t_timestamp::t_time>>(2, 0u);
        t_timevector timevector = createObject<SharedAtomic<t_timestamp::t_time>>(2+1, std::numeric_limits<t_timestamp::t_time>::max());
        timevector->set(timevector->size()-1, 0u);
	auto c0 = createObject<Conservativecore>(network, 0, 2, eotvector, timevector);
	auto c1 = createObject<Conservativecore>(network, 1, 2, eotvector, timevector);
	coreMap.push_back(c0);
	coreMap.push_back(c1);

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setSimType(CONSERVATIVE);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<ModelC>("modelC");
	ctrl.addModel(m);
	c0->setTracers(tracers);
	c0->init();  // LA = oo
    c0->initThread();
	c0->setTerminationTime(endTime);
	c0->setLive(true);
	c0->logCoreState();
	EXPECT_EQ(c0->getCoreID(),0u);

	c1->setTracers(tracers);
	c1->init();  // LA = 30
    c1->initThread();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	/**
	 * C0 : A, influence map = {}
	 * C1 : B  influence map = {0}
	 * initial lookahead 0 = infinity
	 * initial lookahead 1 = 30
	 */
	LOG_INFO("1-------------------------------------------------");
        // Stalled time=0, eit=0
	c0->runSmallStep();                   
        // EOT = 10, EIT=oo                  
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), (10u));
        EXPECT_EQ(c0->getTime().getTime(), 10u);
        // Stalled time=0, eit=0
        c1->runSmallStep();
        EXPECT_EQ(c1->getEit(), (10u));
        
        LOG_INFO("2-------------------------------------------------");
        // C0 is no longer stalled, eit == oo.
        c0->runSmallStep();       
        EXPECT_EQ(c0->getTime().getTime(), 20u);
        
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), (20u));            
        
        c1->runSmallStep();                             
        EXPECT_EQ(c1->getTime(), 20u);
        EXPECT_EQ(c1->getEit(), (20u));
        LOG_INFO("3-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 0->1
        EXPECT_EQ(c0->getTime().getTime(), 30u);
        
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), (30u));            
        
        c1->runSmallStep();                             
        EXPECT_EQ(c1->getTime(), 30u);  // 30, not 20 Nothing to do -> new time = inf, but we have a non-inf LA, so at least that point is safe.
        EXPECT_EQ(c1->getEit(), (30u));
        
        LOG_INFO("4-------------------------------------------------");
        
        
        c0->runSmallStep();                   // Model A 1->2
        EXPECT_EQ(c0->getTime().getTime(), 40u);
        
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), 31u);            // Eot is updated before time is advanced. A message has been sent, so oldtime+eps = eot.
        
        c1->runSmallStep();                   // Model B 0->1          
        EXPECT_EQ(c1->getTime().getTime(), 31u);
        EXPECT_EQ(c1->getEit(), 31u);
        
        LOG_INFO("5-------------------------------------------------");
        
	c0->runSmallStep();                   // Model A 3->4
        EXPECT_EQ(c0->getTime().getTime(), 50u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), 50u);            // Next event = imminent @50, so change eot from 30->50
        
        c1->runSmallStep();                   // Model B 2->3 
        EXPECT_EQ(c1->getTime().getTime(), 40u);
        EXPECT_EQ(c1->getEit(), 50u);
        LOG_INFO("7-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 4->5
        EXPECT_EQ(c0->getTime().getTime(), 60u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), 60u);            
        
        c1->runSmallStep();                   // Model B 3->4
        /**
         * Time : 30->40
         * Transition B:2->3 external
         * Lookahead treshold reached : update La from 30 to 60 by asking state 3
         */
        EXPECT_EQ(c1->getTime().getTime(), 50u);
        EXPECT_EQ(c1->getEit(), 60u);
        LOG_INFO("8-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 5->6
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_EQ(eotvector->get(0), 61u);            
        
        c1->runSmallStep();                   // Model B 4->5
        EXPECT_EQ(c1->getTime().getTime(), 60u);
        EXPECT_EQ(c1->getEit(), 61u);
        LOG_INFO("9-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 6->7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_TRUE(isInfinity(t_timestamp(eotvector->get(0),0u)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 5->6
        EXPECT_EQ(c1->getTime().getTime(), 70u);
        EXPECT_EQ(c1->getEit(), t_timestamp::MAXTIME);
        LOG_INFO("10-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A @7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_TRUE(isInfinity(t_timestamp(eotvector->get(0),0u)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 5->6
        // Lookahead has expired @60, but 6 returns inf as lookahead, so we set that.
        EXPECT_EQ(c1->getTime().getTime(), 70u);
        EXPECT_EQ(c1->getEit(), t_timestamp::MAXTIME);
        
        LOG_INFO("11-------------------------------------------------");
        
        c0->runSmallStep();                   // Model A @7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_EQ(c0->getEit(), t_timestamp::MAXTIME);
        EXPECT_TRUE(isInfinity(t_timestamp(eotvector->get(0),0u)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 6->7 finished
        EXPECT_EQ(c1->getTime().getTime(), 70u);
        EXPECT_EQ(c1->getEit(), t_timestamp::MAXTIME);
        tracers->startTrace();
        c0->shutDown();
        c1->shutDown();
	}
}


TEST(Conservativecore, Deadlock){
        using n_examples_deadlock::Pingset;
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
                CoutRedirect myRedirect(filestream);
                using namespace n_examples_abstract_c;
                auto tracers = createObject<n_tracers::t_tracerset>();

                t_networkptr network = createObject<Network>(3);
                std::vector<t_coreptr> coreMap;
                std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(3);
                
                t_eotvector eotvector = createObject<SharedAtomic<t_timestamp::t_time>>(3, 0u);
                t_timevector timevector = createObject<SharedAtomic<t_timestamp::t_time>>(3+1, std::numeric_limits<t_timestamp::t_time>::max());
                timevector->set(timevector->size()-1, 0u);
                auto c0 = createObject<Conservativecore>(network, 0, 3, eotvector, timevector);
                auto c1 = createObject<Conservativecore>(network, 1, 3, eotvector, timevector);
                auto c2 = createObject<Conservativecore>(network, 2, 3, eotvector, timevector);
                coreMap.push_back(c0);
                coreMap.push_back(c1);
                coreMap.push_back(c2);

                t_timestamp endTime(360, 0);

                n_control::Controller ctrl("testController", coreMap, allocator, tracers);
                ctrl.setSimType(CONSERVATIVE);
                ctrl.setTerminationTime(endTime);

                t_coupledmodelptr m = createObject<Pingset>("feeling_lucky");
                ctrl.addModel(m);
                
                c0->setTracers(tracers);
                c0->init();
                c0->initThread();
                c0->setTerminationTime(endTime);
                c0->setLive(true);
                c0->logCoreState();
                
                c1->setTracers(tracers);
                c1->init();  
                c1->initThread();
                c1->setTerminationTime(endTime);
                c1->setLive(true);
                c1->logCoreState();
                
                c2->setTracers(tracers);
                c2->init();
                c2->initThread();
                c2->setTerminationTime(endTime);
                c2->setLive(true);
                c2->logCoreState();
                
                // Begin in processing state scheduled internal @ 10
                // Eot = 0, Eit=0, time=0
                LOG_INFO("1-------------------------------------------------");
                c0->runSmallStep();     // Stalled round, check if core 1 has passed 0 ? , fails, mark nulltime=0
                c1->runSmallStep();     // Same, but see that 0 has passed 0 as time, so simulation IS safe.
                c2->runSmallStep();     // Same as above
                // Eot becomes 10 (int), Eit moves to 10.
                EXPECT_EQ(c0->getTime().getTime(), 0u);
                EXPECT_EQ(c1->getTime().getTime(), 10u);
                EXPECT_EQ(c2->getTime().getTime(), 10u);
                
                for(size_t i = 0; i<3; ++i){
                        EXPECT_EQ(timevector->get(i), 0u); // 0 round is safe
                        EXPECT_EQ(eotvector->get(i), 10u); // 10 round is next expected
                }
                
                
                LOG_INFO("2-------------------------------------------------");
                
               
                c0->runSmallStep();     // C0 sees that others have nulltime of 0, so simulate to 10. 
                c1->runSmallStep();     // C1 : generate output @10, but C0 is behind, wait.
                c2->runSmallStep();     // C2 : C1 has done all @10, so we're safe.
                
                EXPECT_EQ(c0->getTime().getTime(), 10u);
                EXPECT_EQ(c1->getTime().getTime(), 10u);
                EXPECT_EQ(c2->getTime().getTime(), 10u);
                EXPECT_EQ(timevector->get(0), 0u); // 0 round is safe, 10 has not run yet
                EXPECT_EQ(eotvector->get(0), 10u); // 10 round is next expected
                
                
                EXPECT_EQ(timevector->get(1), 10u); // Core 0 @ 0, so can't advance.
                EXPECT_EQ(eotvector->get(1), 10u); // Since we can't advance, stay on 10 (imminent==10, msg=11)
                EXPECT_EQ(timevector->get(2), 10u); // Core 1 has passed round 10, do a full transition but stay @eit
                EXPECT_EQ(eotvector->get(2), 11u); // Update eot to 11 since we have sent a message and did a full round.
                
                LOG_INFO("3-------------------------------------------------");
                
                c0->runSmallStep(); // Transition @ 10
                c1->runSmallStep(); 
                c2->runSmallStep();
                
                EXPECT_EQ(c0->getTime().getTime(), 11u); // Eot_2 == 11, so [11 is safe.
                EXPECT_EQ(c1->getTime().getTime(), 11u); // 
                EXPECT_EQ(c2->getTime().getTime(), 11u); //
                
                
                EXPECT_EQ(timevector->get(0), 10u); 
                EXPECT_EQ(eotvector->get(0), 11u); 
                EXPECT_EQ(timevector->get(1), 10u); 
                EXPECT_EQ(eotvector->get(1), 11u); 
                EXPECT_EQ(timevector->get(2), 10u); 
                EXPECT_EQ(eotvector->get(2), 11u); 
                
                c0->updateDGVT();
                EXPECT_EQ(c0->getDGVT(), c1->getDGVT());
                EXPECT_EQ(c2->getDGVT(), c1->getDGVT());
                EXPECT_EQ(c2->getDGVT(), 9u);
                
                LOG_INFO("4-------------------------------------------------");
                
               
                c0->runSmallStep();
                c1->runSmallStep();
                c2->runSmallStep();
                
                
                EXPECT_EQ(c0->getTime().getTime(), 11u); 
                EXPECT_EQ(c1->getTime().getTime(), 15u); 
                EXPECT_EQ(c2->getTime().getTime(), 15u); 
                
                EXPECT_EQ(timevector->get(0), 11u); 
                EXPECT_EQ(eotvector->get(0), 15u); 
                EXPECT_EQ(timevector->get(1), 11u); 
                EXPECT_EQ(eotvector->get(1), 15u); 
                EXPECT_EQ(timevector->get(2), 11u); 
                EXPECT_EQ(eotvector->get(2), 15u); 
                c0->shutDown();
                c1->shutDown();
                c2->shutDown();
	}
}
#include <performance/phold/phold.h>

class PHoldAlloc: public n_control::Allocator
{
private:
	std::size_t m_maxn;
	std::size_t m_n;
public:
	PHoldAlloc(): m_maxn(0), m_n(0)
	{

	}
	virtual size_t allocate(const n_model::t_atomicmodelptr&){
		//all models may send to each other. There isn't really an optimal configuration.
		return (m_n++)%coreAmount();
	}

	virtual void allocateAll(const std::vector<n_model::t_atomicmodelptr>& models){
		m_maxn = models.size();
		assert(m_maxn && "Total amount of models can't be zero.");
		for(const n_model::t_atomicmodelptr& ptr: models){
			std::size_t val = allocate(ptr);
			ptr->setCorenumber(val);
			LOG_DEBUG("Assigning model ", ptr->getName(), " to core ", val);
		}
	}
};

TEST(Conservativecore, PHOLD){
        n_control::ControllerConfig conf;
	conf.m_name = "PHOLD";
	conf.m_simType = n_control::SimType::CONSERVATIVE;
	conf.m_coreAmount = 2;
	conf.m_saveInterval = 0;
	conf.m_allocator = n_tools::createObject<PHoldAlloc>();

	auto ctrl = conf.createController();
	t_timestamp endTime(100, 0);
	ctrl->setTerminationTime(endTime);

	t_coupledmodelptr d = n_tools::createObject<n_benchmarks_phold::PHOLD>(1, 2, 1,
	        10);
	ctrl->addModel(d);
        
	Core* zero = ctrl->getCore(0);
        zero->init();
        zero->initThread();
        zero->setTerminationTime(endTime);
        zero->setLive(true);
        Core* one = ctrl->getCore(1);
        one->init();
        one->initThread();
        one->setTerminationTime(endTime);
        one->setLive(true);
        
        // Zero : @16 first event, One : @14 first event
        // Zero <> One, so for each step up to 4 rounds are needed.
        // Special cases : 1->2 @ 14, goes offline. Eot takes over role of time.
        // checkNullrelease() added to updateEOT to preserve >= eot values. @see devstone cpdevs.
        for(size_t i = 0; i<40; ++i){
                LOG_DEBUG("Round ----------------------------- ", i);
                zero->runSmallStep();
                one->runSmallStep();
        }
        EXPECT_TRUE(zero->getTime() > 17u);
        EXPECT_TRUE(one->getTime() > 17u);
	std::ofstream filestream("./phold.txt");
	n_tools::CoutRedirect myRedirect(filestream);
	ctrl->printStats(std::cout);
	d->printStats(std::cout);
        zero->shutDown();
        one->shutDown();
}
