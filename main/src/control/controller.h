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

namespace n_control {

using n_network::t_timestamp;
using n_model::t_coreptr;
using n_model::t_atomicmodelptr;
using n_model::t_coupledmodelptr;

class Controller
{
public:
	enum SimType {CLASSIC, PDEVS, DSDEVS};
	enum class ThreadSignal{ISWAITING, SHOULDWAIT, ISFINISHED, FREE};

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
	n_tracers::t_tracersetptr m_tracers;
	std::vector<std::shared_ptr<std::thread>> m_threads;

public:
	Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
		std::shared_ptr<Allocator> alloc, std::shared_ptr<LocationTable> locTab,
		n_tracers::t_tracersetptr tracers, size_t traceInterval = 5);

	virtual ~Controller();

	/*
	 * Set an atomic model as the main model using the given allocator
	 */
	void addModel(t_atomicmodelptr& atomic);

	/*
	 * Set a coupled model as the main model using the given allocator
	 */
	void addModel(t_coupledmodelptr& coupled);

	/*
	 * Main loop, starts simulation
	 */
	void simulate();

	/*
	 * Set simulation to be classic DEVS
	 */
	void setClassicDEVS();

	/*
	 * Set simulation to be Parallel DEVS
	 */
	void setPDEVS();

	/*
	 * Set simulation to be Dynamic Structure DEVS
	 */
	void setDSDEVS();

	/*
	 * Set time at which the simulation will be halted
	 */
	void setTerminationTime(t_timestamp time);

	/*
	 * Set condition that can terminate the simulation
	 */
	void setTerminationCondition(t_terminationfunctor termination_condition);

	/*
	 * Set checkpointing interval
	 */
	void setCheckpointInterval(t_timestamp interv);

	/*
	 * Start thread for GVT
	 */
	void startGVTThread();

	/*
	 * Waits until all cores are finished
	 */
	void waitFinish(size_t runningCores);

//	void save(std::string filepath, std::string filename) = delete;
//	void load(std::string filepath, std::string filename) = delete;
//	void GVTdone();
//	void checkForTemporaryIrreversible();
//	void dsRemovePort(const n_model::Port& port);
//	void dsScheduleModel(const t_modelPtr model);
//	void dsUndoDirectConnect();
//	void dsUnscheduleModel(const t_modelPtr model);

private:
	/*
	 * Check if simulation needs to continue
	 */
	bool check();

	/*
	 * Serialize all cores and models, dump tracer output
	 */
	void save(bool traceOnly = false);

	/*
	 * Simulation setup and loop using regular DEVS
	 */
	void simDEVS();

	/*
	 * Simulation setup and loop using Parallel DEVS
	 */
	void simPDEVS();

	/*
	 * Simulation setup and loop using Dynamic Structure DEVS
	 */
	void simDSDEVS();

	/*
	 * Removes models from all cores
	 */
	void emptyAllCores();

	/*
	 * Add an atomic model to a specific core
	 */
	void addModel(t_atomicmodelptr& atomic, std::size_t coreID);

//	void threadGVT(n_network::Time freq);
};

void cvworker(std::condition_variable& cv, std::mutex& cvlock, std::size_t myid,
	        std::vector<Controller::ThreadSignal>& threadsignal, std::mutex& vectorlock, std::size_t turns,
	        const t_coreptr& core, size_t saveInterval);

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
