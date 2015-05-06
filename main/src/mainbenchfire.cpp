/*
 * mainbenchfire.cpp
 *
 *  Created on: May 5, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

//performance benchmark for pDEVS and the fire model

//set some macros, if they aren't defined already when compiling
//termination time
#ifndef BENCH_FIRE_MAXTIME
#define BENCH_FIRE_MAXTIME 10
#endif

//x size of the simulated grid
#ifndef BENCH_FIRE_SIZEX
#define BENCH_FIRE_SIZEX 5
#endif

//y size of the simulated grid
#ifndef BENCH_FIRE_SIZEY
#define BENCH_FIRE_SIZEY 5
#endif

#include "controller.h"
#include "examples/forestfire/firespread.h"
#include "objectfactory.h"
#include "simpleallocator.h"
#include "multicore.h"

LOG_INIT("firebench.log")

int main(int, char**)
{
	auto tracers = n_tools::createObject<n_tracers::t_tracerset>();

	n_network::t_networkptr network = n_tools::createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = n_tools::createObject<n_control::SimpleAllocator>(2);
	std::shared_ptr<n_control::LocationTable> locTab = n_tools::createObject<n_control::LocationTable>(2);

	t_coreptr c1 = n_tools::createObject<Multicore>(network, 0, locTab, 2);
	t_coreptr c2 = n_tools::createObject<Multicore>(network, 1, locTab, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	n_network::t_timestamp endTime(BENCH_FIRE_MAXTIME, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, locTab, tracers);
	ctrl.setPDEVS();
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = n_tools::createObject<n_examples::FireSpread>(BENCH_FIRE_SIZEX, BENCH_FIRE_SIZEY);
	ctrl.addModel(m);


	ctrl.simulate();
}
