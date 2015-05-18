/*
 * networktest.cpp
 *
 *  Created on: 9 Mar 2015
 *      Author: Ben Cardoen
 */

#include <gtest/gtest.h>
#include "timestamp.h"
#include "objectfactory.h"
#include "trafficlight.h"
#include "modelc.h"
#include "trafficlightc.h"
#include "policemanc.h"
#include "controller.h"
#include "conservativecore.h"
#include "simpleallocator.h"
#include "tracers.h"
#include "coutredirect.h"
#include "trafficsystemc.h"
#include "controllerconfig.h"
#include "controller.h"
#include "compare.h"
#include <sstream>
#include <unordered_set>
#include <thread>
#include <sstream>
#include <chrono>

using namespace n_model;
using namespace n_tools;
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

enum class ThreadSignal{ISWAITING, SHOULDWAIT, ISFINISHED, FREE};

void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid, std::vector<ThreadSignal>& threadsignal,
        std::mutex& vectorlock, std::size_t turns, const t_coreptr& core)
{
	/// A predicate is needed to refreeze the thread if gets a spurious awakening.
	auto predicate = [&]()->bool {
		std::lock_guard<std::mutex > lv(vectorlock);
		return not (threadsignal[myid]==ThreadSignal::ISWAITING);
	};
	for (size_t i = 0; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop
		{	// If another thread has finished, main will flag us down, we need to stop as well.
			std::lock_guard<std::mutex> signallock(vectorlock);
			if(threadsignal[myid] == ThreadSignal::ISFINISHED){
				core->setLive(false);
				return;
			}
		}

		// Try a simulationstep, if core has terminated, set finished flag, else continue.
		if (core->isLive()) {
			LOG_DEBUG("Thread for core ", core->getCoreID() , " running simstep in round ", i);
			core->runSmallStep();
		}else{
			LOG_DEBUG("Thread for core ", core->getCoreID() , " is finished, setting flag.");
			std::lock_guard<std::mutex> signallock(vectorlock);
			threadsignal[myid] = ThreadSignal::ISFINISHED;
			return;
		}

		// Has Main asked us to wait for the other ??
		bool skip_barrier = false;
		{
			std::lock_guard<std::mutex> signallock(vectorlock);
			// Case 1 : Main has asked us by setting SHOULDWAIT, tell main we're ready waiting.
			if(threadsignal[myid] == ThreadSignal::SHOULDWAIT){
				LOG_DEBUG("Thread for core ", core->getCoreID() , " switching flag to WAITING");
				threadsignal[myid] = ThreadSignal::ISWAITING;
			}
			// Case 2 : We can skip the barrier ahead.
			if(threadsignal[myid] == ThreadSignal::FREE){
				LOG_DEBUG("Thread for core ", core->getCoreID() , " skipping barrier, FREE is set.");
				skip_barrier = true;
			}
		}


		if(skip_barrier){
			continue;
		}else{
			std::unique_lock<std::mutex> mylock(cvlock);
			cv.wait(mylock, predicate);				/// Infinite loop : while(!pred) wait().
		}
		/// We'll get here only if predicate = true (spurious) and/or notifyAll() is called.
	}
}

