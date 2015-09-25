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
#define BENCH_FIRE_MAXTIME 3
#endif

//x size of the simulated grid
#ifndef BENCH_FIRE_SIZEX
#define BENCH_FIRE_SIZEX 3
#endif

//y size of the simulated grid
#ifndef BENCH_FIRE_SIZEY
#define BENCH_FIRE_SIZEY 3
#endif

#include "control/controller.h"
#include "examples/forestfire/firespread.h"
#include "tools/objectfactory.h"
#include "control/simpleallocator.h"
#include "model/multicore.h"

LOG_INIT("firebench.log")

int main(int, char**)
{
	auto tracers = n_tools::createObject<n_tracers::t_tracerset>();

	n_network::t_networkptr network = n_tools::createObject<Network>(2);
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::Allocator> allocator = n_tools::createObject<n_control::SimpleAllocator>(2);

	t_coreptr c1 = n_tools::createObject<Multicore>(network, 0, 2);
	t_coreptr c2 = n_tools::createObject<Multicore>(network, 1, 2);
	coreMap[0] = c1;
	coreMap[1] = c2;

	n_network::t_timestamp endTime(BENCH_FIRE_MAXTIME, 0);

	n_control::Controller ctrl("testController", coreMap, allocator, tracers);
	ctrl.setPDEVS();
	ctrl.setTerminationTime(endTime);

	t_coupledmodelptr m = n_tools::createObject<n_examples::FireSpread>(BENCH_FIRE_SIZEX, BENCH_FIRE_SIZEY);
	ctrl.addModel(m);


	ctrl.simulate();
}
