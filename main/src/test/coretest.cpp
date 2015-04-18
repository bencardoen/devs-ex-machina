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
#include "dynamiccore.h"
#include "trafficlight.h"
#include "trafficlightc.h"
#include "policemanc.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
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
	auto scheduler = n_tools::SchedulerFactory<ModelEntry>::makeScheduler(n_tools::Storage::BINOMIAL, false);
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
	EXPECT_EQ(scheduler->size(), 50);
	token = ModelEntry("", t_timestamp(100, 0));
	scheduler->unschedule_until(imminent, token);
	EXPECT_EQ(scheduler->size(), 0);

	// Test if scheduling models at same time is a problem
	ModelEntry origin("Abc", t_timestamp(0));
	ModelEntry duplicate("Bca", t_timestamp(0));
	ModelEntry third("Cab", t_timestamp(0, 1));
	scheduler->push_back(origin);
	scheduler->push_back(duplicate);
	EXPECT_EQ(scheduler->size(), 2);
	scheduler->push_back(third);
	EXPECT_EQ(scheduler->size(), 3);
	ModelEntry found = scheduler->pop();
	EXPECT_EQ(found.getName(), "Abc");
	EXPECT_EQ(scheduler->pop().getName(), "Bca");
	EXPECT_EQ(scheduler->pop().getName(), "Cab");
	EXPECT_EQ(scheduler->size(), 0);
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
	EXPECT_EQ(set.size(), 2);
	set.clear();
	EXPECT_EQ(set.size(), 0);
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
	EXPECT_EQ(set.size(), 1);
}

