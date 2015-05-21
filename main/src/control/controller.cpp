/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "controller.h"
#include "flags.h"
#include <deque>
#include <thread>
#include <chrono>
#include <fstream>
#include "cereal/archives/binary.hpp"

using namespace n_tools;

namespace n_control {

Controller::Controller(std::string name, std::unordered_map<std::size_t, t_coreptr>& cores,
        std::shared_ptr<Allocator>& alloc, std::shared_ptr<LocationTable>& locTab, n_tracers::t_tracersetptr& tracers,
        size_t saveInterval)
	: m_simType(CLASSIC), m_hasMainModel(false), m_isSimulating(false), m_name(name), m_checkTermTime(false), m_checkTermCond(
	        false), m_saveInterval(saveInterval), m_cores(cores), m_locTab(locTab), m_allocator(alloc), m_tracers(
	        tracers), m_dsPhase(false), m_sleep_gvt_thread(85), m_rungvt(false)
{
	m_root = n_tools::createObject<n_model::RootModel>();
}

Controller::~Controller()
{
}

void Controller::save(const std::string& fname)
{
	if (fname == "")
		return;
	std::fstream fs(fname, std::fstream::out | std::fstream::trunc | std::fstream::binary);
	cereal::BinaryOutputArchive oarchive(fs);

	if(m_coupledOrigin) {
		oarchive(m_coupledOrigin, m_lastGVT);
	} else {
		oarchive(m_atomicOrigin, m_lastGVT);
	}

}

void Controller::load(const std::string& fname, bool isSingleAtomic)
{
	assert(fname != "" && "Can't load simulation without file!");
	std::fstream fs(fname, std::fstream::in | std::fstream::binary);
	cereal::BinaryInputArchive iarchive(fs);

	t_timestamp gvt;
	if(isSingleAtomic) {
		iarchive(m_atomicOrigin, m_lastGVT);
		addModel(m_atomicOrigin); // Just run standard adding procedure
	}
	else {
		iarchive(m_coupledOrigin, m_lastGVT);
		addModel(m_coupledOrigin); // Just run standard adding procedure
	}
	for(auto& core : m_cores) {
		core.second->initExistingSimulation(m_lastGVT);
	}

    //TODO: create cores and iterate over models to allocate them all to the right core
    //TODO: set loaded GVT to the new cores
    // @use void initExistingSimulation(t_timestamp loaddate); in Core.
}

void Controller::addModel(t_atomicmodelptr& atomic)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
		m_hasMainModel = false;
	}
	size_t coreID = m_allocator->allocate(atomic);
	addModel(atomic, coreID);

	if (m_simType == SimType::DSDEVS)
		atomic->setController(this);
	if(!m_hasMainModel) {
		m_atomicOrigin = atomic;
		m_hasMainModel = true;
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
	assert(coupled != nullptr && "Cannot add nullptr as origin coupled model.");

	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	m_coupledOrigin = coupled;
	m_root->directConnect(coupled);

	for (t_atomicmodelptr& model : m_root->getComponents()) {
		size_t coreID = m_allocator->allocate(model);
		addModel(model, coreID);
		if (m_simType != SimType::PDEVS)
			model->setKeepOldStates(false);
		else
			model->setKeepOldStates(true);
		LOG_DEBUG("Controller::addModel added model with name ", model->getName());
	}
	if (m_simType == SimType::DSDEVS)
		coupled->setController(this);
	m_hasMainModel = true;
}

void Controller::doDirectConnect()
{
	if (m_coupledOrigin) {
		m_root->directConnect(m_coupledOrigin);
	} else {
		LOG_DEBUG("doDirectConnect no coupled origin!");
	}
}

