/*
 * trafficlight.h
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_TRAFFICLIGHT_H_
#define SRC_EXAMPLES_TRAFFICLIGHT_H_

#include "atomicmodel.h"
#include "state.h"
#include <assert.h>

namespace n_examples {

using namespace n_model;
using n_network::t_msgptr;

class TrafficLightMode: public State
{
public:
	TrafficLightMode(std::string state);
	std::string toXML();
	std::string toJSON();
	std::string toCell();
	~TrafficLightMode() {}
};

class TrafficLight: public AtomicModel
{
public:
	TrafficLight() = delete;
	TrafficLight(std::string, std::size_t priority = 0);
	~TrafficLight() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message);
	void intTransition();
	t_timestamp timeAdvance();
	std::vector<n_network::t_msgptr> output();
};

}

#endif /* SRC_EXAMPLES_TRAFFICLIGHT_H_ */