TEST(Core, CoreFlow)
{
	RecordProperty("description",
	        "Verify that Core can (re)schedule models, model lookup is working and core can advance in time.");
	using n_network::Message;
	Core c; // single core.
	EXPECT_EQ(c.getCoreID(), 0);
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
	EXPECT_TRUE(c.getTime() == t_timestamp(60));
	auto imminent = c.getImminent();
	EXPECT_EQ(imminent.size(), 2);
	//for(const auto& el : imminent)	std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	c.syncTime();
	imminent = c.getImminent();
	//for(const auto& el : imminent)		std::cout << el << std::endl;
	c.rescheduleImminent(imminent);
	//c.printSchedulerState();
	c.syncTime();
	EXPECT_EQ(imminent.size(), 2);
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
	EXPECT_EQ(finaltime, t_timestamp::infinity());
	c->setTerminationTime(t_timestamp(200, 0));
	finaltime = c->getTerminationTime();
	EXPECT_EQ(finaltime, t_timestamp(200, 0));
	std::vector<t_atomicmodelptr> imms;
	c->getLastImminents(imms);
	EXPECT_EQ(imms.size(), 0);
	c->setLive(true);
	while(c->isLive()){
		c->runSmallStep();
		c->getLastImminents(imms);
		EXPECT_EQ(imms.size(), 2);
		EXPECT_EQ(imms[0],modelfrom);
		EXPECT_EQ(imms[1], modelto);
	}
	// This is not how to run a core, but a check of safety blocks.
	c->setLive(true);
	c->removeModel("Amodel");
	EXPECT_EQ(c->containsModel("Amodel"), false);
	c->removeModel("toBen");
	EXPECT_EQ(c->containsModel("toBen"), false);
	c->runSmallStep();
	c->getLastImminents(imms);
	EXPECT_EQ(imms.size(), 0);
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
	EXPECT_EQ(finaltime, t_timestamp::infinity());
	c->setTerminationFunction(createObject<termfun>());

	t_timestamp coretimebefore = c->getTime();
	// Switch 'on' Core.
	c->setLive(true);
	EXPECT_TRUE(c->terminated() == false);
	EXPECT_TRUE(c->isLive() == true);

	// Run simulation.
	c->runSmallStep();
	t_timestamp coretimeafter = c->getTime();
	EXPECT_TRUE(coretimebefore < coretimeafter);
	EXPECT_TRUE(c->terminated() == true);
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
	EXPECT_EQ(finaltime, t_timestamp::infinity());
	t_timestamp coretimebefore = c->getTime();
	c->setLive(true);
	EXPECT_TRUE(c->terminated() == false);
	EXPECT_TRUE(c->isLive() == true);
	t_timestamp timemessagelight(59,0);
	t_timestamp timemessagecop(59,1);
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

void core_worker(const t_coreptr& core)
{
	core->setLive(true);
	core->init();

	while (core->isLive()) {
		core->runSmallStep();
	}
}

TEST(Core, multicoresafe)
{
	using n_network::Network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	std::mutex vlock;
	t_coreptr coreone = createObject<n_model::Multicore>(network, 1, loctable, vlock,2);
	coreone->setTracers(tracers);
	t_coreptr coretwo = createObject<n_model::Multicore>(network, 0, loctable, vlock,2);
	coretwo->setTracers(tracers);
	std::vector<t_coreptr> coreptrs;
	coreptrs.push_back(coreone);
	coreptrs.push_back(coretwo);
	auto tcmodel = createObject<COUPLED_TRAFFICLIGHT>("mylight", 0);
	auto tc2model = createObject<COUPLED_TRAFFICLIGHT>("myotherlight", 0);
	coreone->addModel(tcmodel);
	EXPECT_TRUE(coreone->containsModel("mylight"));
	coreone->setTerminationTime(t_timestamp(20000, 0));
	coretwo->addModel(tc2model);
	EXPECT_TRUE(coretwo->containsModel("myotherlight"));
	coretwo->setTerminationTime(t_timestamp(20000, 0));
	//const size_t cores = std::thread::hardware_concurrency();
	const size_t threadcount = 2;
	if (threadcount <= 1) {
		LOG_WARNING("Skipping test, no threads!");
		return;
	}
	std::vector<std::thread> workers;
	for (size_t i = 0; i < threadcount; ++i) {
		workers.push_back(std::thread(core_worker, std::cref(coreptrs[i])));
	}
	for (auto& worker : workers) {
		worker.join();
	}
	coreone->signalTracersFlush();
	coretwo->signalTracersFlush();
	EXPECT_TRUE(coreone->getTime() >= coreone->getTerminationTime());
	EXPECT_TRUE(coretwo->getTime() >= coretwo->getTerminationTime());
	EXPECT_TRUE(not coreone->isLive());
	EXPECT_TRUE(not coretwo->isLive());
	coreone->clearModels();
	coretwo->clearModels();
	EXPECT_FALSE(coreone->containsModel("mylight"));
	EXPECT_FALSE(coretwo->containsModel("myotherlight"));
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
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	std::mutex vlock;
	t_coreptr coreone = createObject<n_model::Multicore>(network, 0, loctable, vlock,2 );
	coreone->setTracers(tracers);
	t_coreptr coretwo = createObject<n_model::Multicore>(network, 1, loctable, vlock, 2);
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
	const std::size_t threadcount = 2;			//TODO replace with concurrency
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

TEST(Multicore, GVTfunctions){
	using namespace n_network;
	using n_control::t_location_tableptr;
	using n_control::LocationTable;
	t_networkptr network = createObject<Network>(2);
	t_location_tableptr loctable = createObject<LocationTable>(2);
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	std::mutex vlock;
	t_coreptr coreone = createObject<n_model::Multicore>(network, 0, loctable, vlock,2 );
	coreone->setTracers(tracers);
	auto tcmodel = createObject<COUPLED_TRAFFICLIGHT>("mylight", 0);
	coreone->addModel(tcmodel);
	EXPECT_TRUE(coreone->containsModel("mylight"));
	t_timestamp endtime(2000,0);
	coreone->setTerminationTime(endtime);
	coreone->init();
	coreone->setLive(true);
	coreone->runSmallStep();
	coreone->getTime();
	t_timestamp beforegvt(0,0);
	t_timestamp gvt(1,0);
	t_timestamp aftergvt(2,0);
	t_msgptr msg = createObject<Message>("mylight", beforegvt, "", "");
	msg->setSourceCore(0);
	msg->setDestinationCore(1);
	t_msgptr msggvt = createObject<Message>("mylight", gvt, "", "");
	msggvt->setSourceCore(0);
	msggvt->setDestinationCore(1);
	t_msgptr msgaftergvt = createObject<Message>("mylight", aftergvt, "", "");
	msgaftergvt->setSourceCore(0);
	msgaftergvt->setDestinationCore(1);
	std::vector<t_msgptr> processed = {msg, msggvt, msgaftergvt};
	coreone->markProcessed(processed);
	for(const auto& msg : processed){
		coreone->markMessageStored(msg);
	}
	coreone->setGVT(gvt);
	//coreone->printSchedulerState();
	//coreone->printPendingMessages();
	coreone->revert(gvt);
	coreone->printSchedulerState();
	coreone->printPendingMessages();
	EXPECT_EQ(coreone->getTime(), coreone->getGVT());
	coreone->setTime(t_timestamp(3,0));	// need to cheat here, else we won't get the result we're aiming for.
	Message origin = *msgaftergvt;
	t_msgptr antimessage( new Message(origin));
	antimessage->setAntiMessage(true);
	coreone->receiveMessage(antimessage);
}


TEST(Multicore, GVT){
	std::atomic<bool> run(true);
	std::thread gvt(&n_model::calculateGVT, 500, std::ref(run));
	for(size_t i = 0; i<10000000; ++i){
	}
	run.store(false);
	gvt.join();
}