TEST(Core, threading)
{
	RecordProperty("description", "Multicore threading + signalling. Prototype now in use in controller.");
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	t_coreptr coreone = createObject<n_model::Multicore>(network, 0, loctable, 2 );
	coreone->setTracers(tracers);
	t_coreptr coretwo = createObject<n_model::Multicore>(network, 1, loctable, 2);
	coretwo->setTracers(tracers);
	std::vector<t_coreptr> coreptrs;
	coreptrs.push_back(coreone);
	coreptrs.push_back(coretwo);
	auto tcmodel = createObject<COUPLED_TRAFFICLIGHT>("mylight", 0);
	auto tc2model = createObject<COUPLED_TRAFFICLIGHT>("myotherlight", 0);
	coreone->addModel(tcmodel);
	EXPECT_TRUE(coreone->containsModel("mylight"));

	t_timestamp endtime(2000,0);
	coreone->setTerminationTime(endtime);
	coretwo->addModel(tc2model);
	EXPECT_TRUE(coretwo->containsModel("myotherlight"));
	coretwo->setTerminationTime(endtime);
	coreone->init();
	coreone->setLive(true);
	coretwo->init();
	coretwo->setLive(true);
	//Make a nr of threads run until a flag is set, have the main thread check (wait) until all threads are sleeping, then release them for the next iteration.
	std::mutex cvlock;
	std::condition_variable cv;
	std::vector<t_coreptr> cores;
	cores.push_back(coreone);
	cores.push_back(coretwo);

	std::vector<std::thread> threads;
	const std::size_t threadcount = 2;
	if (threadcount <= 1) {
		LOG_WARNING("Skipping test, no threads!");
		return;
	}
	const std::size_t rounds = 100;	// Safety, if main thread ever reaches this value, consider it a deadlock.

	std::mutex veclock;	// Lock for vector with signals
	std::vector<ThreadSignal> threadsignal = {ThreadSignal::FREE, ThreadSignal::FREE};

	for (size_t i = 0; i < threadcount; ++i) {
		threads.push_back(
		        std::thread(cvworker, std::ref(cv), std::ref(cvlock), i, std::ref(threadsignal),
		                std::ref(veclock), rounds, std::cref(cores[i])));
	}



	for (std::size_t round = 0; round < rounds; ++round) {
		bool exit_threads = false;
		for(size_t j = 0;j<threadcount; ++j){
			std::lock_guard<std::mutex> lock(veclock);
			if(threadsignal[j]==ThreadSignal::ISFINISHED){
				LOG_INFO("Main :: Thread id ", j, " has finished, flagging down the rest.");
				threadsignal = std::vector<ThreadSignal>(threadcount, ThreadSignal::ISFINISHED);
				exit_threads = true;
				break;
			}
		}
		if(exit_threads){break;}

		bool all_waiting = false;
		while (not all_waiting) {
				std::lock_guard<std::mutex> lock(veclock);
				all_waiting = true;
				for (const auto& tsignal : threadsignal)
				{
					if (tsignal == ThreadSignal::SHOULDWAIT) {	// FREE is ok, ISWAITING is ok, SHOULDWAIT is the one that shouldn't be set.
						all_waiting = false;
						break;
					}
				}
		}
		{/// This section is only threadsafe if you have set all threads to SHOULDWAIT
			;
		}/// End threadsafe section
		/// Revive threads, first toggle predicate, then release threads (reverse order will deadlock).
		{
			std::lock_guard<std::mutex> lock(veclock);
			for (size_t i = 0; i < threadsignal.size(); ++i) {
				if(threadsignal[i]!= ThreadSignal::ISFINISHED){
					if(round == 4 || round == 9){				// Signal interrupt, threads will stop before the barrier next time
						//LOG_DEBUG("Main : threads will wait next round", round);
						threadsignal[i] = ThreadSignal::SHOULDWAIT;
					}else{
						//LOG_DEBUG("Main : threads can skip next round", round);
						threadsignal[i] = ThreadSignal::FREE;
					}
				}else{
					LOG_DEBUG("Main : seeing finished thread with id " , i);
				}
			}
		}
		cv.notify_all();// End of a round, it's possible some threads are already running (spurious), release all explicitly. Any FREE threads don't even hit the barrier.
	}
	for (auto& t : threads) {
		t.join();
	}
	// Finally, dump trace buffers.
	n_tracers::traceUntil(t_timestamp::infinity());
	EXPECT_FALSE(coreone->isLive());
	EXPECT_FALSE(coretwo->isLive());
	EXPECT_TRUE(coreone->getTime()>= endtime || coretwo->getTime()>= endtime);
}

