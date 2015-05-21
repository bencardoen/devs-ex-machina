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
#include "conservativecore.h"
#include <unordered_set>
#include <thread>
#include <sstream>
#include <vector>
#include <chrono>

using namespace n_tools;

namespace n_control {

/**
 * @brief Holds several configuration options to set up a simulation, constructs a Controller.
 *
 * If nothing is configured, a default setup is made using:
 * - Classic DEVS
 * - SimpleAllocator
 * - A save interval of 5
 */
struct ControllerConfig
{
	/**
	 * The name of the controller
	 * By default: @c MySimulation
	 */
	std::string name;

	/**
	 * The type of simulation
	 * See Controller::SimType
	 * By default: @c CLASSIC
	 */
	Controller::SimType simType;

	/**
	 * The amount of cores the simulation will run on
	 * By default: @c 1
	 * @attention : This parameter is specific only to PDEVS, in other cases it will be disregarded
	 */
	size_t coreAmount;

	/**
	 * Behavior of the PDEVS simulation
	 * By default: @c OPTIMISTIC
	 * @attention : This parameter is specific only to PDEVS, in other cases it will be disregarded
	 */
	enum PDEVSBehavior {
		OPTIMISTIC,
		CONSERVATIVE
	} pdevsType;

	/**
	 * How many times the simulation will go over the list of cores before tracing and saving everything
	 * By default: @c 5
	 * @attention : This parameter is specific only to Classic DEVS, in other cases it will be disregarded
	 */
	size_t saveInterval;

	/**
	 * The component that will decide on which core to place each model
	 * By default: @c SimpleAllocator
	 */
	std::shared_ptr<Allocator> allocator;

	ControllerConfig();
	virtual ~ControllerConfig();

	/**
	 * @brief Create a Controller using the configuration options of this object
	 */
	std::shared_ptr<Controller> createController();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLERCONFIG_H_ */
