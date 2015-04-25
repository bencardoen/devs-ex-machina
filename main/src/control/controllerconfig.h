/*
 * ControllerConfig.h
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONTROLLERCONFIG_H_
#define SRC_CONTROL_CONTROLLERCONFIG_H_

#include "network.h"
#include "objectfactory.h"
#include "controller.h"
#include "simpleallocator.h"
#include "tracers.h"
#include "multicore.h"
#include "controller.h"
#include "dynamiccore.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_tools;

namespace n_control {

/**
 * Holds several configuration options to set up a simulation, constructs a Controller.
 * If nothing is configured, a default setup is made using:
 * - Classic DEVS
 * - SimpleAllocator
 * - A save interval of 5
 */
struct ControllerConfig
{
	std::string name;
	Controller::SimType simType;
	size_t coreAmount;
	t_timestamp checkpointInterval;
	size_t saveInterval;
	std::shared_ptr<Allocator> allocator;

	ControllerConfig();
	virtual ~ControllerConfig();

	/**
	 * Create a Controller using the configuration options of this object
	 */
	std::shared_ptr<Controller> createController();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLERCONFIG_H_ */
