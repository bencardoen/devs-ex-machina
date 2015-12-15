/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Matthijs Van Os, Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#include "control/controllerconfig.h"

using n_tools::createObject;
using n_tools::SharedVector;
using n_tools::SharedAtomic;

namespace n_control {

ControllerConfig::ControllerConfig()
	: m_name("MySimulation"), m_simType(SimType::CLASSIC), m_coreAmount(1), m_saveInterval(5), m_tracerset(nullptr),m_turns(100000000)
{
}

ControllerConfig::~ControllerConfig()
{
}

std::shared_ptr<Controller> ControllerConfig::createController()
{
	auto tracers = m_tracerset? m_tracerset : createObject<n_tracers::t_tracerset>();

	LOG_DEBUG("# of cores: ", m_coreAmount);
	LOG_DEBUG("save interval: ", m_saveInterval);
	// parallel simulation with only one simulation core should turn into a classic simulation
	if(isParallel(m_simType) && m_coreAmount == 1)
		m_simType = SimType::CLASSIC;
	// if not parallel, fix the core amount to 1
	else if(!isParallel(m_simType))
		m_coreAmount = 1;

	std::vector<t_coreptr> coreMap;

	// If no custom allocator is given, use simple allocator
	if (m_allocator == nullptr)
		m_allocator = createObject<SimpleAllocator>(m_coreAmount);
	m_allocator->setCoreAmount(m_coreAmount);
	m_allocator->setSimType(m_simType);

	// Create all cores
	switch (m_simType) {
	case SimType::CLASSIC:
		coreMap.push_back(createObject<Core>());
		break;
	case SimType::DYNAMIC:
		coreMap.push_back(createObject<DynamicCore>());
		break;
	case SimType::OPTIMISTIC:
	{
		t_networkptr network = createObject<Network>(m_coreAmount);
		for (size_t i = 0; i < m_coreAmount; ++i) {
			coreMap.push_back(createObject<Optimisticcore>(network, i, m_coreAmount));
		}
		break;
	}
	case SimType::CONSERVATIVE:
	{
		t_networkptr network = createObject<Network>(m_coreAmount);
		t_eotvector eotvector = createObject<SharedAtomic<t_timestamp::t_time>>(m_coreAmount, 0u);
                t_timevector timevector = createObject<SharedAtomic<t_timestamp::t_time>>(m_coreAmount+1, std::numeric_limits<t_timestamp::t_time>::max());
                timevector->set(timevector->size()-1, 0u);
		for (size_t i = 0; i < m_coreAmount; ++i) {
			coreMap.push_back(createObject<Conservativecore>(network, i, m_coreAmount, eotvector, timevector));
		}
		break;
	}
	}

	auto ctrl = createObject<Controller>(m_name, coreMap, m_allocator, tracers, m_saveInterval, m_turns);

	ctrl->setSimType(m_simType);

	return ctrl;
}

} /* namespace n_control */