void Controller::setSimType(SimType type)
{
	m_simType = type;
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

void Controller::addPauseEvent(t_timestamp time, size_t duration, bool repeating)
{
	m_events.push(TimeEvent(time, duration, repeating));
}

void Controller::addSaveEvent(t_timestamp time, std::string prefix, bool repeating)
{
	m_events.push(TimeEvent(time, prefix, repeating));
}

void Controller::emptyAllCores()
{
	for (auto core : m_cores) {
		core.second->clearModels();
	}
	m_root = n_tools::createObject<n_model::RootModel>(); // reset root
}

void Controller::setGVTInterval(std::size_t ms)
{
	this->m_sleep_gvt_thread.store(ms);
}

std::size_t Controller::getGVTInterval()
{
	return this->m_sleep_gvt_thread;
}

void Controller::distributeTerminationTime(t_timestamp ntime)
{
	for (const auto& core : m_cores) {
		core.second->setTerminationTime(ntime);
	}
}

void Controller::simulate()
{
	assert(m_isSimulating == false && "Can't start a simulation while already simulating.");

	if (!m_hasMainModel) {
		// nothing to do, so don't even start
		LOG_WARNING("CONTROLLER: Trying to run simulation without any models!");
		return;
	}

	m_events.prepare();
	m_tracers->startTrace();

	m_isSimulating = true;

	// run simulation
	switch (m_simType) {
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
	m_tracers->finishTrace();

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
		} else {
			LOG_INFO("CONTROLLER: Shhh, core ", core->getCoreID(), " is resting now.");
			if (i % m_saveInterval == 0) {
				t_timestamp time = m_cores.begin()->second->getTime();
				n_tracers::traceUntil(time);
				if(m_events.todo(core->getTime())) handleTimeEventsSingle();
			}
		}
		if(core->getZombieRounds() > 1){
			LOG_ERROR("Core has reached zombie state in classic devs.");
			break;
		}
	}
}

void Controller::simPDEVS()
{
	std::mutex cvlock;
	std::condition_variable cv;
	std::mutex veclock;	// Lock for vector with signals
	std::vector<std::size_t> threadsignal;
	constexpr std::size_t deadlockVal = 10000;	// If a thread fails to stop, provide a cutoff value.

	// configure all cores
	for (auto core : m_cores) {
		core.second->setTracers(m_tracers);
		core.second->init();

		if (m_checkTermTime)
			core.second->setTerminationTime(m_terminationTime);
		if (m_checkTermCond)
			core.second->setTerminationFunction(m_terminationCondition);

		core.second->setLive(true);

		threadsignal.push_back(n_threadflags::FREE);
	}

	this->m_rungvt.store(true);

	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
		        std::thread(cvworker, std::ref(cv), std::ref(cvlock), i, std::ref(threadsignal),
		                std::ref(veclock), deadlockVal, std::ref(*this)));
		LOG_INFO("CONTROLLER: Started thread # ", i);
	}

	do {
		this->startGVTThread();	// Starts and joins GVT threads.
	} while (handleTimeEventsParallel(cv, cvlock));
	// Explanation of the above:
	//  The event queue is checked for occurring events each time a GVT is calculated
	//  If there are any, the simulation needs to be halted in any case so the GVT thread is stopped
	//  All events are then handled by handleTimeEvents, which returns True so a new GVT thread is set up again
	//   and the simulation continues
	//  If the GVT thread ended for any other reason we pass through handleTimeEvents and break out of the loop

	for (auto& t : m_threads) {
		t.join();
	}
}

void Controller::simDSDEVS()
{
	auto core = m_cores.begin()->second; // there is only one core in DS DEVS
	core->setTracers(m_tracers);
	core->init();

	if (m_checkTermTime)
		core->setTerminationTime(m_terminationTime);
	if (m_checkTermCond)
		core->setTerminationFunction(m_terminationCondition);

	core->setLive(true);

	std::vector<n_model::t_atomicmodelptr> imminent;
	std::size_t i = 0;
	while (core->isLive()) {
		++i;
		imminent.clear();
		LOG_INFO("CONTROLLER: Commencing DSDEVS simulation loop #", i, " at time ", core->getTime());
		if (core->isLive()) {
			LOG_INFO("CONTROLLER: DSDEVS Core ", core->getCoreID(), " starting small step.");
			core->runSmallStep();
			core->getLastImminents(imminent);
			doDSDevs(imminent);
		} else {
			LOG_DEBUG("CONTROLLER: CORE NO LONGER LIVE");
			break;
		}
		if (i % m_saveInterval == 0) {
			t_timestamp time = m_cores.begin()->second->getTime();
			n_tracers::traceUntil(time);
			if(m_events.todo(core->getTime())) handleTimeEventsSingle();
		}
		if(core->getZombieRounds() > 1){
			LOG_ERROR("Core has reached zombie state in ds devs.");
			break;
		}
	}
}

