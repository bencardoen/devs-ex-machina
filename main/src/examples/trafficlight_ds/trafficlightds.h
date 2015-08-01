/*
 * trafficlightds.h
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_EXAMPLES_TRAFFICLIGHTDS_H_
#define SRC_EXAMPLES_TRAFFICLIGHTDS_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_ds {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_network::t_timestamp;

class TrafficLightMode: public State
{
public:
	TrafficLightMode(std::string state);
	std::string toXML() override;
	std::string toJSON() override;
	std::string toCell() override;
	~TrafficLightMode() {}
};

class TrafficLight: public AtomicModel
{
public:
	TrafficLight() = delete;
	TrafficLight(std::string, std::size_t priority = 0);
	~TrafficLight() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

}

#endif /* SRC_EXAMPLES_TRAFFICLIGHTDS_H_ */
