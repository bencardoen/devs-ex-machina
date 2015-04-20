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
	: m_simType(CLASSIC), m_hasMainModel(false), m_isSimulating(false), m_name(name), m_checkTermTime(false), m_checkTermCond(
	        false), m_saveInterval(saveInterval), m_cores(cores), m_locTab(locTab), m_allocator(alloc), m_tracers(
	        tracers)
{
	m_root = n_tools::createObject<n_model::RootModel>();
}

Controller::~Controller()
{
}

void Controller::save(bool traceOnly)
{
	switch (m_simType) {
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

void Controller::addModel(const t_atomicmodelptr& atomic)
{

	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();		// TODO erases correctly allocated models from core in pdevs.
	}
	size_t coreID = m_allocator->allocate(atomic);
	addModel(atomic, coreID);
	m_hasMainModel = true;
}

void Controller::addModel(const t_atomicmodelptr& atomic, std::size_t coreID)
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
	m_root->directConnect(coupled);

	for(auto& model : m_root->getComponents()){
		// addModel(model); // see git
		size_t coreID = m_allocator->allocate(model);
		addModel(model, coreID);
		LOG_DEBUG("Controller::addModel added model with name ", model->getName());
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
	switch(m_simType) {
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

	LOG_INFO("CONTROLLER: All cores terminated, simulation finished.");

	// finish up tracers
	n_tracers::traceUntil(t_timestamp::infinity());
	n_tracers::clearAll();
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
		if (core->isLive()) {
			LOG_INFO("CONTROLLER: Core ", core->getCoreID(), " starting small step.");
			core->runSmallStep();
		} else
			LOG_INFO("CONTROLLER: Shhh, core ", core->getCoreID(), " is resting now.");
		if (i % m_saveInterval == 0) {
			save(true); // TODO remove boolean when serialization implemented
		}
	}
}

void Controller::simPDEVS()
{
	std::mutex cvlock;
	std::condition_variable cv;
	std::mutex veclock;	// Lock for vector with signals
	std::vector<ThreadSignal> threadsignal;
	const std::size_t deadlockVal = 100;	// Safety, if main thread ever reaches this value, consider it a deadlock.

	// configure all cores
	for (auto core : m_cores) {
		core.second->setTracers(m_tracers);
		core.second->init();

		if (m_checkTermTime)
			core.second->setTerminationTime(m_terminationTime);
		if (m_checkTermCond)
			core.second->setTerminationFunction(m_terminationCondition);

		core.second->setLive(true);

		threadsignal.push_back(ThreadSignal::FREE);
	}

	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
		        n_tools::createObject<std::thread>(cvworker, std::ref(cv), std::ref(cvlock), i, std::ref(threadsignal),
		                std::ref(veclock), deadlockVal, std::cref(m_cores[i]), m_saveInterval));
		LOG_INFO("CONTROLLER: Started thread #", i);
	}

	for (std::size_t round = 0; round < deadlockVal; ++round) {
		bool exit_threads = false;
		for (size_t j = 0; j < m_cores.size(); ++j) {
			std::lock_guard<std::mutex> lock(veclock);
			if (threadsignal[j] == ThreadSignal::ISFINISHED) {
				LOG_INFO("CONTROLLER: Thread id ", j, " has finished, flagging down the rest.");
				threadsignal = std::vector<ThreadSignal>(m_cores.size(), ThreadSignal::ISFINISHED);
				exit_threads = true;
				break;
			}
		}
		if (exit_threads) {
			break;
		}

		bool all_waiting = false;
		while (not all_waiting) {
			std::lock_guard<std::mutex> lock(veclock);
			all_waiting = true;
			for (const auto& tsignal : threadsignal) {
				if (tsignal == ThreadSignal::SHOULDWAIT) {// FREE is ok, ISWAITING is ok, SHOULDWAIT is
									  // the one that shouldn't be set.
					all_waiting = false;
					break;
				}
			}
		}

		{	/// This section is only threadsafe if you have set all threads to SHOULDWAIT
			;
		}	/// End threadsafe section

		/// Revive threads, first toggle predicate, then release threads (reverse order will deadlock).
		{
			std::lock_guard<std::mutex> lock(veclock);
			for (size_t i = 0; i < threadsignal.size(); ++i) {
				if (threadsignal[i] != ThreadSignal::ISFINISHED) {
//					LOG_DEBUG("CONTROLLER: threads can skip next round", round);
					threadsignal[i] = ThreadSignal::FREE;
				} else {
					LOG_DEBUG("CONTROLLER : seeing finished thread with id ", i);
				}
			}
		}
		cv.notify_all();	// End of a round, it's possible some threads are already running (spurious),
					// release all explicitly. Any FREE threads don't even hit the barrier.
	}

	for (auto& t : m_threads) {
		t->join();
	}
}

void Controller::simDSDEVS()
{
	throw std::logic_error("Controller : simDSDEVS not implemented");
}

void Controller::setClassicDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simType = CLASSIC;
}

void Controller::setPDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simType = PDEVS;
}

void Controller::setDSDEVS()
{
	assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simType = DSDEVS;
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

// TODO integrate cvworker better with Controller
void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid,
        std::vector<Controller::ThreadSignal>& threadsignal, std::mutex& vectorlock, std::size_t turns,
        const t_coreptr& core, size_t saveInterval)
{
	/// A predicate is needed to refreeze the thread if gets a spurious awakening.
	auto predicate = [&]()->bool {
		std::lock_guard<std::mutex > lv(vectorlock);
		return not (threadsignal[myid]==Controller::ThreadSignal::ISWAITING);
	};
	for (size_t i = 0; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop
		{	// If another thread has finished, main will flag us down, we need to stop as well.
			std::lock_guard<std::mutex> signallock(vectorlock);
			if (threadsignal[myid] == Controller::ThreadSignal::ISFINISHED) {
				core->setLive(false);
				return;
			}
		}

		// Try a simulationstep, if core has terminated, set finished flag, else continue.
		if (core->isLive()) {
			LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " running simstep in round ", i);
			core->runSmallStep();
		} else {
			LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " is finished, setting flag.");
			std::lock_guard<std::mutex> signallock(vectorlock);
			threadsignal[myid] = Controller::ThreadSignal::ISFINISHED;
			return;
		}

		if (i % saveInterval == 0) {
			n_tracers::traceUntil(core->getTime());
		}

		// Has Main asked us to wait for the other ??
		bool skip_barrier = false;
		{
			std::lock_guard<std::mutex> signallock(vectorlock);
			// Case 1 : Main has asked us by setting SHOULDWAIT, tell main we're ready waiting.
			if (threadsignal[myid] == Controller::ThreadSignal::SHOULDWAIT) {
				LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " switching flag to WAITING");
				threadsignal[myid] = Controller::ThreadSignal::ISWAITING;
			}
			// Case 2 : We can skip the barrier ahead.
			if (threadsignal[myid] == Controller::ThreadSignal::FREE) {
				LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " skipping barrier, FREE is set.");
				skip_barrier = true;
			}
		}

		if (skip_barrier) {
			continue;
		} else {
			std::unique_lock<std::mutex> mylock(cvlock);
			cv.wait(mylock, predicate);				/// Infinite loop : while(!pred) wait().
		}
		/// We'll get here only if predicate = true (spurious) and/or notifyAll() is called.
	}
}

} /* namespace n_control */
