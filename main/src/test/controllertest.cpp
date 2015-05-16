/*
 * controllertest.cpp
 *
 *  Created on: 26 Mar 2015
 *      Author: Matthijs Van Os
 */

#include <gtest/gtest.h>
#include "controllerconfig.h"
#include "network.h"
#include "objectfactory.h"
#include "controller.h"
#include "simpleallocator.h"
#include "trafficlight.h"
#include "tracers.h"
#include "coutredirect.h"
#include "compare.h"
#include "multicore.h"
#include "trafficsystemc.h"
#include "trafficsystemds.h"
#include "dynamiccore.h"
#include "timeevent.h"
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
void testAddModel(t_atomicmodelptr& model, std::shared_ptr<Allocator> allocator,
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

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setClassicDEVS();

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

		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<Core>();
		coreMap[0] = c;

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setClassicDEVS();
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

		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(1);

		t_coreptr c = createObject<DynamicCore>();
		coreMap[0] = c;

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setDSDEVS();
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

TEST(Controller, pDEVS)
{
	RecordProperty("description", "Running a simple multicore simulation");
	std::ofstream filestream(TESTFOLDER "controller/pdevstest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto tracers = createObject<n_tracers::t_tracerset>();

		t_networkptr network = createObject<Network>(2);
		std::unordered_map<std::size_t, t_coreptr> coreMap;
		std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
		std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

		t_coreptr c1 = createObject<Multicore>(network, 0, locTab, 2);
		t_coreptr c2 = createObject<Multicore>(network, 1, locTab, 2);
		coreMap[0] = c1;
		coreMap[1] = c2;

		t_timestamp endTime(2000, 0);

		Controller ctrl("testController", coreMap, allocator, locTab, tracers);
		ctrl.setPDEVS();
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
	conf.name = "SimpleSim";
	conf.saveInterval = 30;

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

TEST(Controller, Pause)
{
	RecordProperty("description", "Tests pause functionality of simulator");

	ControllerConfig conf;
	conf.name = "SimpleSim";
	conf.saveInterval = 30;
	conf.simType = Controller::PDEVS;
	conf.coreAmount = 2;

	std::ofstream filestream(TESTFOLDER "controller/pausetest.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);
		ctrl->addPauseEvent(t_timestamp(120,0),5);

		t_coupledmodelptr m1 = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl->addModel(m1);

		ctrl->simulate();
	}
}

TEST(Controller, RepeatPause)
{
	RecordProperty("description", "Tests repeating pause");

	ControllerConfig conf;
	conf.name = "SimpleSim";
	conf.saveInterval = 30;
	conf.simType = Controller::PDEVS;
	conf.coreAmount = 2;

	std::ofstream filestream(TESTFOLDER "controller/pausetest2.txt");
	{
		CoutRedirect myRedirect(filestream);
		auto ctrl = conf.createController();
		t_timestamp endTime(360, 0);
		ctrl->setTerminationTime(endTime);
		ctrl->addPauseEvent(t_timestamp(60,0),2, true);

		t_coupledmodelptr m1 = createObject<n_examples_coupled::TrafficSystem>("trafficSystem");
		ctrl->addModel(m1);

		ctrl->simulate();
	}
}