void Controller::startGVTThread()
{
	constexpr std::size_t infguard = 100;
	std::size_t i = 0;
	std::chrono::milliseconds ms { 5 };	// Wait before running gvt, this prevents an obvious gvt of zero.
	std::this_thread::sleep_for(ms);
	LOG_INFO("Controller:: starting GVT thread");
	std::thread runonce(&runGVT, std::ref(*this), std::ref(m_rungvt));
	runonce.join();
	std::chrono::milliseconds sleep { m_sleep_gvt_thread };	// Wait before running gvt, this prevents an obvious gvt of zero.
	std::this_thread::sleep_for(sleep);
	LOG_INFO("Controller:: joined GVT thread");
	while(m_rungvt.load()==true){
		if(infguard < ++i){
			LOG_WARNING("Controller :: GVT overran max ", infguard, " nr of invocations, breaking of.");
			m_rungvt.store(false);
			break;	// No join, have not started thread.
		}
		std::chrono::milliseconds ms { this->getGVTInterval() };// Wait before running gvt, this prevents an obvious gvt of zero.
		std::this_thread::sleep_for(ms);
		LOG_INFO("Controller:: starting GVT thread");
		std::thread runnxt(&runGVT, std::ref(*this), std::ref(m_rungvt));
		runnxt.join();
	}
	LOG_INFO("Controller:: GVT thread joined.");
}

bool Controller::check()
{
	for (auto core : m_cores) {
		if (core.second->isLive())
			return true;
	}
	return false;
}

void Controller::handleTimeEventsSingle()
{
	LOG_INFO("CONTROLLER: Handling any events");
	std::vector<TimeEvent> worklist = m_events.popUntil(m_lastGVT);
	uint pause = 0;
	for (TimeEvent& event : worklist) {
		switch (event.m_type) {
		case TimeEvent::Type::PAUSE:
			LOG_INFO("CONTROLLER: Pausing at ", n_tools::toString(event.m_time.getTime()), " for ",
			        event.m_duration, " seconds ", ((event.m_repeating) ? "[REPEATING]" : "[SINGLE]"));
			pause += event.m_duration;
			break;
		case TimeEvent::Type::SAVE:
			save(event.m_prefix + "_" + n_tools::toString(event.m_time.getTime()));
			break;
		}
	}
	if (pause) {
		sleep(pause);
	}
}

bool Controller::handleTimeEventsParallel(std::condition_variable& cv, std::mutex& cvlock)
{
	LOG_INFO("CONTROLLER: Handling any events");
	bool svd = false;
	size_t pause = 0;
	std::vector<TimeEvent> worklist = m_events.popUntil(m_lastGVT);
	for (TimeEvent& event : worklist) {
		switch (event.m_type) {
		case TimeEvent::Type::PAUSE:
			LOG_INFO("CONTROLLER: Pausing at ", n_tools::toString(event.m_time.getTime()), " for ",
			        event.m_duration, " seconds ", ((event.m_repeating) ? "[REPEATING]" : "[SINGLE]"));
			pause += event.m_duration;
			break;
		case TimeEvent::Type::SAVE:
			svd = true;
			cvlock.lock();		//Stop cores
			cv.notify_all();
			save(event.m_prefix + "_" + n_tools::toString(event.m_time.getTime()));
			cvlock.unlock();	//Start cores again
			cv.notify_all();
			break;
		}
	}
	if (pause) {
		cvlock.lock();		//Stop cores
		cv.notify_all();
		sleep(pause);
		cvlock.unlock();	//Start cores again
		cv.notify_all();
	}
	return (svd || pause > 0);
}

void Controller::doDSDevs(std::vector<n_model::t_atomicmodelptr>& imminent)
{
	m_dsPhase = true;
	// loop over all atomic models
	std::deque<t_modelptr> models;
	for (t_atomicmodelptr& model : imminent) {
		if (model->modelTransition(&m_sharedState)) {
			// keep a reference to the parent of this model
			if (model->getParent().expired())
				continue;
			t_modelptr parent = model->getParent().lock();
			if (parent)
				models.push_back(parent);
		}
	}
	// continue looping until we’re at the top of the tree
	while (models.size()) {
		t_modelptr top = models.front();
		models.pop_front();
		if (top->modelTransition(&m_sharedState)) {
			// do the DS transition
			if (!top->modelTransition(&m_sharedState))
				continue; // no need to continue
			// if nothing happened .
			// keep a reference to the parent of this model
			if (top->getParent().expired())
				continue;
			t_modelptr parent = top->getParent().lock();
			if (parent)
				models.push_back(parent);
		}
	}
	// perform direct connect
	// if nothing structurally changed , nothing will happen .
	doDirectConnect();
	m_dsPhase = false;
}

