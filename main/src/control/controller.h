/*
 * Controller.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONTROLLER_H_
#define SRC_CONTROL_CONTROLLER_H_

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <condition_variable>
#include "network/timestamp.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/rootmodel.h"
#include "model/port.h"
#include "control/locationtable.h"
#include "control/allocator.h"
#include "model/core.h"
#include "tracers/tracers.h"
#include "tools/globallog.h"
#include "model/dssharedstate.h"
#include "control/timeevent.h"
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
	bool m_isLoadedSim;	/// Whether this is a simulation reinitialized from a binary save

	std::string m_name;

	bool m_checkTermTime;
	t_timestamp m_terminationTime;
	bool m_checkTermCond;
	t_terminationfunctor m_terminationCondition;
	size_t m_saveInterval;
	std::atomic<int> m_zombieIdleThreshold;

	std::unordered_map<std::size_t, t_coreptr> m_cores;
	t_location_tableptr m_locTab;
	std::shared_ptr<Allocator> m_allocator;
	std::shared_ptr<n_model::RootModel> m_root;
	t_atomicmodelptr m_atomicOrigin;
	t_coupledmodelptr m_coupledOrigin;
	n_tracers::t_tracersetptr m_tracers;
	TimeEventQueue m_events;
	t_timestamp m_lastGVT;

	DSSharedState m_sharedState;
	bool m_dsPhase;

	std::vector<std::thread> m_threads;

	/**
	 * Interval time (in milliseconds) during which consecutive gvt calculations are performed.
	 * A GVT thread will run [5ms |run| interval | interval | ... ].
	 * @attention : The OS will schedule at least interval sleep time, but more is of course possible.
	 * @synchronized
	 * @default value = 85 (ms). A lower value starves threads/increases CPU, a higher value uses more VMEM.
	 */
	std::atomic<std::size_t> m_sleep_gvt_thread;

	/**
	 * Shared memory flag. Used to signal between threads simulating Cores and
	 * the GVT thread whether or not the last should continue.
	 * False means interrupt at earliest possible time to do so cleanly.
	 */
	std::atomic<bool> 	m_rungvt;
        
        /// Add keyword inline, if we can't use __attribute(pure)__, 
        /// inline + ifdef will convince compiler the function is empty, and throw it
        // away if we're nog statlogging.
        inline
        void logStat(enum CTRLSTAT_TYPE);

public:
	Controller(std::string name, std::unordered_map<std::size_t, t_coreptr>& cores,
		std::shared_ptr<Allocator>& alloc, std::shared_ptr<LocationTable>& locTab,
		n_tracers::t_tracersetptr& tracers, size_t traceInterval = 5);

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
	 * @brief Serialize all models
	 * @precondition All cores need to be stopped beforehand
	 */
	void save(const std::string& fname, const t_timestamp& time);

	/**
	 * @brief Load all models
	 * @param isSingleAtomic : Whether nor not the simulation was of a single atomic model, false by default
	 */
	void load(const std::string& fname, bool isSingleAtomic = false);

	/**
	 * @brief Main loop, starts simulation
	 */
	void simulate();

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
	 * @brief Add a moment during which the simulation will pause for a certain time
	 */
	void addPauseEvent(t_timestamp time, size_t duration, bool repeating = false);

	/**
	 * @brief Add a moment on which the simulation will be paused, saved and continued
	 */
	void addSaveEvent(t_timestamp time, std::string prefix, bool repeating = false);

	/**
	 * Return the current GVT threading interval.
	 */
	std::size_t
	getGVTInterval();

	/**
	 * @brief Start thread for GVT
	 */
	void startGVTThread();

//	void checkForTemporaryIrreversible();

	/**
	 * @brief Adds a connection during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 * @param from The starting port of the connection
	 * @param to The destination port of the connection
	 */
	void dsAddConnection(const n_model::t_portptr& from, const n_model::t_portptr& to, const t_zfunc& zFunction);
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

	/**
	 * @brief forces "zombie" cores to go IDLE
	 */
	void setZombieIdleThreshold(int threshold = -1);

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

	/**
	 * @brief Handle all time events until now, returns whether the simulation should continue
	 * @attention This method should only be used in CLASSIC or DSDEVS mode
	 */
	void handleTimeEventsSingle(const t_timestamp& now);

	/**
	 * @brief Handle all time events until now, returns whether the simulation should continue
	 * @attention This method should only be used in PDEVS mode
	 */
	bool handleTimeEventsParallel(std::condition_variable& cv, std::mutex& cvlock);

	void doDirectConnect();
	void doDSDevs(std::vector<n_model::t_atomicmodelptr>& imminent);

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
	void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid,
	        std::vector<std::size_t>& threadsignal, std::mutex& vectorlock, std::size_t turns,
	        Controller&);


//-------------statistics gathering--------------
//#ifdef USESTAT
private:
	n_tools::t_intstat m_gvtStarted;
	n_tools::t_intstat m_gvtSecondRound;
	n_tools::t_intstat m_gvtFailed;
	n_tools::t_intstat m_gvtFound;
public:
	void printStats(std::ostream& out = std::cout) const
	{
		out << m_gvtStarted << '\n';
		out << m_gvtSecondRound << '\n';
		out << m_gvtFound << '\n';
		out << m_gvtFailed << '\n';
		for(const auto& i:m_cores)
			i.second->printStats(out);
	}
//#endif
};

/**
 * Find GVT using Mattern's algorithm.
 * @param rungvt : thread interrupt flag.
 */
void runGVT(Controller&, std::atomic<bool>& rungvt);

/**
 * Worker function. Runs a Core and communicates with other threads and GVT thread.
 * @param cv Queues working threads if main asks them to.
 * @param cvlock lock needed for cv
 * @param myid unique identifier, for logging it is best this is equal to coreid
 * @param threadsignal : stores thread signalling flags
 * @param vectorlock : lock threadsignal
 * @param turns : infinite loop cutoff value.
 */
void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid,
        std::vector<std::size_t>& threadsignal, std::mutex& vectorlock, std::size_t turns,
        Controller&);

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
