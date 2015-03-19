/*
 * trafficlight.h
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#ifndef SOURCE_DIRECTORY__SRC_EXAMPLES_TRAFFICLIGHT_CLASSIC_TRAFFICLIGHT_H_
#define SOURCE_DIRECTORY__SRC_EXAMPLES_TRAFFICLIGHT_CLASSIC_TRAFFICLIGHT_H_

#include "atomicmodel.h"
#include "state.h"
#include <assert.h>

namespace n_examples {

using namespace n_model;
using n_network::t_msgptr;

enum e_colors {RED, GREEN, YELLOW};

class TrafficLightMode: public State
{
public:
	e_colors m_color;

	TrafficLightMode(e_colors color = RED);
	std::string toString();
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

	void extTransition(const t_msgptr & message);
	void intTransition();
	t_timestamp timeAdvance();
	std::map<t_portptr, t_msgptr> output();
	std::string outputFnc();
};

}

#endif /* SOURCE_DIRECTORY__SRC_EXAMPLES_TRAFFICLIGHT_CLASSIC_TRAFFICLIGHT_H_ */
