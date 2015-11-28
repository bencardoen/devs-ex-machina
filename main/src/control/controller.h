/*
 * Controller.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONTROLLER_H_
#define SRC_CONTROL_CONTROLLER_H_

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>
#include "network/timestamp.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/rootmodel.h"
#include "model/port.h"
#include "control/allocator.h"
#include "model/core.h"
#include "tracers/tracers.h"
#include "tools/globallog.h"
#include "model/dssharedstate.h"
#include "control/simtype.h"
#include "tools/statistic.h"

namespace n_control {

using n_network::t_timestamp;
using n_model::t_coreptr;
using n_model::t_atomicmodelptr;
using n_model::t_coupledmodelptr;

enum CTRLSTAT_TYPE{GVT_2NDRND,GVT_FOUND,GVT_START,GVT_FAILED};

/**
 * @brief Provides control over a simulation
 */
class Controller
{
private:
	SimType m_simType;
	bool m_hasMainModel;
	bool m_isSimulating;

	std::string m_name;

	bool m_checkTermTime;
	t_timestamp m_terminationTime;
	bool m_checkTermCond;
	t_terminationfunctor m_terminationCondition;
	size_t m_saveInterval;
	std::atomic<size_t> m_zombieIdleThreshold;

	std::vector<t_coreptr> m_cores;
	std::shared_ptr<Allocator> m_allocator;
	n_model::RootModel m_root;
	t_atomicmodelptr m_atomicOrigin;
	t_coupledmodelptr m_coupledOrigin;
	n_tracers::t_tracersetptr m_tracers;
	t_timestamp m_lastGVT;

	DSSharedState m_sharedState;
	bool m_dsPhase;

	std::vector<std::thread> m_threads;

	/**
	 * Interval time (in milliseconds) during which consecutive gvt calculations are performed.
	 * A GVT thread will run [5ms |run| interval | interval | ... ].
	 * @attention : The OS will schedule at least interval sleep time, but more is of course possible.
	 * @synchronized
	 */
	std::atomic<std::size_t> m_sleep_gvt_thread;

	/**
	 * Shared memory flag. Used to signal between threads simulating Cores and
	 * the GVT thread whether or not the last should continue.
	 * False means interrupt at earliest possible time to do so cleanly.
	 */
	std::atomic<bool> 	m_rungvt;
        
        /**
         * Designate how many (possibly including idle) rounds any core can run.
         */
        std::size_t             m_turns;
        
        /// Add keyword inline, if we can't use __attribute(pure)__, 
        /// inline + ifdef will convince compiler the function is empty, and throw it
        // away if we're nog statlogging.
        inline
        void logStat(enum CTRLSTAT_TYPE);

public:
	Controller(std::string name, std::vector<t_coreptr>& cores,
		std::shared_ptr<Allocator>& alloc,
		n_tracers::t_tracersetptr& tracers, size_t traceInterval = 5, std::size_t turns=10000000);

	virtual ~Controller();

	/**
	 * @brief Set an atomic model as the main model using the given allocator
	 */
	void addModel(const t_atomicmodelptr& atomic);

	/**
	 * @brief Set a coupled model as the main model using the given allocator
	 */
	void addModel(const t_coupledmodelptr& coupled);

	/**
	 * @brief Main loop, starts simulation
	 */
	void simulate();
        
        Core* getCore(size_t id){return m_cores[id].get();}

	/**
	 * @brief Sets the simulation type
	 */
	void setSimType(SimType type);

	/**
	 * @brief Set time at which the simulation will be halted
	 */
	void setTerminationTime(t_timestamp time);

	/**
	 * @brief Set condition that can terminate the simulation
	 */
	void setTerminationCondition(t_terminationfunctor termination_condition);

	/**
	 * Update the GVT interval with a new value.
	 */
	void setGVTInterval(std::size_t ms);

	/**
	 * Return the current GVT threading interval.
	 */
	std::size_t
	getGVTInterval();

	/**
	 * @brief Start thread for GVT
	 */
	void startGVTThread();

