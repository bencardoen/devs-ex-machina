/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Matthijs Van Os, Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Pieter Lauwers
 */

#include "control/controller.h"
#include "tools/flags.h"
#include <deque>
#include <thread>
#include <chrono>
#include "tools/objectfactory.h"
#include "pools/pools.h"

using namespace n_tools;

namespace n_control {

Controller::Controller(std::string name, std::vector<t_coreptr>& cores,
        std::shared_ptr<Allocator>& alloc, n_tracers::t_tracersetptr& tracers,
        size_t saveInterval, size_t turns)
	: m_simType(SimType::CLASSIC), m_hasMainModel(false), m_isSimulating(false), m_name(name), m_checkTermTime(
	false), m_checkTermCond(false), m_saveInterval(saveInterval), m_zombieIdleThreshold(10),m_cores(cores), m_allocator(
	        alloc), m_tracers(tracers), m_dsPhase(false), m_sleep_gvt_thread(200), m_rungvt(false), m_turns(turns)
#ifdef USE_STAT
	, m_gvtStarted("_controller/gvt_started", ""),
	m_gvtSecondRound("_controller/gvt_2nd_rounds", ""),
	m_gvtFailed("_controller/gvt_failed", ""),
	m_gvtFound("_controller/gvt_found", "")
#endif
{
        n_pools::setMain();
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
//	atomic->setKeepOldStates(m_simType == SimType::OPTIMISTIC);
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
	if(m_simType != SimType::CLASSIC && m_simType != SimType::DYNAMIC) {
	    m_allocator->allocateAll(atomics);

	    for (const t_atomicmodelptr& atomic : atomics) {
	        //size_t coreID = m_allocator->allocate(model);
	        addModel(atomic, atomic->getCorenumber());
	//      atomic->setKeepOldStates(m_simType == SimType::OPTIMISTIC);        // Can't use isParallell here (which returns true for cpdevs)
	        LOG_DEBUG("Controller::addModel added model with name ", atomic->getName());
	    }
	} else {    //single core, don't look at what the allocator said, don't even run the allocator!
	    for (const t_atomicmodelptr& atomic : atomics) {
	        //size_t coreID = m_allocator->allocate(model);
	        addModel(atomic, 0);
	//      atomic->setKeepOldStates(m_simType == SimType::OPTIMISTIC);        // Can't use isParallell here (which returns true for cpdevs)
	        LOG_DEBUG("Controller::addModel added model with name ", atomic->getName());
	    }
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
	for (const auto& core : m_cores) {
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
        
        for (const auto& core : m_cores) {
		core->setTracers(m_tracers);
		core->init();
		if (m_checkTermTime)
			core->setTerminationTime(m_terminationTime);
		if (m_checkTermCond)
			core->setTerminationFunction(m_terminationCondition);

		core->setLive(true);
	}


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
	size_t i = 0;
        const auto& core = m_cores.front();
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
	this->m_rungvt.store(true);
        std::atomic<int> atint(m_cores.size());
        std::condition_variable cv;
        std::mutex mu;

	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
                                std::thread(cvworker, 
                                        i, m_turns, std::ref(*this), std::ref(atint), std::ref(mu), std::ref(cv))
                        );
		LOG_INFO("CONTROLLER: Started thread # ", i);
	}
        
	this->startGVTThread();
        
	for (auto& t : m_threads) {
		t.join();
	}
}

void Controller::simCPDEVS()
{	
        std::atomic<int> atint(m_cores.size());
        std::condition_variable cv;
        std::mutex mu;

	for (size_t i = 0; i < m_cores.size(); ++i) {
		m_threads.push_back(
		        std::thread(cvworker_con, i, m_turns, std::ref(*this), std::ref(atint), std::ref(mu),std::ref(cv))
                        );
		LOG_INFO("CONTROLLER: Started thread # ", i);
	}

	for (auto& t : m_threads) {
		t.join();
	}
}

void Controller::simDSDEVS()
{
	std::vector<n_model::t_raw_atomic> imminent;
	std::size_t i = 0;
        const auto& core = m_cores.front();
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
	std::thread charlie(&beginGVT, std::ref(*this), std::ref(m_rungvt));
	charlie.join();
	LOG_INFO("Controller:: GVT thread joined");
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
        LOG_DEBUG("Adding new atomic model during DS phase: ", model->getName());
        //it is an atomic model. Just remove this one from the core and the root devs
        if(!m_cores[0]->containsModel(model->getName())){
                m_cores.front()->addModelDS(atomic);
        }
        else{
                LOG_ERROR("Trying to add model that already exists in core ????", model->getName());
        }
        atomic->setKeepOldStates(false);
}

void Controller::dsUnscheduleModel(const n_model::t_atomicmodelptr& model)
{
	assert(isInDSPhase() && "Controller::dsUnscheduleModel called while not in the DS phase.");
	dsUndoDirectConnect();

	LOG_DEBUG("removing model: ", model->getName());
	//it is an atomic model. Just remove this one from the core
	m_cores.front()->removeModelDS(model->getLocalID());
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

void cvworker(std::size_t myid, std::size_t turns, Controller& ctrl, std::atomic<int>& atint, std::mutex& mu, std::condition_variable& cv)
{
        const auto& core = ctrl.m_cores[myid];
        auto at_exit = [&]()->void{
                core->setLive(false);
                LOG_DEBUG("Core ", core->getCoreID(), "exiting.");
                core->shutDown();
        };
        core->initThread();
        LOG_DEBUG("CVWORKER : TURNS == ctrl", ctrl.m_turns, " turns = ", turns);
        size_t i = 0;
        for (; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop
                if (core->getZombieRounds() > ctrl.m_zombieIdleThreshold) {
                        LOG_INFO("CVWORKER: Thread for core ", core->getCoreID(),
                                " Core is zombie, yielding thread. [round ", core->getZombieRounds(), "]");
                        core->setLive(false);
                }
                        
                
                if (!core->isLive()) {
                        bool quit = true;
                        for (const auto& coreentry : ctrl.m_cores) {
                                if (coreentry->isLive()) {
                                        quit = false;
                                        break;
                                }
                        }
                        if (quit) {              
                                if (!core->existTransientMessage()) {// If we've sent a message or there is one waiting, we can't quit (revert)
                                        LOG_INFO("CVWORKER: Thread ", std::this_thread::get_id(), " ", myid, " for core ", core->getCoreID(),
                                                " all other threads are stopped or idle, network is idle, quitting, gvt_run = false now.");
                                        ctrl.m_rungvt.store(false);
                                        break;
                                }else{
                                        LOG_INFO("CVWORKER: Thread ", myid, " for core ", core->getCoreID(),
                                " all other threads are stopped or idle, network still reports transients, idling.");
                                        if(!ctrl.m_rungvt){// If 1 or more cores have stopped, network can remain transient, we can't guarantee it emptying.
                                                LOG_INFO("CVWORKER: Thread ", myid, " for core ", core->getCoreID(), " at least one core has quit, have to exit as well.");
                                                break;
                                        }
                                }
                        }
                }
                LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " running simstep in round ", i,
                        " [zrounds:", core->getZombieRounds(), "]");
                core->runSmallStep();
        }
        ctrl.m_rungvt.store(false);             // Required to halt gvt.
         // Wait for all other cores to go idle.
        // Should a core have reached the nr of turns (a safety catch), make sure we set Live ourselves.
        if(i==turns){
                LOG_WARNING("CVWORKER: ", core->getCoreID(), " overran nr of simulation steps allowed !!");
                core->setLive(false);
        }
        
        LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " decreasing atint.");
        