void Controller::dsAddConnection(const n_model::t_portptr&, const n_model::t_portptr&, const t_zfunc&)
{
	assert(isInDSPhase() && "Controller::dsAddConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemoveConnection(const n_model::t_portptr&, const n_model::t_portptr&)
{
	assert(isInDSPhase() && "Controller::dsRemoveConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemovePort(n_model::t_portptr&)
{
	assert(isInDSPhase() && "Controller::dsRemovePort called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsScheduleModel(const n_model::t_modelptr& model)
{
	assert(isInDSPhase() && "Controller::dsScheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();
	//recursively add submodels, if necessary
	t_coupledmodelptr coupled = std::dynamic_pointer_cast<CoupledModel>(model);
	if (coupled) {
		LOG_DEBUG("Adding new coupled model during DS phase: ", model->getName());
		//it is a coupled model, remove all its children
		std::vector<t_modelptr> comp = coupled->getComponents();
		for (t_modelptr& sub : comp)
			dsScheduleModel(sub);
		return;
	}
	t_atomicmodelptr atomic = std::dynamic_pointer_cast<AtomicModel>(model);
	if (atomic) {
		LOG_DEBUG("Adding new atomic model during DS phase: ", model->getName());
		//it is an atomic model. Just remove this one from the core and the root devs
		m_cores.begin()->second->addModelDS(atomic);
		atomic->setKeepOldStates(false);
		//no need to remove the model from the root devs. We have to redo direct connect anyway
		return;
	}
	model->setController(this);
	assert(false && "Tried to add a model that is neither an atomic nor a coupled model.");
}

void Controller::dsUnscheduleModel(n_model::t_atomicmodelptr& model)
{
	assert(isInDSPhase() && "Controller::dsUnscheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();

	LOG_DEBUG("removing model: ", model->getName());
	//it is an atomic model. Just remove this one from the core
	m_cores.begin()->second->removeModel(model->getName());
}

void Controller::dsUndoDirectConnect()
{
	assert(isInDSPhase() && "Controller::dsUndoDirectConnect called while not in the DS phase.");
	m_root->undoDirectConnect();
}

bool Controller::isInDSPhase() const
{
	return m_dsPhase;
}

void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid, std::vector<std::size_t>& threadsignal,
        std::mutex& vectorlock, std::size_t turns, Controller& ctrl)
{
	auto core = ctrl.m_cores[myid];
	constexpr size_t YIELD_ZOMBIE = 10;	// @see Core::m_zombie_rounds
	auto predicate = [&]()->bool {
		std::lock_guard<std::mutex> lv(vectorlock);
		return not flag_is_set(threadsignal[myid], n_threadflags::ISWAITING);
	};

	for (size_t i = 0; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop
		if(core->getZombieRounds()>YIELD_ZOMBIE){
			LOG_INFO("CVWORKER: Thread for core ", core->getCoreID(), " Core is zombie, yielding thread.");
			std::chrono::milliseconds ms{25};
			std::this_thread::sleep_for(ms);// Don't kill a core, only yield.
		}

		{	/// Intercept a direct order to stop myself.
			std::lock_guard<std::mutex> signallock(vectorlock);
			if (flag_is_set(threadsignal[myid], n_threadflags::STOP)) {
				core->setLive(false);
				ctrl.m_rungvt.store(false);
				return;
			}
		}

		if (core->isIdle()) {
			std::chrono::milliseconds ms{45};	// this is a partial solution to a hard problem
			std::this_thread::sleep_for(ms);	// Allow some time before checking all cores are idle
			{					// lowers the probability of triggering deadlock.
				std::lock_guard<std::mutex> signallock(vectorlock);
				set_flag(threadsignal[myid], n_threadflags::IDLE);
				LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID()," threadsignal setting flag to IDLE");
			}
			if(core->terminatedByFunctor()){
				ctrl.distributeTerminationTime(core->getTime());
			}
			/// Decide if everyone is idle.
			// Can't be done by counting flags, these are to slow to be set.
			// Atomic isIdle is waay faster.
			bool quit = true;
			for(const auto& coreentry : ctrl.m_cores ){
				if( not coreentry.second->isIdle()){
					quit = false;
					break;
				}
			}
			if(quit){
				if (not core->existTransientMessage()) {	// Final deathtrap : network has message, can't quit.
					LOG_INFO("CVWORKER: Thread ", myid, " for core ", core->getCoreID(),
					        " all other threads are stopped or idle, network is idle, quitting.");
					ctrl.m_rungvt.store(false);
					std::lock_guard<std::mutex> signallock(vectorlock);
					set_flag(threadsignal[myid], n_threadflags::STOP);
					return;
				} else {
					LOG_INFO("CVWORKER: Thread ", myid, " for core ", core->getCoreID(),
					" all other threads are stopped or idle, network still reports transients, idling.");
				}
			}
		} else {
			std::lock_guard<std::mutex> signallock(vectorlock);
			if (flag_is_set(threadsignal[myid], n_threadflags::IDLE)) {
				LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(),
				        " core state changed from idle to working, unsetting flag from IDLE to FREE");
				unset_flag(threadsignal[myid], n_threadflags::IDLE);
				if (core->getTerminationTime() != ctrl.m_terminationTime) {// Core possibly idle due to term functor
					ctrl.distributeTerminationTime(ctrl.m_terminationTime);	// but revert can invalidate that, need
				}						// to reset all termtimes. (cascade!)
			}
		}
		if (core->isLive() or core->isIdle()) {
			LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " running simstep in round ", i);
			core->runSmallStep();
		}

		bool skip_barrier = false;
		{
			std::lock_guard<std::mutex> signallock(vectorlock);
			if (not flag_is_set(threadsignal[myid], n_threadflags::FREE)) {
				LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(),
				        " switching flag to WAITING");
				set_flag(threadsignal[myid], n_threadflags::ISWAITING);
			} else {// Don't log, this can easily go into 100MB logs if anything at all goes wrong in the sim.
				skip_barrier = true;
			}
		}
		if (not skip_barrier) {
			std::unique_lock<std::mutex> mylock(cvlock);
			cv.wait(mylock, predicate);
		}
	}
}

void runGVT(Controller& cont, std::atomic<bool>& gvtsafe)
{
	if (not gvtsafe) {
		LOG_INFO("Controller:  rungvt set to false by some Core thread, stopping GVT.");
		return;
	}
	const std::size_t corecount = cont.m_cores.size();
	t_controlmsg cmsg = n_tools::createObject<ControlMessage>(corecount, t_timestamp::infinity(),
	        t_timestamp::infinity());

	for (size_t i = 0; i < corecount; ++i) {
		cont.m_cores[i]->receiveControl(cmsg, (i == 0), gvtsafe);
		if (gvtsafe == false) {
			LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
			return;
		}
	}
	/// First round done, let Pinit check if we have found a gvt.
	t_coreptr first = cont.m_cores[0];
	first->receiveControl(cmsg, false, gvtsafe);

	if (cmsg->isGvtFound()) {
		LOG_INFO("Controller: found GVT after first round, gvt=", cmsg->getGvt(), " updating cores.");
		for (const auto& ucore : cont.m_cores)
			ucore.second->setGVT(cmsg->getGvt());
		n_tracers::traceUntil(cmsg->getGvt());
		cont.m_lastGVT = cmsg->getGvt();
		if (cont.m_events.countTodo(cmsg->getGvt()) > 0) {
			LOG_INFO("CONTROLLER: We have incoming events to handle, stopping GVT!");
			gvtsafe.store(false); // We have some events to handle, so we need to stop the GVT in any case
		}
	} else {
		if (gvtsafe == false) {
			LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
			return;
		}
		for (std::size_t j = 1; j < corecount; ++j)
			cont.m_cores[j]->receiveControl(cmsg, false, gvtsafe);
		if (cmsg->isGvtFound()) {
			for (const auto& ucore : cont.m_cores)
				ucore.second->setGVT(cmsg->getGvt());
			n_tracers::traceUntil(cmsg->getGvt());
			cont.m_lastGVT = cmsg->getGvt();
			if (cont.m_events.countTodo(cmsg->getGvt()) > 0) {
				LOG_INFO("CONTROLLER: We have incoming events to handle, stopping GVT!");
				gvtsafe.store(false); // We have some events to handle, so we need to stop the GVT in any case
			}
		} else {
			LOG_WARNING("Controller : Algorithm did not find GVT in second round. Not doing anything.");
		}
	}
}

} /* namespace n_control */
