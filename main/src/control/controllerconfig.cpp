/*
 * ControllerConfig.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#include "control/controllerconfig.h"

using n_tools::createObject;
using n_tools::SharedVector;
using n_tools::SharedAtomic;

namespace n_control {

ControllerConfig::ControllerConfig()
	: m_name("MySimulation"), m_simType(SimType::CLASSIC), m_coreAmount(1), m_saveInterval(5), m_tracerset(nullptr), m_zombieIdleThreshold(-1),m_turns(10000000)
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
	LOG_DEBUG("zombie thresshold: ", m_zombieIdleThreshold);
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
		t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(m_coreAmount, t_timestamp(0,0));
                t_timevector timevector = createObject<SharedAtomic<t_timestamp::t_time>>(m_coreAmount, std::numeric_limits<t_timestamp::t_time>::max());
		for (size_t i = 0; i < m_coreAmount; ++i) {
			coreMap.push_back(createObject<Conservativecore>(network, i, m_coreAmount, eotvector, timevector));
		}
		break;
	}
	}

	auto ctrl = createObject<Controller>(m_name, coreMap, m_allocator, tracers, m_saveInterval, m_turns);

	ctrl->setSimType(m_simType);
	ctrl->setZombieIdleThreshold(m_zombieIdleThreshold);

	return ctrl;
}

} /* namespace n_control */
