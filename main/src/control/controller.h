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
//#include "model.h"	// TODO uncomment after models are complete.
#include "port.h"
#include "locationtable.h"
#include "core.h"

using n_model::t_coreptr;

namespace n_control {

using n_network::t_timestamp;

class Controller
{
public:
	Controller(std::string name);

	virtual ~Controller();

	void addModel(const t_modelptr& model);

	void simulate();

	void setClassicDEVS(bool classicDEVS = true);

	void setTerminationTime(t_timestamp time);

	void setTerminationCondition(std::function<bool(t_timestamp,const t_modelptr&)> termination_condition);

//	void save(std::string filepath, std::string filename) = delete;
//	void load(std::string filepath, std::string filename) = delete;
//	void setDSDEVS(bool dsdevs = true);
//	void GVTdone();
//	void startGVTThread(n_network::Time gvt_interval);
//	void waitFinish(uint amntRunningKernels);
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

//	bool isFinished(uint amntRunningKernels);
//	void threadGVT(n_network::Time freq);

private:
	bool m_isClassicDEVS;
	std::string m_name;
	bool m_checkTermTime;
	t_timestamp m_terminationTime;
	bool m_checkTermCond;
	std::function<bool(t_timestamp, const t_modelptr&)> m_terminationCondition;
	std::unordered_map<std::size_t, t_coreptr> m_cores;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
