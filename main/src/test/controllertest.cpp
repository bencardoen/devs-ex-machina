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
	size_t allocate(t_atomicmodelptr)
	{
		int i = m_i;
		m_i = (m_i + 1) % m_cores;
		return i;
	}
};

TEST(Controller, allocation)
{
	RecordProperty("description", "Adding and allocation of models, recorded in LocationTable");

	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c1 = createObject<Core>();
	t_coreptr c2 = createObject<Core>();
	coreMap[0] = c1;
	coreMap[1] = c2;
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output

	Controller c = Controller("testController", coreMap, allocator, locTab, 0);

	// Add Models
	t_atomicmodelptr m1 = createObject<TrafficLight>("Fst");
	t_atomicmodelptr m2 = createObject<TrafficLight>("Snd");
	t_atomicmodelptr m3 = createObject<TrafficLight>("Thd");
	t_atomicmodelptr m4 = createObject<TrafficLight>("Fth");

	c.addModel(m1); // Model placement via allocator
	c.addModel(m2);
	c.addModel(m3);
	c.addModel(m4, 0); // Manual placement

	EXPECT_EQ(locTab->lookupModel("Fst"), 0);
	EXPECT_EQ(locTab->lookupModel("Snd"), 1);
	EXPECT_EQ(locTab->lookupModel("Thd"), 0);
	EXPECT_EQ(locTab->lookupModel("Fth"), 0);
}

TEST(Controller, cDEVS)
{
	RecordProperty("description", "Running a simple single core simulation");

	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<Allocator> allocator = createObject<SimpleAllocator>(1);
	std::shared_ptr<n_control::LocationTable> locTab = createObject<n_control::LocationTable>(2);

	t_coreptr c = createObject<Core>();
	n_tracers::t_tracersetptr tracers = createObject<n_tracers::t_tracerset>();
	tracers->stopTracers();	//disable the output
	coreMap[0] = c;

	Controller ctrl = Controller("testController", coreMap, allocator, locTab, 0);
	ctrl.setClassicDEVS();
	ctrl.setTerminationTime(t_timestamp(360, 0));

	t_atomicmodelptr m1 = createObject<TrafficLight>("Fst");
	t_atomicmodelptr m2 = createObject<TrafficLight>("Snd");
	ctrl.addModel(m1);
	ctrl.addModel(m2);

	ctrl.simulate();
	EXPECT_TRUE(c->terminated() == true);
	EXPECT_TRUE(c->getTime() >= t_timestamp(360, 0));
}