	/**
	 * @brief Adds a connection during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 * @param from The starting port of the connection
	 * @param to The destination port of the connection
	 */
	void dsAddConnection(const n_model::t_portptr& from, const n_model::t_portptr& to, t_zfunc zFunction);
	/**
	 * @brief Removes a connection during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 * @param from The starting port of the connection
	 * @param to The destination port of the connection
	 */
	void dsRemoveConnection(const n_model::t_portptr& from, const n_model::t_portptr& to);
	/**
	 * @brief Remove a port during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsRemovePort(const n_model::t_portptr& port);
	/**
	 * @brief add a model during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsScheduleModel(const n_model::t_modelptr& model);
	/**
	 * @brief remove a model during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsUnscheduleModel(const n_model::t_atomicmodelptr& model);
	/**
	 * @brief Undo direct connect during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsUndoDirectConnect();

	/**
	 * @return Whether or not the simulator is currently performing Dynamic Structured DEVS
	 */
	bool isInDSPhase() const;

private:
	/**
	 * @brief Check if simulation needs to continue
	 */
	bool check();

	/**
	 * @brief Simulation setup and loop using regular DEVS
	 */
	void simDEVS();

	/**
	 * @brief Simulation setup and loop using Parallel DEVS
	 */
	void simOPDEVS();
        
        void simCPDEVS();

	/**
	 * @brief Simulation setup and loop using Dynamic Structure DEVS
	 */
	void simDSDEVS();

	/**
	 * @brief Removes models from all cores
	 */
	void emptyAllCores();

	/**
	 * @brief Add an atomic model to a specific core
	 */
	void addModel(const t_atomicmodelptr& atomic, std::size_t coreID);

	void doDirectConnect();
	void doDSDevs(std::vector<n_model::t_raw_atomic>& imminent);

	/**
	 * If a core triggers a termination functor, it will pass its current time to
	 * the controller as the new termination time for the other cores.
	 * Example scenario: C1: triggers f() @ 101, C2 is @ 200. C2 will keep simulating (f()) need
	 * not evaluate to true on C2.
	 */
	void distributeTerminationTime(t_timestamp);

	friend
	void runGVT(Controller&, std::atomic<bool>& rungvt);

	friend
	void cvworker( std::size_t myid, std::size_t turns,Controller&, std::atomic<int>& atint, std::mutex& mu, std::condition_variable& cv);
        
        friend
	void cvworker_con( std::size_t myid, std::size_t turns,Controller&, std::atomic<int>&, std::mutex& mu, std::condition_variable& cv);

#ifdef USE_VIZ
public:
        void visualize(){
                LOG_DEBUG("Controller :: have ", m_cores.size(), " cores");
                for(const auto& core : m_cores){
                        core->writeGraph();
                }
                delete n_tools::GVizWriter::getWriter("sim.dot");
        }
#endif
        
//-------------statistics gathering--------------
//#ifdef USE_STAT
private:
	n_tools::t_uintstat m_gvtStarted;
	n_tools::t_uintstat m_gvtSecondRound;
	n_tools::t_uintstat m_gvtFailed;
	n_tools::t_uintstat m_gvtFound;
public:
	void printStats(std::ostream& out = std::cout) const
	{
		out << m_gvtStarted
			<< m_gvtSecondRound
			<< m_gvtFound
			<< m_gvtFailed;
                
		for(const auto& i:m_cores){
			i->printStats(out);
                }
                
	}
//#endif
};

void beginGVT(Controller&, std::atomic<bool>& rungvt);

/**
 * Find GVT using Mattern's algorithm.
 * @param rungvt : thread interrupt flag.
 */
void runGVT(Controller&, std::atomic<bool>& rungvt);

/**
 * Worker function. Runs a Core and communicates with other threads and GVT thread.
 * @param myid unique identifier, for logging it is best this is equal to coreid
 * @param turns : infinite loop cutoff value.
 */
void cvworker(std::size_t myid,std::size_t turns,Controller&, std::atomic<int>& atint, std::mutex& mu, std::condition_variable& cv);

/**
 * Worker function. Runs a Core and communicates with other threads and GVT thread.
 * @param myid unique identifier, for logging it is best this is equal to coreid
 * @param turns : infinite loop cutoff value.
 * @param at : threadcount value, initialized to nr of thread running this function.
 * @param mu/cv : cvar/mu pair.
 */
void cvworker_con(std::size_t myid,std::size_t turns,Controller&, std::atomic<int>& at, std::mutex& mu, std::condition_variable& cv);

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
