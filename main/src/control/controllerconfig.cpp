/*
 * ControllerConfig.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#include "control/controllerconfig.h"

using n_tools::createObject;
using n_tools::SharedVector;

namespace n_control {

ControllerConfig::ControllerConfig()
	: m_name("MySimulation"), m_simType(Controller::CLASSIC), m_coreAmount(1), m_pdevsType(OPTIMISTIC), m_saveInterval(5), m_tracerset(nullptr), m_zombieIdleThreshold(-1)
{
}

ControllerConfig::~ControllerConfig()
{
}

std::shared_ptr<Controller> ControllerConfig::createController()
{
	auto tracers = m_tracerset? m_tracerset : createObject<n_tracers::t_tracerset>();
	std::unordered_map<std::size_t, t_coreptr> coreMap;
	std::shared_ptr<n_control::LocationTable> locTab =
		createObject<n_control::LocationTable>((m_simType == Controller::PDEVS) ? m_coreAmount : 1);

	// If no custom allocator is given, use simple allocator
	if (m_allocator == nullptr)
		m_allocator = createObject<SimpleAllocator>((m_simType == Controller::PDEVS) ? m_coreAmount : 1);

	// Create all cores
	switch (m_simType) {
	case Controller::CLASSIC:
		coreMap[0] = createObject<Core>();
		break;
	case Controller::DSDEVS:
		coreMap[0] = createObject<DynamicCore>();
		break;
	case Controller::PDEVS:
		t_networkptr network = createObject<Network>(m_coreAmount);
		switch (m_pdevsType) {
		case OPTIMISTIC:
			for (size_t i = 0; i < m_coreAmount; ++i) {
				coreMap[i] = createObject<Multicore>(network, i, locTab, m_coreAmount);
			}
			break;
		case CONSERVATIVE:
			t_eotvector eotvector = createObject<SharedVector<t_timestamp>>(m_coreAmount, t_timestamp(0,0));
			for (size_t i = 0; i < m_coreAmount; ++i) {
				coreMap[i] = createObject<Conservativecore>(network, i, locTab, m_coreAmount, eotvector);
			}
			break;
		}
		break;
	}

	auto ctrl = createObject<Controller>(m_name, coreMap, m_allocator, locTab, tracers, m_saveInterval);

	ctrl->setSimType(m_simType);
	ctrl->setZombieIdleThreshold(m_zombieIdleThreshold);

	return ctrl;
}

} /* namespace n_control */
