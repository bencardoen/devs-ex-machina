/*
 * controllertest.cpp
 *
 *  Created on: 26 Mar 2015
 *      Author: Matthijs Van Os
 */

#include <gtest/gtest.h>
#include "control/controllerconfig.h"
#include "network/network.h"
#include "tools/objectfactory.h"
#include "control/controller.h"
#include "model/conservativecore.h"
#include "control/simpleallocator.h"
#include "examples/trafficlight_classic/trafficlight.h"
#include "tracers/tracers.h"
#include "tools/coutredirect.h"
#include "test/compare.h"
#include "model/optimisticcore.h"
#include "examples/trafficlight_coupled/trafficsystemc.h"
#include "examples/trafficlight_ds/trafficsystemds.h"
#include "model/dynamiccore.h"
#include "test/testmodels.h"
#include "examples/abstract_conservative/modelc.h"
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
 * Function to test part of the functionality present in Controller's addModel methods
 */
void testAddModel(const std::vector<t_atomicmodelptr>& models, std::shared_ptr<Allocator> allocator,
        std::shared_ptr<n_control::LocationTable> locTab)
{
	allocator->allocateAll(models);
	for(const auto& m : models)
		locTab->registerModel(m, m->getCorenumber());
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

	const std::vector<t_atomicmodelptr> models= {m1,m2,m3,m4};
	testAddModel(models, allocator, locTab);

	EXPECT_EQ(locTab->lookupModel("Fst"), 0u);
	EXPECT_EQ(locTab->lookupModel("Snd"), 1u);
	EXPECT_EQ(locTab->lookupModel("Thd"), 0u);
	EXPECT_EQ(locTab->lookupModel("Fth"), 1u);
}

TEST(Controller, cDEVS)
{
	RecordProperty("description", "Running a simple single core simulation");
	std::ofstream filestream(TESTFOLDER "controller/devstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		std::vector<t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<Core>();
		coreMap.push_back(c);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setSimType(SimType::CLASSIC);

		ctrl.setTerminationTime(t_timestamp(360, 0));

		t_atomicmodelptr m1 = createObject<TrafficLight>("Fst");
		ctrl.addModel(m1);

		ctrl.simulate();
		EXPECT_TRUE(c->isLive() == false);
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

		std::vector<t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<Core>();
		coreMap.push_back(c);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setSimType(SimType::CLASSIC);
		ctrl.setTerminationTime(t_timestamp(360, 0));

		t_coupledmodelptr m1 = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl.addModel(m1);

		ctrl.simulate();
		EXPECT_TRUE(c->isLive() == false);
		EXPECT_TRUE(c->getTime() >= t_timestamp(360, 0));
	};

	EXPECT_EQ(
	        n_misc::filecmp(TESTFOLDER "controller/devstestcoupled.txt", TESTFOLDER "controller/devstestcoupled.corr"),
	        0);
}

TEST(Controller, DSDEVS_connections)
{
	RecordProperty("description", "Running a simple single core simulation with DS");
	std::ofstream filestream(TESTFOLDER "controller/dstestConnections.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		std::vector<t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<DynamicCore>();
		coreMap.push_back(c);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setSimType(SimType::DYNAMIC);
		ctrl.setTerminationTime(t_timestamp(3600, 0));

		t_coupledmodelptr m = createObject<n_examples_ds::TrafficSystem>("trafficSystem");
		ctrl.addModel(m);

		ctrl.simulate();
		EXPECT_TRUE(c->isLive() == false);
		EXPECT_TRUE(c->getTime() >= t_timestamp(3600, 0));
	};

	EXPECT_EQ(
	        n_misc::filecmp(TESTFOLDER "controller/dstestConnections.txt", TESTFOLDER "controller/dstestConnections.corr"),
	        0);
}

TEST(Controller, DSDevs_full)
{
	RecordProperty("description", "Full dynamic structured test");

	ControllerConfig conf;
	conf.m_name = "DSDevsSim";
	conf.m_simType = SimType::DYNAMIC;
	conf.m_saveInterval = 30;

	std::ofstream filestream(TESTFOLDER "controller/dstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(400, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr m1 = createObject<n_testmodel::DSDevsRoot>();
		ctrl->addModel(m1);

		ctrl->simulate();
	}
	EXPECT_EQ(
	        n_misc::filecmp(TESTFOLDER "controller/dstest.txt", TESTFOLDER "controller/dstest.corr"),
	        0);
}

TEST(Controller, pDEVS)
{
	RecordProperty("description", "Running a simple multicore simulation");
	std::ofstream filestream(TESTFOLDER "controller/pdevstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		t_networkptr network = createObject<Network>(2);
		std::vector<t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

		t_coreptr c1 = createObject<Optimisticcore>(network, 0, locTab, 2);
		t_coreptr c2 = createObject<Optimisticcore>(network, 1, locTab, 2);
		coreMap.push_back(c1);
		coreMap.push_back(c2);

		t_timestamp endTime(2000, 0);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
                ctrl.setSimType(SimType::OPTIMISTIC);
		ctrl.setTerminationTime(endTime);

		t_coupledmodelptr m = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl.addModel(m);

		EXPECT_TRUE(locTab->lookupModel("trafficLight") != locTab->lookupModel("policeman"));

		ctrl.simulate();
		EXPECT_TRUE(c1->isLive() == false);
		EXPECT_TRUE(c2->isLive() == false);
		EXPECT_TRUE(c1->getTime() >= endTime || c2->getTime() >= endTime);
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/pdevstest.txt", TESTFOLDER "controller/pdevstest.corr"), 0);
}

TEST(Controller, ControllerConfig)
{
	RecordProperty("description", "Setting up a simulation using the ControllerConfig object");

	ControllerConfig conf;
	conf.m_name = "SimpleSim";
	conf.m_saveInterval = 30;

	std::ofstream filestream(TESTFOLDER "controller/configtest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);

		t_coupledmodelptr m1 = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl->addModel(m1);

		ctrl->simulate();
	}
}

TEST(Controller, CONDEVS)
{
	RecordProperty("description", "Running a simple multicore simulation");
	std::ofstream filestream(TESTFOLDER "controller/condevstest.txt");
	using namespace n_examples_abstract_c;
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		t_networkptr network = createObject<Network>(2);
		std::vector<t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

		t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(2, t_timestamp(0,0));
                t_timevector timevector = createObject<SharedVector<t_timestamp>>(2, t_timestamp::infinity());
		auto c0 = createObject<Conservativecore>(network, 0, locTab, eotvector, timevector);
		auto c1 = createObject<Conservativecore>(network, 1, locTab, eotvector, timevector);

		coreMap.push_back(c0);
		coreMap.push_back(c1);

		t_timestamp endTime(70, 0);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setSimType(SimType::CONSERVATIVE);
		ctrl.setTerminationTime(endTime);

		t_coupledmodelptr m = createObject<ModelC>("modelC");
		ctrl.addModel(m);

		ctrl.simulate();
		EXPECT_TRUE(c0->isLive() == false);
		EXPECT_TRUE(c1->isLive() == false);
		EXPECT_TRUE(c0->getTime() >= endTime || c1->getTime() >= endTime);
	};

	EXPECT_EQ(n_misc::filecmp(TESTFOLDER "controller/condevstest.txt", TESTFOLDER "controller/condevstest.corr"), 0);
}