TEST(Multicore, revert){
	RecordProperty("description", "Revert/timewarp basic tests.");
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	t_coreptr coreone = createObject<n_model::Multicore>(network, 0, loctable, 2 );
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


TEST(Multicore, revertidle){
	RecordProperty("description", "Revert: test if a core can go from idle/terminated back to working.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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

TEST(Multicore, revertedgecases){
	RecordProperty("description", "Revert : test revert in scenario.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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

TEST(Multicore, revertoffbyone){
	RecordProperty("description", "Test revert in beginstate.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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


TEST(Multicore, revertstress){
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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

TEST(Multicore, revert_antimessaging){
	RecordProperty("description", "Try to break revert by doing illogical tests.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	auto tracers = createObject<n_tracers::t_tracerset>();
	CoutRedirect myRedirect(filestream);
	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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


TEST(Multicore, GVT){
	RecordProperty("description", "Manually run GVT.");
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
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


TEST(Conservativecore, GVT){
	std::ofstream filestream(TESTFOLDER "controller/tmp.txt");
	{
	CoutRedirect myRedirect(filestream);
	auto tracers = createObject<n_tracers::t_tracerset>();

	t_networkptr network = createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(2, t_timestamp(0,0));
	auto c0 = createObject<Conservativecore>(network, 0, locTab, 2, eotvector);
	auto c1 = createObject<Conservativecore>(network, 1, locTab, 2, eotvector);
	coreMap[0] = c0;
	coreMap[1] = c1;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
	ctrl.addModel(m);
	c0->setTracers(tracers);
	c0->init();
	c0->setTerminationTime(endTime);
	c0->setLive(true);
	c0->logCoreState();
	EXPECT_EQ(c0->getCoreID(),0u);		// 0 has policeman, influenceemap = empty

	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();

	tracers->startTrace();			// 1 has trafficlight, influenceemap = 0
	c1->runSmallStep();			// Will try to advance, but can't. EOT[0]=0
	c0->runSmallStep();			// synctime 0->200, EOT[0] = 200, EIT=oo
	c1->runSmallStep();			// synctime 0->58   EOT[1] = 58   EIT=200
	EXPECT_EQ(eotvector->get(0).getTime(), 200u);
	EXPECT_EQ(eotvector->get(1).getTime(),58u);
	c1->runSmallStep();			// time 58 -> 108
	c1->runSmallStep();			// time 108 -> 118
	c1->runSmallStep();			// time 118 -> 178
	c1->runSmallStep();			// want to advance from 178->228, but EIT=200, time =200
	c0->runSmallStep();			// EIT = oo, EOT[0]=200, since we have sent a message
	EXPECT_EQ(eotvector->get(0).getTime(), 200u);
	c1->runSmallStep();			// still stuck @200
	EXPECT_EQ(eotvector->get(1).getTime(), 200u);
	std::atomic<bool> rungvt(true);
	n_control::runGVT(ctrl, rungvt);
	EXPECT_EQ(c0->getGVT().getTime(), 200u);
	EXPECT_EQ(c1->getGVT().getTime(), 200u);
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
	auto c0 = createObject<Conservativecore>(network, 0, locTab, 2, eotvector);
	auto c1 = createObject<Conservativecore>(network, 1, locTab, 2, eotvector);
	coreMap[0] = c0;
	coreMap[1] = c1;

	t_timestamp endTime(360, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = createObject<ModelC>("modelC");
	ctrl.addModel(m);
	c0->setTracers(tracers);
	c0->init();
	c0->setTerminationTime(endTime);
	c0->setLive(true);
	c0->logCoreState();
	EXPECT_EQ(c0->getCoreID(),0u);

	c1->setTracers(tracers);
	c1->init();
	c1->setTerminationTime(endTime);
	c1->setLive(true);
	c1->logCoreState();
	/**
	 * C0 : A, influence map = {}
	 * C1 : B  influence map = {0}
	 * initial lookahead 0 = infinity
	 * initial lookahead 1 = 30
	 */
	/// We begin in a null state, cores start from 0-time, but will advance time (eit/eot)
	c0->runSmallStep();
	EXPECT_EQ(eotvector->get(0).getTime(), 10u);		// min of : scheduled=10, eit=inf
	EXPECT_TRUE(isInfinity(c0->getEit()));
	c1->runSmallStep();
	EXPECT_EQ(eotvector->get(1).getTime(), 10u);		// min of : lookahead=30, eit=10
	EXPECT_EQ(c1->getEit().getTime(), 10u);
	// Still in beginstate.

	c0->runSmallStep();
	EXPECT_EQ(eotvector->get(0).getTime(), 20u);		// min of : scheduled=20, eit=inf
	EXPECT_TRUE(isInfinity(c0->getEit()));
	c1->runSmallStep();
	EXPECT_EQ(eotvector->get(1).getTime(), 20u);		// min of : lookahead=30, y=20 (scheduled)
	EXPECT_EQ(c1->getEit().getTime(), 20u);
	// State A:1, B:1


	c0->runSmallStep();
	EXPECT_EQ(eotvector->get(0).getTime(), 30u);
	EXPECT_TRUE(isInfinity(c0->getEit()));
	c1->runSmallStep();
	EXPECT_EQ(eotvector->get(1).getTime(), 30u);		// B : ta=oo @ state 2
	EXPECT_EQ(c1->getEit().getTime(), 30u);
	// State A:2, B:2

	c0->runSmallStep();					// Send Message
	EXPECT_EQ(eotvector->get(0).getTime(), 30u);		// x=infinity, first scheduled = 30, Coretime == 40 !!!
	EXPECT_EQ(c0->getTime().getTime(), 40u);
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	c1->runSmallStep();					// receive message, but do not process it (yet), nexttime=30
	EXPECT_TRUE(isInfinity(eotvector->get(1)));		// 1(B) is longer scheduled, so EOT does not find anything.
	EXPECT_EQ(c1->getEit().getTime(), 30u);			// Eit == 30 based on EOT value of 0
	// State A:3 B:2

	c0->runSmallStep();					//
	EXPECT_EQ(eotvector->get(0).getTime(), 50u);		// x=infinity, first scheduled = 50
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	EXPECT_TRUE(c0->getTime().getTime()==50);
	c1->runSmallStep();					// process pending message, become live again
	EXPECT_EQ(eotvector->get(1).getTime(), 40u);		// min(x=60 (time=30+lookahead 30), y=40 first scheduled)
	EXPECT_EQ(c1->getEit().getTime(), 50u);			// Eit = eot _0
	EXPECT_EQ(c1->getTime().getTime(),40u);
	// A:4, B:3

	c0->runSmallStep();					//
	EXPECT_EQ(eotvector->get(0).getTime(), 60u);		// x=infinity, first scheduled = 60
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	EXPECT_TRUE(c0->getTime().getTime()==60);
	c1->runSmallStep();					//
	EXPECT_EQ(eotvector->get(1).getTime(), 50u);		// min(x=eit=50 + lookahead=20 == 70, y=50 first scheduled)
	EXPECT_EQ(c1->getEit().getTime(), 60u);			// Eit = eot _0
	EXPECT_EQ(c1->getTime().getTime(),50u);			// Time is 50, we can advance safely to 60, but
	// A:5, B:4							// that requires more than 1 round. In parallel B could
								// do this.


	c0->runSmallStep();					// send message
	EXPECT_EQ(eotvector->get(0).getTime(), 60u);		// x=infinity, just sent message @60, so 60,1
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	EXPECT_TRUE(c0->getTime().getTime()==70);		// Time advances to 70 (next scheduled)
	c1->runSmallStep();					// Get message from A, but queue it
								// B AGAIN goes offline : oo
	EXPECT_EQ(eotvector->get(1).getTime(), 70u);		// lookahead = 10, eit=60 == 70
	EXPECT_EQ(c1->getEit().getTime(), 60u);			// Eit = eot _0
	EXPECT_EQ(c1->getTime().getTime(),60u);			// Can forward time to 60 (time of rec'd message
	// A:6, B:5

	c0->runSmallStep();					// new timeadvance=infinity, lookahead =infinity, no msgs
	EXPECT_TRUE(isInfinity(eotvector->get(0)));		// x=infinity, y=infinity
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	EXPECT_TRUE(c0->getTime().getTime()==70);		// Time is now stuck @ 70, we're zombie

	c1->runSmallStep();					// Process message from A, B becomes live again
								// reschedules @60+10, lookahead=inf
	EXPECT_EQ(eotvector->get(1).getTime(), 70u);		// x=inf, y=70 (first scheduled)
	EXPECT_TRUE(isInfinity(c1->getEit()));			// Eit = eot _0
	EXPECT_EQ(c1->getTime().getTime(),70u);			// Time can advance to 70 (next scheduled)
	// A:7, B:6

	c0->runSmallStep();					// new timeadvance=infinity, lookahead =infinity, no msgs
	EXPECT_TRUE(isInfinity(eotvector->get(0)));		// x=infinity, y=infinity
	EXPECT_TRUE(isInfinity(c0->getEit()));			// unchanged
	EXPECT_TRUE(c0->getTime().getTime()==70);		// Time is now stuck @ 70, we're zombie

	c1->runSmallStep();					// Internal transition 6->7 @ B
								// B goes offline (ta=inf), la=oo
	EXPECT_TRUE(isInfinity(eotvector->get(1)));		// x=inf, y=inf
	EXPECT_TRUE(isInfinity(c1->getEit()));			// Eit = eot _0
	EXPECT_EQ(c1->getTime().getTime(),70u);			// Time is stuck
	// A:7, B:7

	/// That's all folks.
	tracers->startTrace();
	}
}
