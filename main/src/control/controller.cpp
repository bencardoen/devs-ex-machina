/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
        std::shared_ptr<Allocator> alloc, std::shared_ptr<LocationTable> locTab, n_tracers::t_tracersetptr tracers,
        size_t saveInterval)
	: m_simtype(CLASSIC), m_hasMainModel(false), m_isSimulating(false), m_name(name), m_checkTermTime(false), m_checkTermCond(
	        false), m_saveInterval(saveInterval), m_cores(cores), m_locTab(locTab), m_allocator(alloc), m_tracers(
	        tracers)
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

void Controller::save(bool traceOnly)
{
	switch (m_simtype) {
	case CLASSIC: {
		if (!traceOnly) {
			throw std::logic_error("Controller : serialization for CLASSIC not implemented");
		}
		t_timestamp time = m_cores.begin()->second->getTime();
		n_tracers::traceUntil(time);
		break;
	}
	case PDEVS: {
		throw std::logic_error("Controller : save() for PDEVS not implemented");
		break;
	}
	case DSDEVS: {
		throw std::logic_error("Controller : save() for DSDEVS not implemented");
		break;
	}
	}
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
	for (auto at : atomics) {
		addModel(at);
	}
	m_hasMainModel = true;
}

void Controller::simulate()
{
	assert(m_isSimulating == false && "Can't start a simulation while already simulating, dummy");

	if (!m_hasMainModel) {
		// nothing to do, so don't even start
		LOG_WARNING("CONTROLLER: Trying to run simulation without any models!");
		return;
	}

	m_isSimulating = true;

	// run simulation
	switch(m_simtype) {
	case CLASSIC:
		simDEVS();
		break;
	case PDEVS:
		simPDEVS();
		break;
	case DSDEVS:
		simDSDEVS();
		break;
	}

	n_tracers::traceUntil(t_timestamp::infinity());
//	n_tracers::clearAll();
	n_tracers::waitForTracer();

	m_isSimulating = false;
}

void Controller::simDEVS()
{
	// configure core
	auto core = m_cores.begin()->second; // there is only one core in Classic DEVS
	core->setTracers(m_tracers);
	core->init();

	if (m_checkTermTime)
		core->setTerminationTime(m_terminationTime);
	if (m_checkTermCond)
		core->setTerminationFunction(m_terminationCondition);

	core->setLive(true);

	uint i = 0;
	while (check()) { // As long any cores are active
		++i;
		LOG_INFO("CONTROLLER: Commencing simulation loop #", i, "...");
		for (auto core : m_cores) {
			if (core.second->isLive()) {
				LOG_INFO("CONTROLLER: Core ", core.second->getCoreID(), " starting small step.");
				core.second->runSmallStep();
			} else
				LOG_INFO("CONTROLLER: Shhh, core ", core.second->getCoreID(), " is resting now.");
		}
		if (i % m_saveInterval == 0) {
			save(true); // TODO remove boolean when serialization implemented
		}
	}
	LOG_INFO("CONTROLLER: All cores terminated, simulation finished.");
}

void Controller::simPDEVS()
{
	throw std::logic_error("Controller : simPDEVS not implemented");
}

void Controller::simDSDEVS()
{
	throw std::logic_error("Controller : simDSDEVS not implemented");
}

void Controller::setClassicDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simtype = CLASSIC;
}

void Controller::setPDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simtype = PDEVS;
}

void Controller::setDSDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simtype = DSDEVS;
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
