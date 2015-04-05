/*
 * trafficlightc.h
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_TRAFFICLIGHTC_H_
#define SRC_EXAMPLES_TRAFFICLIGHTC_H_

#include "atomicmodel.h"
#include "state.h"
#include <assert.h>

namespace n_examples_coupled {

using namespace n_model;
using n_network::t_msgptr;

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
	t_timestamp timeAdvance() override;
	std::vector<n_network::t_msgptr> output() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

}

#endif /* SRC_EXAMPLES_TRAFFICLIGHTC_H_ */
