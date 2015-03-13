/*
 * Configuration.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONFIGURATION_H_
#define SRC_CONTROL_CONFIGURATION_H_

#include "Controller.h"

//TEMPORARY:
//FIXME
namespace n_model {
class State;
} /* namespace n_model*/

namespace n_control {

enum Formalism {};

class Configuration
{
	typedef std::shared_ptr<Controller> t_ctrlrPtr;
	typedef std::shared_ptr<n_model::Model> t_modelPtr;
public:
	Configuration(t_ctrlrPtr ctrlr);
	virtual ~Configuration();

	Formalism getFormalism();

	void setClassicDEVS(bool classic = true);
	void setDSDEVS(bool dsdevs = true);
	// TODO void setScheduler...
	// TODO void setTracer...
	// TODO void setAllocator...
	void setTerminationTime(n_network::Time time);
	void removeTracers();
	void setCheckpointing(std::string filename, n_network::Time interval);
	void setModelState(t_modelPtr model, n_model::State newState);
	// FIXME void setModelStateAttr(t_modelPtr model, n_model::State newState, <attribute>);
	// FIXME void setModelAttr(t_modelPtr model, <attribute>, <value>);
private:
	Controller m_ctrlr;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONFIGURATION_H_ */
