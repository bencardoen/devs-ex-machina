/*
 * ControllerConfig.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#include <controllerconfig.h>

namespace n_control {

ControllerConfig::ControllerConfig()
	: name("MySimulation"), simType(Controller::CLASSIC), coreAmount(1), pdevsType(OPTIMISTIC), saveInterval(5)
{
}

ControllerConfig::~ControllerConfig()
{
}

std::shared_ptr<Controller> ControllerConfig::createController()
{
	auto tracers = createObject<n_tracers::t_tracerset>();
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::LocationTable> locTab =
		createObject<n_control::LocationTable>((simType == Controller::PDEVS) ? coreAmount : 1);

	// If no custom allocator is given, use simple allocator
	if (allocator == nullptr)
		allocator = createObject<SimpleAllocator>((simType == Controller::PDEVS) ? coreAmount : 1);

	// Create all cores
	switch (simType) {
	case Controller::CLASSIC:
		coreMap[0] = createObject<Core>();
		break;
	case Controller::DSDEVS:
		coreMap[0] = createObject<DynamicCore>();
		break;
	case Controller::PDEVS:
		t_networkptr network = createObject<Network>(coreAmount);
		switch (pdevsType) {
		case OPTIMISTIC:
			for (size_t i = 0; i < coreAmount; ++i) {
				coreMap[i] = createObject<Multicore>(network, i, locTab, coreAmount);
			}
			break;
		case CONSERVATIVE:
			t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(coreAmount, t_timestamp(0,0));
			for (size_t i = 0; i < coreAmount; ++i) {
				coreMap[i] = createObject<Conservativecore>(network, i, locTab, coreAmount, eotvector);
			}
			break;
		}
		break;
	}

	auto ctrl = createObject<Controller>(name, coreMap, allocator, locTab, tracers, saveInterval);

	ctrl->setSimType(simType);

	return ctrl;
}

} /* namespace n_control */
