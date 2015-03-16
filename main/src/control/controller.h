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
#include "../network/timestamp.h"
#include "../model/model.h"
#include "../model/port.h"
#include "locationtable.h"

// TEMPORARY:
// FIXME
namespace n_core {
class Core;
typedef int t_coreID;
} /* namespace n_core */

namespace n_control {

class Controller
{
	typedef std::shared_ptr<n_model::Model> t_modelPtr;
public:
	Controller(std::string name);

	virtual ~Controller();

	void addModel(t_modelPtr model);

	void simulate();

	void setClassicDEVS(bool classicDEVS = true);

	void setTerminationTime(n_network::Time time);

	void setTerminationCondition(std::function<bool(n_network::Time, n_model::Model)> termination_condition);

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
	n_network::Time m_terminationTime;
	bool m_checkTermCond;
	std::function<bool(n_network::Time, n_model::Model)> m_terminationCondition;
	std::unordered_map<n_core::t_coreID, n_core::Core> m_cores;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
