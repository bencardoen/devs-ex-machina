/*
 * Controller.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "control/controller.h"
#include "tools/flags.h"
#include <deque>
#include <thread>
#include <chrono>
#include <fstream>
#include "tools/objectfactory.h"
#include "cereal/archives/binary.hpp"
#include "cereal/types/polymorphic.hpp"

using namespace n_tools;

namespace n_control {

Controller::Controller(std::string name, std::vector<t_coreptr>& cores,
        std::shared_ptr<Allocator>& alloc, n_tracers::t_tracersetptr& tracers,
        size_t saveInterval, size_t turns)
	: m_simType(SimType::CLASSIC), m_hasMainModel(false), m_isSimulating(false), m_name(name), m_checkTermTime(
	false), m_checkTermCond(false), m_saveInterval(saveInterval), m_cores(cores), m_allocator(
	        alloc), m_tracers(tracers), m_dsPhase(false), m_sleep_gvt_thread(85), m_rungvt(false), m_turns(turns)
#ifdef USE_STAT
	, m_gvtStarted("_controller/gvt started", ""),
	m_gvtSecondRound("_controller/gvt 2nd rounds", ""),
	m_gvtFailed("_controller/gvt failed", ""),
	m_gvtFound("_controller/gvt found", "")
#endif
{
	m_zombieIdleThreshold.store(-1);
}

Controller::~Controller()
{
}

//enum CTRLSTAT_TYPE{GVT_2NDRND,GVT_FOUND,GVT_START,GVT_FAILED};

void Controller::logStat(CTRLSTAT_TYPE ev)
{
#ifdef USE_STAT

        switch(ev){
        case GVT_2NDRND:{
                ++m_gvtSecondRound;
                break;
        }
        case GVT_FAILED:{
                ++m_gvtFailed;
                break;
        }
        case GVT_START:{
                ++m_gvtStarted;
                break;
        }
        case GVT_FOUND:{
                ++m_gvtFound;
                break;
        }
        default:
                LOG_ERROR("No such stattype");
        //case GVT
        }
//#else // TODO use compiler attribute pure, allows compiler to ignore fcall().
        // see http://stackoverflow.com/questions/6623879/c-optimizing-function-with-no-side-effects
#endif
}

void Controller::addModel(const t_atomicmodelptr& atomic)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
		m_hasMainModel = false;
	}
	//size_t coreID = m_allocator->allocate(atomic);		// Do we still need 1 atomic usecase ??
	const std::vector<t_atomicmodelptr> models = {atomic};		// anyway, let's not leave landmines
	m_allocator->allocateAll(models);
	addModel(atomic, atomic->getCorenumber());

	if (m_simType == SimType::DYNAMIC)
		atomic->setController(this);
	if(!m_hasMainModel) {
		m_atomicOrigin = atomic;
		m_hasMainModel = true;
	}
}

void Controller::addModel(const t_atomicmodelptr& atomic, std::size_t coreID)
{
	m_cores[coreID]->addModel(atomic);
}

void Controller::addModel(const t_coupledmodelptr& coupled)
{
	assert(m_isSimulating == false && "Cannot replace main model during simulation");
	assert(coupled != nullptr && "Cannot add nullptr as origin coupled model.");

	if (m_hasMainModel) { // old models need to be replaced
		LOG_WARNING("CONTROLLER: Replacing main model, any older models will be dropped!");
		emptyAllCores();
	}
	m_coupledOrigin = coupled;
	m_root.directConnect(coupled);

	const std::vector<t_atomicmodelptr>& atomics = m_root.getComponents();
	m_allocator->allocateAll(atomics);

	for (const t_atomicmodelptr& atomic : atomics) {
		//size_t coreID = m_allocator->allocate(model);
		addModel(atomic, atomic->getCorenumber());
		atomic->setKeepOldStates(isParallel(m_simType));
		LOG_DEBUG("Controller::addModel added model with name ", atomic->getName());
	}
	if (m_simType == SimType::DYNAMIC)
		coupled->setController(this);
	m_hasMainModel = true;
}

void Controller::doDirectConnect()
{
	if (m_coupledOrigin) {
		m_root.directConnect(m_coupledOrigin);
	} else {
		LOG_DEBUG("doDirectConnect no coupled origin!");
	}
}

void Controller::setSimType(SimType type)
{
        assert(m_isSimulating == false && "Cannot change DEVS type during simulation");
	m_simType = type;
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

void Controller::emptyAllCores()
{
	for (auto core : m_cores) {
		core->clearModels();
	}
	m_root.reset(); // reset root
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
		core->setTerminationTime(ntime);
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

	LOG_DEBUG("simulating to ending time: ", m_checkTermTime? m_terminationTime:t_timestamp::infinity());

	m_tracers->startTrace();

	m_isSimulating = true;

	// run simulation
	switch (m_simType) {
	case SimType::CLASSIC:
		simDEVS();
		break;
	case SimType::OPTIMISTIC:
                simOPDEVS();
		break;
	case SimType::CONSERVATIVE:
		simCPDEVS();
		break;
	case SimType::DYNAMIC:
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
	auto& core = m_cores.front(); // there is only one core in Classic DEVS
	core->setTracers(m_tracers);

	core->init();


	if (m_checkTermTime)
		core->setTerminationTime(m_terminationTime);
	if (m_checkTermCond)
		core->setTerminationFunction(m_terminationCondition);

	core->setLive(true);

	uint i = 0;
	while (core->isLive()) { // As long any cores are active
		++i;
		LOG_INFO("CONTROLLER: Commencing simulation loop #", i, "...");
		LOG_INFO("CONTROLLER: Core ", core->getCoreID(), " starting small step.");
		core->runSmallStep();
                
		if (i % m_saveInterval == 0) {
			t_timestamp time = core->getTime();
			n_tracers::traceUntil(time);
		}
                
		if (core->getZombieRounds() > 1) {
			LOG_ERROR("Core has reached zombie state in classic devs.");
			break;
		}
	}
        t_timestamp time = core->getTime();
        n_tracers::traceUntil(time);
}

void Controller::simOPDEVS()
{
	std::mutex cvlock;
	std::condition_variable cv;
	std::mutex veclock;	// Lock for vector with signals
	std::vector<std::size_t> threadsignal;
	threadsignal.reserve(m_cores.size());
	constexpr std::size_t deadlockVal = 10000;	// If a thread fails to stop, provide a cutoff value.

	// configure all cores
	for (auto& core : m_cores) {
		core->setTracers(m_tracers);
		core->init();

		if (m_checkTermTime)
			core->setTerminationTime(m_terminationTime);
		if (m_checkTermCond)
			core->setTerminationFunction(m_terminationCondition);

		core->setLive(true);

		threadsignal.push_back(n_threadflags::FREE);
	}

	this->m_rungvt.store(true);

	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
		        std::thread(cvworker, std::ref(cv), std::ref(cvlock), i, std::ref(threadsignal),
		                std::ref(veclock), deadlockVal, std::ref(*this)));
		LOG_INFO("CONTROLLER: Started thread # ", i);
	}
        
	this->startGVTThread();	// Starts and joins GVT threads.
        
	for (auto& t : m_threads) {
		t.join();
	}
}

void Controller::simCPDEVS()
{
	std::mutex cvlock;
	std::condition_variable cv;
	std::mutex veclock;	// Lock for vector with signals
	std::vector<std::size_t> threadsignal;
	threadsignal.reserve(m_cores.size());

	// configure all cores
	for (auto& core : m_cores) {
		core->setTracers(m_tracers);
		core->init();

		if (m_checkTermTime)
			core->setTerminationTime(m_terminationTime);
		if (m_checkTermCond)
			core->setTerminationFunction(m_terminationCondition);

		core->setLive(true);

		threadsignal.push_back(n_threadflags::FREE);
	}


	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
		        std::thread(cvworker, std::ref(cv), std::ref(cvlock), i, std::ref(threadsignal),
		                std::ref(veclock), m_turns, std::ref(*this)));
		LOG_INFO("CONTROLLER: Started thread # ", i);
	}

	for (auto& t : m_threads) {
		t.join();
	}
}

void Controller::simDSDEVS()
{
	auto& core = m_cores.front(); // there is only one core in DS DEVS
	core->setTracers(m_tracers);

	core->init();


	if (m_checkTermTime)
		core->setTerminationTime(m_terminationTime);
	if (m_checkTermCond)
		core->setTerminationFunction(m_terminationCondition);

	core->setLive(true);

	std::vector<n_model::t_raw_atomic> imminent;
	std::size_t i = 0;
	while (core->isLive()) {
		++i;
		imminent.clear();
		LOG_INFO("CONTROLLER: Commencing DSDEVS simulation loop #", i, " at time ", core->getTime());
		LOG_INFO("CONTROLLER: DSDEVS Core ", core->getCoreID(), " starting small step.");
		core->runSmallStep();
		core->getLastImminents(imminent);
		doDSDevs(imminent);
		core->validateModels();

		if (i % m_saveInterval == 0) {
			t_timestamp time = core->getTime();
			n_tracers::traceUntil(time);
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
	for (const auto& core : m_cores) {
		if (core->isLive())
			return true;
	}
	return false;
}


void Controller::doDSDevs(std::vector<n_model::t_raw_atomic>& imminent)
{
	m_dsPhase = true;
	// loop over all atomic models
	std::deque<Model*> models;
	for (t_raw_atomic model : imminent) {
		if (model->doModelTransition(&m_sharedState)) {
			// keep a reference to the parent of this model
			Model* parent = model->getParent();
			if (parent)
				models.push_back(parent);
		}
	}
	// continue looping until we’re at the top of the tree
	while (models.size()) {
		Model* top = models.front();
		models.pop_front();
		if (top->doModelTransition(&m_sharedState)) {
			// keep a reference to the parent of this model
			Model* parent = top->getParent();
			if (parent)
				models.push_back(parent);
		}
	}
	// perform direct connect
	// if nothing structurally changed , nothing will happen .
	doDirectConnect();
	m_dsPhase = false;
}

void Controller::dsAddConnection(const n_model::t_portptr&, const n_model::t_portptr&, t_zfunc)
{
	assert(isInDSPhase() && "Controller::dsAddConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemoveConnection(const n_model::t_portptr&, const n_model::t_portptr&)
{
	assert(isInDSPhase() && "Controller::dsRemoveConnection called while not in the DS phase.");
	dsUndoDirectConnect();
}

void Controller::dsRemovePort(const n_model::t_portptr&)
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
	t_atomicmodelptr atomic = std::static_pointer_cast<AtomicModel_impl>(model);
//	if (atomic) {
		LOG_DEBUG("Adding new atomic model during DS phase: ", model->getName());
		//it is an atomic model. Just remove this one from the core and the root devs
                if(!m_cores[0]->containsModel(model->getName())){
                        m_cores.front()->addModelDS(atomic);
                }
                else{
                        LOG_ERROR("Trying to add model that already exists in core ????", model->getName());
                }
                        
		atomic->setKeepOldStates(false);
		//no need to remove the model from the root devs. We have to redo direct connect anyway
//		return;
//	}
//	model->setController(this);
//	assert(false && "Tried to add a model that is neither an atomic nor a coupled model.");
}

void Controller::dsUnscheduleModel(const n_model::t_atomicmodelptr& model)
{
	assert(isInDSPhase() && "Controller::dsUnscheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();

	LOG_DEBUG("removing model: ", model->getName());
	//it is an atomic model. Just remove this one from the core
	m_cores.front()->removeModelDS(model->getName());
}

void Controller::dsUndoDirectConnect()
{
	assert(isInDSPhase() && "Controller::dsUndoDirectConnect called while not in the DS phase.");
	m_root.undoDirectConnect();
}

bool Controller::isInDSPhase() const
{
	return m_dsPhase;
}

void Controller::setZombieIdleThreshold(int threshold)
{
	m_zombieIdleThreshold.store(threshold);
}

void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid, std::vector<std::size_t>& threadsignal,
        std::mutex& vectorlock, std::size_t turns, Controller& ctrl)
{
	auto core = ctrl.m_cores[myid];
	constexpr size_t YIELD_ZOMBIE = 25;	// @see Core::m_zombie_rounds
	auto predicate = [&]()->bool {
		std::lock_guard<std::mutex> lv(vectorlock);
		return not flag_is_set(threadsignal[myid], n_threadflags::ISWAITING);
	};

	for (size_t i = 0; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop
		if(core->getZombieRounds()>YIELD_ZOMBIE){
			LOG_INFO("CVWORKER: Thread for core ", core->getCoreID(), " Core is zombie, yielding thread. [round ",core->getZombieRounds(),"]");
			std::chrono::milliseconds ms{25};
			std::this_thread::sleep_for(ms);// Don't kill a core, only yield.
			int thres = ctrl.m_zombieIdleThreshold.load();
			if(thres >= 0 && core->getZombieRounds() >= (size_t)thres) {
				LOG_WARNING("CVWORKER: Reached zombie threshold (",thres,"), idling the core!");
				core->setIdle(true);
			}
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
			// Can't be done by counting flags, these are too slow to be set.
			// Atomic isIdle is faster.
			bool quit = true;
			for(const auto& coreentry : ctrl.m_cores ){
				if( not coreentry->isIdle()){
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
			LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " running simstep in round ", i, " [zrounds:",core->getZombieRounds(),"]");
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
        cont.logStat(GVT_START);
	const std::size_t corecount = cont.m_cores.size();
	t_controlmsg cmsg = n_tools::createObject<ControlMessage>(corecount, t_timestamp::infinity(),
	        t_timestamp::infinity());

        /// Let Pinit start first round
        t_coreptr first = cont.m_cores[0];
        first->receiveControl(cmsg, 0, gvtsafe);
	for (size_t i = 1; i < corecount; ++i) {
		cont.m_cores[i]->receiveControl(cmsg, 0, gvtsafe);
		if (gvtsafe == false) {
			LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
			return;
		}
	}
        
	/// First round done, let Pinit check if we have found a gvt.
	first->receiveControl(cmsg, 1, gvtsafe);

	if (cmsg->isGvtFound()) {
		cont.logStat(GVT_FOUND);
		LOG_INFO("Controller: found GVT after first round, gvt=", cmsg->getGvt(), " updating cores.");
		for (const auto& ucore : cont.m_cores)
			ucore->setGVT(cmsg->getGvt());
		n_tracers::traceUntil(cmsg->getGvt());
		cont.m_lastGVT = cmsg->getGvt();
	} else {
                ///// 2nd round initiated.
		cont.logStat(GVT_2NDRND);
		if (gvtsafe == false) {
			LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
			return;
		}
		for (std::size_t j = 1; j < corecount; ++j)
			cont.m_cores[j]->receiveControl(cmsg, 1, gvtsafe);
                first->receiveControl(cmsg, 2, gvtsafe);  // Controlmessage must be passed to Invoking core again.
                
		if (cmsg->isGvtFound()) {
			cont.logStat(GVT_FOUND);
			LOG_INFO("Controller: found GVT after second round, gvt=", cmsg->getGvt(), " updating cores.");

			for (const auto& ucore : cont.m_cores)
				ucore->setGVT(cmsg->getGvt());
			n_tracers::traceUntil(cmsg->getGvt());
			cont.m_lastGVT = cmsg->getGvt();
		} else {
			cont.logStat(GVT_FAILED);
			LOG_ERROR("Controller : Algorithm did not find GVT in second round. Not doing anything.");
                        cmsg->logMessageState();
		}
	}
}

} /* namespace n_control */