        mu.lock(); 
        atint -=1;
        if(atint>0){
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " going to acquire lock.");
                mu.unlock();
                std::unique_lock<std::mutex> lk(mu);
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " acquired the lock.");
                cv.wait(lk,  [&atint]{
                                return (atint.load()<=0); // false if need to wait, meaning !i>0
                                }
                        );
        } else {
                mu.unlock(); 
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " notifying all.");
                cv.notify_all();
        }
        LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(),
                " exiting working function,  setting gvt intercept flag to false.");
        at_exit();
}

void cvworker_con(std::size_t myid, std::size_t turns, Controller& ctrl, std::atomic<int>& atint, std::mutex& mu, std::condition_variable& cv)
{
	const auto& core = ctrl.m_cores[myid];
        LOG_DEBUG("CVWORKER : TURNS == ctrl", ctrl.m_turns, " turns = ", turns);
        size_t i = 0;
        core->initThread();
	for (; i < turns; ++i) {		// Turns are only here to avoid possible infinite loop

		if (!core->isLive()) {          
                        LOG_DEBUG(" Core no longer live :: ", core->getCoreID(), " exiting working function."); // don't log time (^sync)
                        break;
		}
                LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " running simstep in round ", i );
                core->runSmallStep();
	}
        // Wait for all other cores to go idle.
        // Should a core have reached the nr of turns (a safety catch), make sure we set Live ourselves.
        if(i==turns){
                LOG_WARNING("CVWORKER: ", core->getCoreID(), " overran nr of simulation steps allowed !!");
                core->setLive(false);
                // Edge case : if we hit this, we can't guarantee deallocation since sender can't verify we have reached termination.
        }
        
        LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " decreasing atint.");
        
        // Atomic does not protect from any code coming between the update and the conditional check
        // Meaning that with i = 2 and 2 threads, i=1,i=0 and 2 threads evaluating @0 can occur.
        // Then add O3 and see what happens.
        // In short, any shared variable read/write needs the same lock if conditionals are involved.
        mu.lock(); // csect_begin
        atint -=1;
        if(atint>0){
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " going to acquire lock.");
                mu.unlock();//csect_end
                std::unique_lock<std::mutex> lk(mu);
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " acquired the lock.");
                cv.wait(lk,  [&atint]{
                                return (atint.load()<=0); // false if need to wait, meaning !i>0
                                }
                        );
        } else {
                mu.unlock(); //csect_end
                LOG_DEBUG("CVWORKER: Thread ", std::this_thread::get_id(), " for core ", core->getCoreID(), " notifying all.");
                cv.notify_all();
        }
        /**
        while(true){
                size_t i = 0;
                for(const auto& core : ctrl.m_cores){
                        if(!core->isLive())
                                ++i;
                }
                if(i == ctrl.m_cores.size())
                        break;
                else{
                        std::this_thread::yield();
                        i = 0;
                }
        }
        */
        LOG_DEBUG("CVWORKER: Thread for core ", core->getCoreID(), " exiting working function");
        core->shutDown();
}

