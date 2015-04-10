/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
        std::shared_ptr<Allocator> alloc, std::shared_ptr<LocationTable> locTab, n_tracers::t_tracersetptr tracers)
	: m_isClassicDEVS(true), m_isDSDEVS(false), m_hasMainModel(false), m_isSimulating(false), m_name(name),
	  m_checkTermTime(false), m_checkTermCond(false), m_cores(cores), m_locTab(locTab), m_allocator(alloc),
	  m_tracers(tracers)
{
	m_root = n_tools::createObject<n_model::RootModel>();
}

Controller::~Controller()
{
}

void Controller::addModel(t_atomicmodelptr& atomic)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	size_t coreID = m_allocator->allocate(atomic);
	addModel(atomic, coreID);
	m_hasMainModel = true;
}

void Controller::addModel(t_atomicmodelptr& atomic, std::size_t coreID)
{
	m_cores[coreID]->addModel(atomic);
	m_locTab->registerModel(atomic, coreID);
}

void Controller::addModel(t_coupledmodelptr& coupled)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	throw std::logic_error("Controller : simDSDEVS not implemented");
	const std::vector<t_atomicmodelptr>& atomics = m_root->directConnect(coupled);
	for( auto at : atomics ) {
		addModel(at);
	}
	m_hasMainModel = true;
}

void Controller::simulate()
{
//	if (!m_tracers->isInitialized()) {
//		// TODO ERROR
//	}
	assert(m_isSimulating == false && "Can't start a simulation while already simulating, dummy");

	if (!m_hasMainModel) {
		// nothing to do, so don't even start
		LOG_WARNING("CONTROLLER: Trying to run simulation without any models!");
		return;
	}

	m_isSimulating = true;

	if (!m_isClassicDEVS && m_checkpointInterval.getTime() > 0) { // checkpointing is active
		startGVTThread();
	}

	// configure all cores
	for (auto core : m_cores) {
		core.second->setTracers(m_tracers);
		core.second->init();
		if (m_checkTermTime) core.second->setTerminationTime(m_terminationTime);
		if (m_checkTermCond) core.second->setTerminationFunction(m_terminationCondition);
		core.second->setLive(true);
	}

	// run simulation
	if (m_isDSDEVS) {
		simDSDEVS();
	} else {
		simDEVS();
	}

	m_isSimulating = false;
}

void Controller::simDEVS()
{
	uint i = 0;
	while (check()) { // As long any cores are active
		++i;
		LOG_INFO("CONTROLLER: Commencing simulation loop #", i, "...");
		for (auto core : m_cores) {
			if (core.second->isLive()) {
				LOG_INFO("CONTROLLER: Core ", core.second->getCoreID(), " starting small step.");
				core.second->runSmallStep();
			} else LOG_INFO("CONTROLLER: Shhh, core ", core.second->getCoreID(), " is resting now.");
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
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_isClassicDEVS = classicDEVS;
	if (!classicDEVS)
		m_isDSDEVS = false;
}

void Controller::setDSDEVS(bool dsdevs)
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	if (dsdevs)
		m_isClassicDEVS = true;
	m_isDSDEVS = dsdevs;
}

void Controller::setTerminationTime(t_timestamp time)
{
	assert(m_isSimulating == false && "Cannot change termination time during simulation");
	m_checkTermTime = true;
	m_terminationTime = time;
}

void Controller::setTerminationCondition(t_terminationfunctor termination_condition)
{
	assert(m_isSimulating == false && "Cannot change termination condition during simulation");
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
		if (!core.second->terminated())
			return true;
	}
	return false;
}

void Controller::emptyAllCores()
{
	for (auto core : m_cores) {
		core.second->clearModels();
	}
	m_root = n_tools::createObject<n_model::RootModel>(); // reset root
}

} /* namespace n_control */
