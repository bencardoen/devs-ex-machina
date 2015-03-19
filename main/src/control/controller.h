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
#include "port.h"
#include "locationtable.h"
#include "allocator.h"
#include "core.h"

namespace n_control {

using n_network::t_timestamp;
using n_model::t_coreptr;
using n_model::t_atomicmodelptr;
using n_model::t_coupledmodelptr;

class Controller
{
private:
	bool m_isClassicDEVS;
	std::string m_name;
	t_timestamp m_checkpointInterval;
	bool m_checkTermTime;
	t_timestamp m_terminationTime;
	bool m_checkTermCond;
	std::function<bool(t_timestamp, const t_atomicmodelptr&)> m_terminationCondition;
	std::unordered_map<std::size_t, t_coreptr> m_cores;
	std::shared_ptr<LocationTable> m_locTab;
	std::shared_ptr<Allocator> m_allocator;

public:
	Controller(std::string name, std::unordered_map<std::size_t, t_coreptr> cores,
	        std::shared_ptr<LocationTable> locTab);

	virtual ~Controller();

	/*
	 * Add an atomic model
	 */
	void addModel(t_atomicmodelptr& atomic);

	/*
	 * Add an atomic model to a specific core
	 */
	void addModel(t_atomicmodelptr& atomic, std::size_t coreID);

	/*
	 * Add a coupled model to the simulation
	 */
	void addModel(t_coupledmodelptr& coupled);

	/*
	 * Main loop, starts simulation
	 */
	void simulate();

	/*
	 * Set simulation to be classic DEVS
	 */
	void setClassicDEVS(bool classicDEVS = true);

	/*
	 * Set time at which the simulation will be halted
	 */
	void setTerminationTime(t_timestamp time);

	/*
	 * Set condition that can terminate the simulation
	 */
	void setTerminationCondition(std::function<bool(t_timestamp, const t_atomicmodelptr&)> termination_condition);

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
//	void setDSDEVS(bool dsdevs = true);
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
	 * Flattens coupled model to interconnected atomic models
	 */
	std::vector<t_atomicmodelptr> directConnect(t_coupledmodelptr coupled);

//	bool isFinished(uint amntRunningKernels);
//	void threadGVT(n_network::Time freq);
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
