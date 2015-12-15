/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Matthijs Van Os, Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#ifndef SRC_CONTROL_CONTROLLERCONFIG_H_
#define SRC_CONTROL_CONTROLLERCONFIG_H_

#include "network/network.h"
#include "tools/objectfactory.h"
#include "control/controller.h"
#include "control/simpleallocator.h"
#include "control/controller.h"
#include "tracers/tracers.h"
#include "model/optimisticcore.h"
#include "model/dynamiccore.h"
#include "model/conservativecore.h"
#include <unordered_set>
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>
#include <chrono>

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
	std::string m_name;

	/**
	 * @brief The type of simulation.
	 * @See Controller::SimType
	 * By default: @c CLASSIC
	 */
	SimType m_simType;

	/**
	 * The amount of cores the simulation will run on
	 * By default: @c 1
	 * @attention: This parameter is only used for a parallel simulation.
	 * @note: If only one core is available, the simulation will default to classic non-parallel mode.
	 */
	std::size_t m_coreAmount;

	/**
	 * How many times the simulation will go over the list of cores before tracing and saving everything
	 * By default: @c 5
	 * @attention : This parameter is only used for non-parallel simulations.
	 */
	std::size_t m_saveInterval;

	/**
	 * The component that will decide on which core to place each model
	 * By default: @c SimpleAllocator
	 */
	std::shared_ptr<Allocator> m_allocator;

	/**
	 * The tracerset used by the simulator.
	 * By default: @c nullptr
	 * @attention: If this parameter is not set, a new TracerSet will be created using the default constructor.
	 * @see Tracers
	 */
	n_tracers::t_tracersetptr m_tracerset;
        
        /**
         * The nr of turns a core can simulate (including any idle rounds).
         */
        std::size_t     m_turns;

	ControllerConfig();
	virtual ~ControllerConfig();

	/**
	 * @brief Create a Controller using the configuration options of this object
	 */
	std::shared_ptr<Controller> createController();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLERCONFIG_H_ */
