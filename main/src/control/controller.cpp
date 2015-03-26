/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
	std::shared_ptr<Allocator> alloc, std::shared_ptr<LocationTable> locTab, std::shared_ptr<t_tracerset> tracers)
	: m_isClassicDEVS(true), m_isDSDEVS(false), m_name(name), m_checkTermTime(false), m_checkTermCond(false), m_cores(
	        cores), m_locTab(locTab), m_allocator(alloc), m_tracers(tracers)
{
}

Controller::~Controller()
{
}

void Controller::addModel(t_atomicmodelptr& atomic)
{
	size_t coreID = m_allocator->allocate(atomic);
	addModel(atomic, coreID);
}

void Controller::addModel(t_atomicmodelptr& atomic, std::size_t coreID)
{
	m_cores[coreID]->addModel(atomic);
	m_locTab->registerModel(atomic, coreID);
}

void Controller::addModel(t_coupledmodelptr&)
{
	throw std::logic_error("Controller : addModel(Coupled) not implemented");
}

void Controller::simulate()
{
//	if (!m_tracers->isInitialized()) {
//		// TODO ERROR
//	}

	if (m_isDSDEVS) {
		simDSDEVS();
	} else {
		simDEVS();
	}
}

void Controller::simDEVS()
{
	if (!m_isClassicDEVS && m_checkpointInterval.getTime() > 0) { // checkpointing is active
		startGVTThread();
	}

	// start off all cores
	for (auto core : m_cores) {
		core.second->init();
		if (m_checkTermTime) {
			core.second->setTerminationTime(m_terminationTime);
		}
		if (m_checkTermCond) {
			// TODO what to do with condition?
		}
		core.second->setLive(true);
	}

	// run simulation
	uint i = 0;
	while (check()) { // As long any cores are active
		++i;
		LOG_INFO("CONTROLLER: Commencing simulation loop #",i,"...");
		for (auto core : m_cores) {
			if(core.second->isLive()) {
				LOG_INFO("CONTROLLER: Core ",core.second->getCoreID()," starting small step.");
				core.second->runSmallStep();
			} else LOG_INFO("CONTROLLER: Shhh, core ",core.second->getCoreID()," is resting now.");
		}
	}
	LOG_INFO("CONTROLLER: All cores terminated, simulation finished.");
}

void Controller::simDSDEVS()
{
	throw std::logic_error("Controller : simDSDEVS not implemented");
}

void Controller::setClassicDEVS(bool classicDEVS)
{
	m_isClassicDEVS = classicDEVS;
	if (!classicDEVS) m_isDSDEVS = false;
}

void Controller::setDSDEVS(bool dsdevs)
{
	if (dsdevs) m_isClassicDEVS = true;
	m_isDSDEVS = dsdevs;
}

void Controller::setTerminationTime(t_timestamp time)
{
	m_checkTermTime = true;
	m_terminationTime = time;
}

void Controller::setTerminationCondition(
        std::function<bool(t_timestamp, const t_atomicmodelptr&)> termination_condition)
{
	m_checkTermCond = true;
	m_terminationCondition = termination_condition;
}

void Controller::setCheckpointInterval(t_timestamp interv)
{
	m_checkpointInterval = interv;
}

void Controller::startGVTThread()
{
	throw std::logic_error("Controller : startGVTThread not implemented");
}

void Controller::waitFinish(size_t)
{
	throw std::logic_error("Controller : waitFinish not implemented");
}

bool Controller::check()
{
	for (auto core : m_cores) {
		if(!core.second->terminated()) return true;
	}
	return false;
}

bool Controller::isFinished(size_t)
{
	throw std::logic_error("Controller : isFinished not implemented");
}

} /* namespace n_control */
