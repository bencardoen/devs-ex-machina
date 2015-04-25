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
#include "timestamp.h"
#include "atomicmodel.h"
#include "coupledmodel.h"
#include "rootmodel.h"
#include "port.h"
#include "locationtable.h"
#include "allocator.h"
#include "core.h"
#include "tracers.h"
#include "globallog.h"
#include "dssharedstate.h"

namespace n_control {

using n_network::t_timestamp;
using n_model::t_coreptr;
using n_model::t_atomicmodelptr;
using n_model::t_coupledmodelptr;

class Controller
{
public:
	/// The type of simulation
	enum SimType
	{
		CLASSIC, 	///< Classic DEVS
		PDEVS, 		///< Parallel DEVS
		DSDEVS 		///< Dynamic Structure DEVS
	};
	enum class ThreadSignal
	{
		ISWAITING, SHOULDWAIT, ISFINISHED, FREE
	};

private:
	SimType m_simType;
	bool m_hasMainModel;
	bool m_isSimulating;

	std::string m_name;

	t_timestamp m_checkpointInterval;
	bool m_checkTermTime;
	t_timestamp m_terminationTime;
	bool m_checkTermCond;
	t_terminationfunctor m_terminationCondition;
	size_t m_saveInterval;

	std::unordered_map<std::size_t, t_coreptr> m_cores;
	t_location_tableptr m_locTab;
	std::shared_ptr<Allocator> m_allocator;
	std::shared_ptr<n_model::RootModel> m_root;
	t_coupledmodelptr m_coupledOrigin;
	n_tracers::t_tracersetptr m_tracers;

	DSSharedState m_sharedState;
	bool m_dsPhase;

	void doDirectConnect();
	void doDSDevs(std::vector<n_model::t_atomicmodelptr>& imminent);
	std::vector<std::shared_ptr<std::thread>> m_threads;

public:
	Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores, std::shared_ptr<Allocator> alloc,
	        std::shared_ptr<LocationTable> locTab, n_tracers::t_tracersetptr tracers, size_t traceInterval = 5);

	virtual ~Controller();

	/**
	 * @brief Set an atomic model as the main model using the given allocator
	 */
	void addModel(t_atomicmodelptr& atomic);

	/**
	 * @brief Set a coupled model as the main model using the given allocator
	 */
	void addModel(t_coupledmodelptr& coupled);

	/**
	 * @brief Main loop, starts simulation
	 */
	void simulate();

	/**
	 * @brief Sets the simulation type
	 */
	void setSimType(SimType type);

	/**
	 * @brief Set simulation to be classic DEVS
	 */
	void setClassicDEVS();

	/**
	 * @brief Set simulation to be Parallel DEVS
	 */
	void setPDEVS();

	/**
	 * @brief Set simulation to be Dynamic Structure DEVS
	 */
	void setDSDEVS();

	/**
	 * @brief Set time at which the simulation will be halted
	 */
	void setTerminationTime(t_timestamp time);

	/**
	 * @brief Set condition that can terminate the simulation
	 */
	void setTerminationCondition(t_terminationfunctor termination_condition);

	/**
	 * @brief Set checkpointing interval
	 */
	void setCheckpointInterval(t_timestamp interv);

	/**
	 * @brief Start thread for GVT
	 */
	void startGVTThread();

//	void save(std::string filepath, std::string filename) = delete;
//	void load(std::string filepath, std::string filename) = delete;
//	void GVTdone();
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
	void dsRemovePort(n_model::t_portptr& port);
	/**
	 * @brief add a model during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsScheduleModel(const n_model::t_modelptr& model);
	/**
	 * @brief remove a model during Dynamic Structured DEVS
	 * @preconditions We are in the Dynamic Structured phase.
	 */
	void dsUnscheduleModel(n_model::t_atomicmodelptr& model);
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
	 * @brief Serialize all cores and models, dump tracer output
	 */
	void save(bool traceOnly = false);

	/**
	 * @brief Simulation setup and loop using regular DEVS
	 */
	void simDEVS();

	/**
	 * @brief Simulation setup and loop using Parallel DEVS
	 */
	void simPDEVS();

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
	void addModel(t_atomicmodelptr& atomic, std::size_t coreID);

//	void threadGVT(n_network::Time freq);
};

void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid,
        std::vector<Controller::ThreadSignal>& threadsignal, std::mutex& vectorlock, std::size_t turns,
        const t_coreptr& core, size_t saveInterval);

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
