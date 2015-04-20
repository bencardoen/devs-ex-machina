/*
 * controllertest.cpp
 *
 *  Created on: 26 Mar 2015
 *      Author: Matthijs Van Os
 */

#include <gtest/gtest.h>
#include "network.h"
#include "objectfactory.h"
#include "controller.h"
#include "trafficlight.h"
#include "tracers.h"
#include "coutredirect.h"
#include "compare.h"
#include "multicore.h"
#include "trafficsystemc.h"
#include "trafficsystemds.h"
#include "dynamiccore.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_control;
using namespace n_model;
using namespace n_tools;
using namespace n_examples;

/*
 * Simple, dumb allocator for test use
 * Spreads models evenly
 */
class SimpleAllocator: public Allocator
{
private:
	size_t m_i;
	size_t m_cores;
public:
	SimpleAllocator(size_t c)
		: m_i(0), m_cores(c)
	{
	}
	virtual ~SimpleAllocator()
	{
	}
	size_t allocate(const t_atomicmodelptr&)
	{
		int i = m_i;
		m_i = (m_i + 1) % m_cores;
		return i;
	}
};

/*
 * Function to test part of the functionality present in Controller's addModel methods
 */
void testAddModel(const t_atomicmodelptr& model, std::shared_ptr<Allocator> allocator,
        std::shared_ptr<n_control::LocationTable> locTab)
{
	size_t coreID = allocator->allocate(model);
	locTab->registerModel(model, coreID);
}

TEST(Controller, allocation)
{
	RecordProperty("description", "Adding and allocation of models, recorded in LocationTable");

	std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	// Add Models
	t_atomicmodelptr m1 = createObject<TrafficLight>("Fst");
	t_atomicmodelptr m2 = createObject<TrafficLight>("Snd");
	t_atomicmodelptr m3 = createObject<TrafficLight>("Thd");
	t_atomicmodelptr m4 = createObject<TrafficLight>("Fth");

	testAddModel(m1, allocator, locTab);
	testAddModel(m2, allocator, locTab);
	testAddModel(m3, allocator, locTab);
	testAddModel(m4, allocator, locTab);

	EXPECT_EQ(locTab->lookupModel("Fst"), 0);
	EXPECT_EQ(locTab->lookupModel("Snd"), 1);
	EXPECT_EQ(locTab->lookupModel("Thd"), 0);
	EXPECT_EQ(locTab->lookupModel("Fth"), 1);
}

TEST(Controller, cDEVS)
{
	RecordProperty("description", "Running a simple single core simulation");
	std::ofstream filestream(TESTFOLDER "controller/devstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<Core>();
		coreMap[0] = c;

		Controller ctrl = Controller("testController", coreMap, allocator, locTab, tracers);
		ctrl.setClassicDEVS();
		ctrl.setTerminationTime(t_timestamp(360, 0));

		t_atomicmodelptr m1 = createObject<TrafficLight>("Fst");
		ctrl.addModel(m1);

		ctrl.simulate();
		EXPECT_TRUE(c->terminated() == true);
		EXPECT_TRUE(c->getTime() >= t_timestamp(360, 0));
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/devstest.txt", TESTFOLDER "controller/devstest.corr"), 0);
}

TEST(Controller, cDEVS_coupled)
{
	RecordProperty("description", "Running a simple single core simulation");
	std::ofstream filestream(TESTFOLDER "controller/devstestcoupled.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<Core>();
		coreMap[0] = c;

		Controller ctrl = Controller("testController", coreMap, allocator, locTab, tracers);
		ctrl.setClassicDEVS();
		ctrl.setTerminationTime(t_timestamp(360, 0));

		t_coupledmodelptr m1 = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl.addModel(m1);

		ctrl.simulate();
		EXPECT_TRUE(c->terminated() == true);
		EXPECT_TRUE(c->getTime() >= t_timestamp(360, 0));
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/devstestcoupled.txt", TESTFOLDER "controller/devstestcoupled.corr"), 0);
}

TEST(Controller, DSDEVS_connections)
{
	RecordProperty("description", "Running a simple single core simulation with DS");
	std::ofstream filestream(TESTFOLDER "controller/dstestConnections.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<DynamicCore>();
		coreMap[0] = c;

		Controller ctrl = Controller("testController", coreMap, allocator, locTab, tracers);
		ctrl.setDSDEVS();
		ctrl.setTerminationTime(t_timestamp(3600, 0));

		t_coupledmodelptr m = createObject<n_examples_ds::TrafficSystem>("trafficSystem");
		ctrl.addModel(m);

		ctrl.simulate();
		EXPECT_TRUE(c->terminated() == true);
		EXPECT_TRUE(c->getTime() >= t_timestamp(3600, 0));
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/dstestConnections.txt", TESTFOLDER "controller/dstestConnections.corr"), 0);
}

TEST(Controller, pDEVS)
{
	return;
	RecordProperty("description", "Running a simple multicore simulation");
	std::ofstream filestream(TESTFOLDER "controller/pdevstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		t_networkptr network = createObject<Network>(2);
		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
		t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
		coreMap[0] = c1;
		coreMap[1] = c2;

		t_timestamp endTime(2000, 0);

		Controller ctrl = Controller("testController", coreMap, allocator, locTab, tracers);
		ctrl.setPDEVS();
		ctrl.setTerminationTime(endTime);

		t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl.addModel(m);

		EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

		ctrl.simulate();
		EXPECT_TRUE(c1->isLive() == false);
		EXPECT_TRUE(c2->isLive() == false);
		EXPECT_TRUE(c1->getTime()>= endTime || c2->getTime()>= endTime);
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/pdevstest.txt", TESTFOLDER "controller/pdevstest.corr"), 0);
}