void beginGVT(Controller& ctrl, std::atomic<bool>& m_rungvt)
{
	constexpr std::size_t infguard = 100;
	std::size_t i = 0;

	while(m_rungvt.load()==true){
		if(infguard < ++i){
			LOG_WARNING("Controller :: GVT overran max ", infguard, " nr of invocations, breaking off.");
			m_rungvt.store(false);
			break;
		}
		std::chrono::milliseconds ms { ctrl.getGVTInterval() };// Wait before running gvt, this prevents an obvious gvt of zero.
		std::this_thread::sleep_for(ms);
		LOG_INFO("Controller:: starting GVT");
		runGVT(ctrl, m_rungvt);
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
        const t_coreptr& first = cont.m_cores[0];
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
        if (gvtsafe == false) {
                LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
                return;
        }
        
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
		for (std::size_t j = 1; j < corecount; ++j){
			cont.m_cores[j]->receiveControl(cmsg, 1, gvtsafe);
                        if (gvtsafe == false) {
                                LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
                                return;
                        }       
                }
                if (gvtsafe == false) {
			LOG_INFO("Controller rungvt set to false by some Core thread, stopping GVT.");
			return;
		}
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
                        gvtsafe.store(false);
			LOG_ERROR("Controller : Algorithm did not find GVT in second round. Stopping invocations of algorithm.");
                        cmsg->logMessageState();
		}
	}
}

} /* namespace n_control */
