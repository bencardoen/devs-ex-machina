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

namespace n_control {

class Controller
{
public:
	Controller(const std::string& name, Model& model);
	virtual ~Controller();

	void save(std::string filepath, std::string filename);
	void load(std::string filepath, std::string filename);
	void setClassicDEVS(bool classicDEVS);
	void setDSDEVS(bool dsdevs);
	void setTerminationCondition(std::function<Time, Model> termination_condition);
	void simulate();
	void GVTdone();
	void startGVTThread(Time gvt_interval);
	void stateChange(uint model_id, std::string variable, value);
	void waitFinish(uint amntRunningKernels);
	void checkForTemporaryIrreversible();
	void dsRemovePort(const Port& port);
	void dsScheduleModel(const Model& model);
	void dsUndoDirectConnect();
	void dsUnscheduleModel(const Model& model);

private:
	bool isFinished(uint amntRunningKernels);
	void threadGVT(Time freq);
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONTROLLER_H_ */
