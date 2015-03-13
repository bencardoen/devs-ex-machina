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
#include "../network/Time.h"

// TEMPORARY:
// FIXME
namespace n_model {
class Model;
class Port;
} /* namespace n_model */

namespace n_control {

class Controller
{
	typedef std::shared_ptr<n_model::Model> t_modelPtr;
public:
	Controller(const std::string& name, t_modelPtr model);
	virtual ~Controller();

	void save(std::string filepath, std::string filename);
	void load(std::string filepath, std::string filename);
	void setClassicDEVS(bool classicDEVS);
	void setDSDEVS(bool dsdevs);
	void setTerminationCondition(std::function<bool(n_network::Time, n_model::Model)> termination_condition);
	void simulate();
	void GVTdone();
	void startGVTThread(n_network::Time gvt_interval);
	// FIXME void stateChange(uint model_id, std::string variable, value);
	void waitFinish(uint amntRunningKernels);
	void checkForTemporaryIrreversible();
	void dsRemovePort(const n_model::Port& port);
	void dsScheduleModel(const t_modelPtr model);
	void dsUndoDirectConnect();
	void dsUnscheduleModel(const t_modelPtr model);

private:
	bool isFinished(uint amntRunningKernels);
	void threadGVT(n_network::Time freq);
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
