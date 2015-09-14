/*
 * trafficlight.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "examples/trafficlight_classic/trafficlight.h"

namespace n_examples {

TrafficLightMode::TrafficLightMode(std::string state)
	: m_value(state)
{
}

TrafficLight::TrafficLight(std::string name, std::size_t priority)
	: AtomicModel(name, TrafficLightMode("red"), priority)
{
//	this->setState(n_tools::createObject<TrafficLightMode>("red"));
	//this->setState(std::make_shared<TrafficLightMode>("red"));
	// Initialize elapsed attribute if required
	m_elapsed = 0;
}

void TrafficLight::extTransition(const std::vector<n_network::t_msgptr> &)
{
	// No external transitions available yet...
}

void TrafficLight::intTransition()
{
	TrafficLightMode& mode = state();
	if (mode.m_value == "red")
		mode.m_value = "green";
	else if (mode.m_value == "green")
		mode.m_value = "yellow";
	else if (mode.m_value == "yellow")
		mode.m_value = "red";
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp TrafficLight::timeAdvance() const
{
	const TrafficLightMode& mode = state();
	if (mode.m_value == "red")
		return t_timestamp(60);
	else if (mode.m_value == "green")
		return t_timestamp(50);
	else if (mode.m_value == "yellow")
		return t_timestamp(10);
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

void TrafficLight::output(std::vector<n_network::t_msgptr>&) const
{
	//nothing to do here
}

}

