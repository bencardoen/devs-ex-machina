/*
 * networktest.cpp
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
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
	auto scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::FIBONACCI, false);
	EXPECT_TRUE(scheduler->empty());
	std::stringstream s;
	for (size_t i = 0; i < 100; ++i) {
		s << i;
		std::string name = s.str();
		s.str(std::string(""));
		scheduler->push_back(ModelEntry(name, t_timestamp(i, 0)));
		EXPECT_EQ(scheduler->size(), i + 1);
	}
	std::vector<ModelEntry> imminent;
	ModelEntry token("", t_timestamp(50, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 50u);
	token = ModelEntry("", t_timestamp(100, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 0u);

	// Test if scheduling models at same time is a problem
	ModelEntry origin("Abc", t_timestamp(0));
	ModelEntry duplicate("Bca", t_timestamp(0));
	ModelEntry third("Cab", t_timestamp(0, 1));
	scheduler->push_back(origin);
	scheduler->push_back(duplicate);
	EXPECT_EQ(scheduler->size(), 2u);
	scheduler->push_back(third);
	EXPECT_EQ(scheduler->size(), 3u);
	ModelEntry found = scheduler->pop();
	EXPECT_EQ(found.getName(), "Abc");
	EXPECT_EQ(scheduler->pop().getName(), "Bca");
	EXPECT_EQ(scheduler->pop().getName(), "Cab");
	EXPECT_EQ(scheduler->size(), 0u);
}

TEST(ModelScheduling, BasicOperations)
{
	RecordProperty("description",
	        "Verify that std::hash, std::less and related operators are well defined and execute as expected.");
	ModelEntry me("alone", t_timestamp(0, 0));
	ModelEntry you("home", t_timestamp(0, 0));
	EXPECT_FALSE(me == you);
	std::unordered_set<ModelEntry> set;
	set.insert(me);
	set.insert(you);
	EXPECT_EQ(set.size(), 2u);
	set.clear();
	EXPECT_EQ(set.size(), 0u);
	// This is evil, and is never guaranteed to work.
	// A model entry with the same name is equal no matter what time is set.
	// The alternative allows insertion multiple times (an error).
	// It's written as a test to detect if/when somebody clobbers the logic of the operators in devious ways.
	me = ModelEntry("alone", t_timestamp(1, 0));
	you = ModelEntry("alone", t_timestamp(1, 1));
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
	std::string portname_stub = "model_port";
	t_msgptr mymessage = createObject<Message>("toBen", (0), portname_stub, portname_stub);
	t_atomicmodelptr modelfrom = createObject<ATOMIC_TRAFFICLIGHT>("Amodel");
	t_atomicmodelptr modelto = createObject<ATOMIC_TRAFFICLIGHT>("toBen");
	EXPECT_EQ(modelfrom->getName(), "Amodel");
	c.addModel(modelfrom);
	EXPECT_EQ(c.getModel("Amodel"), modelfrom);
	c.addModel(modelto);
	EXPECT_EQ(c.getModel("toBen"), modelto);
	EXPECT_FALSE(mymessage->getDestinationCore() == 0);
	c.init();
	//c.printSchedulerState();
	c.syncTime();
	EXPECT_EQ(c.getTime().getTime() , t_timestamp(60, 0).getTime());
	auto imminent = c.getImminent();
	EXPECT_EQ(imminent.size(), 2u);
	//for(const auto& el : imminent)	std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	c.syncTime();
	imminent = c.getImminent();
	//for(const auto& el : imminent)		std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	//c.printSchedulerState();
	c.syncTime();
	EXPECT_EQ(imminent.size(), 2u);
}

TEST(DynamicCore, smallStep)
{
	t_coreptr c = createObject<DynamicCore>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
	t_atomicmodelptr modelfrom = createObject<ATOMIC_TRAFFICLIGHT>("Amodel");
	t_atomicmodelptr modelto = createObject<ATOMIC_TRAFFICLIGHT>("toBen");
	c->addModel(modelfrom);
	c->addModel(modelto);
	c->init();
	auto finaltime = c->getTerminationTime();
	EXPECT_TRUE(isInfinity(finaltime));
	c->setTerminationTime(t_timestamp(200, 0));
	finaltime = c->getTerminationTime();
	EXPECT_EQ(finaltime, t_timestamp(200, 0));
	std::vector<t_atomicmodelptr> imms;
	c->getLastImminents(imms);
	EXPECT_EQ(imms.size(), 0u);
	c->setLive(true);
	c->syncTime();
	while(c->isLive()){
		c->runSmallStep();
		c->getLastImminents(imms);
		EXPECT_EQ(imms.size(), 2u);
		if(imms.size()==2){
			EXPECT_EQ(imms[0],modelfrom);
			EXPECT_EQ(imms[1], modelto);
		}
	}
	// This is not how to run a core, but a check of safety blocks.
	c->setLive(true);
	c->setIdle(false);
	c->removeModel("Amodel");
	EXPECT_EQ(c->containsModel("Amodel"), false);
	c->removeModel("toBen");
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
	c->removeModel("Amodel");
}

TEST(Core, Messaging)
{
	RecordProperty("description", "Core simulation steps with term function.");
	t_coreptr c = createObject<Core>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	c->setTracers(tracers);
	t_atomicmodelptr modellight = createObject<COUPLED_TRAFFICLIGHT>("mylight");
	t_atomicmodelptr modelcop = createObject<COUPLED_POLICEMAN>("mycop");
	c->addModel(modellight);
	c->addModel(modelcop);

	c->init();
	auto finaltime = c->getTerminationTime();
	EXPECT_TRUE(isInfinity(finaltime));
	t_timestamp coretimebefore = c->getTime();
	c->setLive(true);
	EXPECT_TRUE(c->isLive() == true);
	t_timestamp timemessagelight(57,0);
	t_timestamp timemessagecop(57,1);
	// Set time slightly before first firing to detect if messagetimestamp can overrule firing.
	c->setTime(t_timestamp(50,0));	// Note that otherwise getTime would be 60 (first transition)
	auto msgtolight = createObject<Message>("mylight", timemessagelight, "dport", "sport", "payload");
	auto msgtocop = createObject<Message>("mycop", timemessagecop, "dport", "sport", "payload");
	std::vector<t_msgptr> messages;
	messages.push_back(msgtolight);
	messages.push_back(msgtocop);
	c->sortMail(messages);
	EXPECT_EQ(c->getFirstMessageTime(), timemessagelight);
	c->syncTime();
	EXPECT_EQ(c->getTime(), timemessagelight);
	EXPECT_FALSE(c->getTime() == timemessagecop);
	std::unordered_map<std::string, std::vector<t_msgptr>> mailbag;
	c->getPendingMail(mailbag);
	EXPECT_EQ(mailbag["mylight"][0], msgtolight);
	EXPECT_EQ(mailbag["mycop"][0], msgtocop);
}

TEST(Optimisticcore, revert){
	RecordProperty("description", "Revert/timewarp basic tests.");
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	t_coreptr coreone = createObject<n_model::Optimisticcore>(network, 0, loctable, 2 );
	coreone->setTracers(tracers);
	auto tcmodel = createObject<COUPLED_TRAFFICLIGHT>("mylight", 0);
	coreone->addModel(tcmodel);
	EXPECT_TRUE(coreone->containsModel("mylight"));
	t_timestamp endtime(2000,0);
	coreone->setTerminationTime(endtime);
	coreone->init();
	coreone->syncTime();
	EXPECT_EQ(coreone->getTime().getTime(), 58u);
	//coreone->printSchedulerState();
	coreone->setLive(true);
	coreone->runSmallStep();
	EXPECT_EQ(coreone->getTime().getTime(), 108u);
	//coreone->printSchedulerState();
	// Trigger revert
	// Setup message state to test all paths
	t_timestamp beforegvt(61,0);
	t_timestamp gvt(62,0);
	t_timestamp aftergvt(63,0);
	t_msgptr msg = createObject<Message>("mylight", beforegvt, "", "");
	msg->setSourceCore(0);
	msg->setDestinationCore(1);
	t_msgptr msggvt = createObject<Message>("mylight", gvt, "", "");
	msggvt->setSourceCore(0);
	msggvt->setDestinationCore(1);
	t_msgptr msgaftergvt = createObject<Message>("mylight", aftergvt, "", "");
	msgaftergvt->setSourceCore(0);
	msgaftergvt->setDestinationCore(1);

	coreone->setGVT(gvt);
	coreone->revert(gvt);		// We were @110, went back to 62

	EXPECT_EQ(coreone->getTime(), 62u);
	EXPECT_EQ(coreone->getTime(), coreone->getGVT());
	coreone->setTime(t_timestamp(67,0));	// need to cheat here, else we won't get the result we're aiming for.
	Message origin = *msgaftergvt;
	t_msgptr antimessage( new Message(origin));
	antimessage->setSourceCore(42);	// Work around error in log.
	antimessage->setAntiMessage(true);
	coreone->receiveMessage(antimessage);		// this triggers a new revert, we were @67, now @63
	EXPECT_EQ(coreone->getTime().getTime(), 63u);
	coreone->runSmallStep();			// does nothing, check that empty transitioning works. (next = 108, time = 62)
	EXPECT_EQ(coreone->getTime().getTime(), 108u);
}


TEST(Optimisticcore, revertidle){
	RecordProperty("description", "Revert: test if a core can go from idle/terminated back to working.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
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
	c1->revert(t_timestamp(201,42));
	// Expecting time = 201,42, revert of model 1x antimessage @300.
	EXPECT_EQ(c1->getTime().getTime(), 201u);
	c1->logCoreState();
	c1->runSmallStep();	// no imminents, no messages, adv time to 300::1	// 1 only if single test is run
	EXPECT_EQ(c1->getTime().getTime(), 300u);
	c1->logCoreState();
	c1->runSmallStep();	// Imminent = cop : 300,1 , internal transition, time 300:1->500:1, go to terminated, idle
	c1->logCoreState();
	EXPECT_TRUE(c1->isIdle() && !c1->isLive());
	c1->runSmallStep();	// idle, does nothing.

	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	};

}

TEST(Optimisticcore, revertedgecases){
	RecordProperty("description", "Revert : test revert in scenario.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
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
	/// Revert is triggered, light is rescheduled @228, but then wiped (correctly).
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

	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	};

}

TEST(Optimisticcore, revertoffbyone){
	RecordProperty("description", "Test revert in beginstate.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
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
	c2->revert(t_timestamp(57,0));	// Go back to first state, try to crash.
	EXPECT_EQ(c2->getTime().getTime(), 57u);
	c2->runSmallStep();		// first scheduled is 58:2, time == 57:0, does nothing but increase time.
	EXPECT_EQ(c2->getTime().getTime(), 58u);
	c2->runSmallStep();		// Fires light @ 58:2, advances to 108.
	EXPECT_EQ(c2->getTime().getTime(), 108u);
	/// Next simulate what happens if light gets a confluent transition, combined with a revert.
	/// 108::0 < 108::2, forces revert.
	t_msgptr msg = createObject<Message>("trafficLight", t_timestamp(108, 0), "trafficLight.INTERRUPT", "policeman.OUT", "toManual");
	msg->setSourceCore(0);
	msg->setDestinationCore(1);
	msg->paint(MessageColor::WHITE);
	network->acceptMessage(msg);
	c2->runSmallStep();
	c2->logCoreState();
	EXPECT_EQ(c2->getTime().getTime(), 108u);// Model switched to oo, so time will not advance.
	EXPECT_EQ(c2->getZombieRounds(), 1u);	// so zombie round = 1;


	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
	n_tracers::waitForTracer();
	tracers->finishTrace();

	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	}
}


TEST(Optimisticcore, revertstress){
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
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
	c2->revert(t_timestamp(57,0));	// Go back to first state, try to crash.
	EXPECT_EQ(c2->getTime().getTime(), 57u);
	c2->runSmallStep();		// first scheduled is 58:2, time == 57:0, does nothing but increase time.
	EXPECT_EQ(c2->getTime().getTime(), 58u);
	c2->runSmallStep();		// Fires light @ 58:2, advances to 108.
	EXPECT_EQ(c2->getTime().getTime(), 108u);
	/// Next simulate what happens if light gets a confluent transition, combined with a double revert.
	t_msgptr msg = createObject<Message>("trafficLight", t_timestamp(101, 0), "trafficLight.INTERRUPT","policeman.OUT", "toManual");
	msg->setSourceCore(0);
	msg->setDestinationCore(1);
	msg->paint(MessageColor::WHITE);
	network->acceptMessage(msg);
	t_msgptr msglater = createObject<Message>("trafficLight", t_timestamp(100, 0), "trafficLight.INTERRUPT","policeman.OUT", "toManual");
	msglater->setSourceCore(0);
	msglater->setDestinationCore(1);
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

	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	}
}

TEST(Optimisticcore, revert_antimessaging){
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();

	c2->setTracers(tracers);
	c2->init();
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
	c1->revert(t_timestamp(32,0));
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

	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	}
}


TEST(Optimisticcore, GVT){
	RecordProperty("description", "Manually run GVT.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(SimType::OPTIMISTIC);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	//c1->printSchedulerState();

	c2->setTracers(tracers);
	c2->init();
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
	EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

	}

}


TEST(Conservativecore, Abstract){
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	using namespace n_examples_abstract_c;
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(2, t_timestamp(0,0));
        t_timevector timevector = createObject<SharedVector<t_timestamp>>(2, t_timestamp::infinity());
	auto c0 = createObject<Conservativecore>(network, 0, locTab, eotvector, timevector);
	auto c1 = createObject<Conservativecore>(network, 1, locTab, eotvector, timevector);
	coreMap[0] = c0;
	coreMap[1] = c1;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setSimType(CONSERVATIVE);
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<ModelC>("modelC");
	ctrl.addModel(m);
	c0->setTracers(tracers);
	c0->init();  // LA = oo
	c0->setTerminationTime(endTime);
	c0->setLive(true);
	c0->logCoreState();
	EXPECT_EQ(c0->getCoreID(),0u);

	c1->setTracers(tracers);
	c1->init();  // LA = 30
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	/**
	 * C0 : A, influence map = {}
	 * C1 : B  influence map = {0}
	 * initial lookahead 0 = infinity
	 * initial lookahead 1 = 30
	 */
	LOG_INFO("--------------------------------------------------");
        // Stalled time=0, eit=0
	c0->runSmallStep();                   
        // EOT = 10, EIT=oo                  
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), (10u));
        // Stalled time=0, eit=0
        c1->runSmallStep();
        EXPECT_EQ(c1->getEit(), (10u));
        LOG_INFO("--------------------------------------------------");
        // C0 is no longer stalled, eit == oo.
        c0->runSmallStep();       // Move time to 10
        EXPECT_EQ(c0->getTime().getTime(), 10u);
        // EOT = 10, EIT=oo                  
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), (10u));            // We have sent a msg at 10, so eot is still 10
        // Normal round, advance to 10 (eit=10).
        // La stays at 30, no output.
        c1->runSmallStep();                             // Nothing to do at 0, move time to 10
        EXPECT_EQ(c1->getTime(), 10u);
        EXPECT_EQ(c1->getEit(), (10u));
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 0->1
        EXPECT_EQ(c0->getTime().getTime(), 20u);
        
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), (20u));            
        
        c1->runSmallStep();                             
        EXPECT_EQ(c1->getTime(), 10u);
        EXPECT_EQ(c1->getEit(), (20u));
        
        LOG_INFO("--------------------------------------------------");
        
        
        c0->runSmallStep();                   // Model A 1->2
        EXPECT_EQ(c0->getTime().getTime(), 30u);
        
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), 30u);            
        
        c1->runSmallStep();                   // Model B 0->1          
        EXPECT_EQ(c1->getTime().getTime(), 20u);
        EXPECT_EQ(c1->getEit(), 30u);
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 2->3
        EXPECT_EQ(c0->getTime().getTime(), 40u);        
        
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), 30u);            // Message sent @30, so EOT=30
        
        c1->runSmallStep();                   // Model B 1->2
        EXPECT_EQ(c1->getTime().getTime(), 30u);
        EXPECT_EQ(c1->getEit(), 30u);
        LOG_INFO("--------------------------------------------------");
        
	c0->runSmallStep();                   // Model A 3->4
        EXPECT_EQ(c0->getTime().getTime(), 50u);
                    
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), 50u);            // Next event = imminent @50, so change eot from 30->50
        
        c1->runSmallStep();                   // Model B 2 @ oo 
        EXPECT_EQ(c1->getTime().getTime(), 30u);
        EXPECT_EQ(c1->getEit(), 50u);
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 4->5
        EXPECT_EQ(c0->getTime().getTime(), 60u);
                    
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), 60u);            
        
        c1->runSmallStep();                   // Model B 2->3
        /**
         * Time : 30->40
         * Transition B:2->3 external
         * Lookahead treshold reached : update La from 30 to 60 by asking state 3
         */
        EXPECT_EQ(c1->getTime().getTime(), 40u);
        EXPECT_EQ(c1->getEit(), 60u);
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 5->6
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_TRUE(isInfinity(c0->getEit()));
        EXPECT_EQ(eotvector->get(0), 60u);            // Have sent message @ 60, so allthough new time = 70, output time is eot.
        
        c1->runSmallStep();                   // Model B 3->4
        EXPECT_EQ(c1->getTime().getTime(), 50u);
        EXPECT_EQ(c1->getEit(), 60u);
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A 6->7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_TRUE( isInfinity(c0->getEit()) );
        EXPECT_TRUE(isInfinity(eotvector->get(0)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 4->5
        EXPECT_EQ(c1->getTime().getTime(), 60u);
        EXPECT_TRUE( isInfinity(c1->getEit()) );
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A @7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_TRUE( isInfinity(c0->getEit()) );
        EXPECT_TRUE(isInfinity(eotvector->get(0)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 5->6
        // Lookahead has expired @60, but 6 returns inf as lookahead, so we set that.
        EXPECT_EQ(c1->getTime().getTime(), 70u);
        EXPECT_TRUE( isInfinity(c1->getEit()) );
        
        LOG_INFO("--------------------------------------------------");
        
        c0->runSmallStep();                   // Model A @7
        EXPECT_EQ(c0->getTime().getTime(), 70u);
                    
        EXPECT_TRUE( isInfinity(c0->getEit()) );
        EXPECT_TRUE(isInfinity(eotvector->get(0)) );            // Eot is now infinity, c0 has nothing to do anymore
        
        c1->runSmallStep();                   // Model B 6->7 finished
        EXPECT_EQ(c1->getTime().getTime(), 70u);
        EXPECT_TRUE( isInfinity(c1->getEit()) );
        tracers->startTrace();
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
                std::unordered_map<std::size_t, t_coreptr> coreMap;
                std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(3);
                std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(3);

                t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(3, t_timestamp(0,0));
                t_timevector timevector = createObject<SharedVector<t_timestamp>>(3, t_timestamp::infinity());
                auto c0 = createObject<Conservativecore>(network, 0, locTab, eotvector, timevector);
                auto c1 = createObject<Conservativecore>(network, 1, locTab, eotvector, timevector);
                auto c2 = createObject<Conservativecore>(network, 2, locTab, eotvector, timevector);
                coreMap[0] = c0;
                coreMap[1] = c1;
                coreMap[2] = c2;

                t_timestamp endTime(360, 0);

                n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
                ctrl.setSimType(CONSERVATIVE);
                ctrl.setTerminationTime(endTime);

                t_coupledmodelptr m = createObject<Pingset>("feeling_lucky");
                ctrl.addModel(m);
                
                c0->setTracers(tracers);
                c0->init();  
                c0->setTerminationTime(endTime);
                c0->setLive(true);
                c0->logCoreState();
                
                c1->setTracers(tracers);
                c1->init();  
                c1->setTerminationTime(endTime);
                c1->setLive(true);
                c1->logCoreState();
                
                c2->setTracers(tracers);
                c2->init();  
                c2->setTerminationTime(endTime);
                c2->setLive(true);
                c2->logCoreState();
                
                c0->runSmallStep();
                c1->runSmallStep();
                c2->runSmallStep();
                
                
                c0->runSmallStep();
                c1->runSmallStep();
                c2->runSmallStep();
                
                c0->runSmallStep();
                c1->runSmallStep();
                c2->runSmallStep();
                //tracers->startTrace();
	}
}
