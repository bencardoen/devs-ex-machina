/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
        std::shared_ptr<LocationTable> locTab, std::shared_ptr<Allocator> alloc)
	: m_isClassicDEVS(true), m_name(name), m_checkTermTime(false), m_checkTermCond(false), m_cores(cores),
	  m_locTab(locTab), m_allocator(alloc)
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

void Controller::addModel(t_coupledmodelptr& coupled)
{
	std::vector<t_atomicmodelptr> atomics = directConnect(coupled);
	for (auto at : atomics) {
		addModel(at);
	}
}

void Controller::simulate()
{
	if (m_checkpointInterval.getTime() > 0) { // checkpointing is active
		startGVTThread();
	}
	while(true) {
		// start off all cores
		for (auto core : m_cores) {
			//core smallstep
			//check if stopped
		}
	}
}

void Controller::setClassicDEVS(bool classicDEVS)
{
	m_isClassicDEVS = classicDEVS;
}

void Controller::setDSDEVS(bool dsdevs)
{
	m_isClassicDEVS = !dsdevs;
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

void Controller::waitFinish(size_t runningCores)
{
	throw std::logic_error("Controller : waitFinish not implemented");
}

bool Controller::check()
{
	if (m_checkTermTime) {
		//TODO
	}
	if (m_checkTermCond) {
		//TODO
	}
	return false;
}

bool Controller::isFinished(size_t runningCores)
{
	throw std::logic_error("Controller : isFinished not implemented");
}

} /* namespace n_control */
