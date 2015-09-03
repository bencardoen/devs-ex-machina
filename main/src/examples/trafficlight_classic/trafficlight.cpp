/*
 * trafficlight.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "examples/trafficlight_classic/trafficlight.h"

namespace n_examples {

TrafficLightMode::TrafficLightMode(std::string state)
	: State(state)
{

}

std::string TrafficLightMode::toXML()
{
	return "<state color =\"" + this->toString() + "\"/>";
}

std::string TrafficLightMode::toJSON()
{
	return "{ \"state\": \"" + this->toString() + "\" }";
}

std::string TrafficLightMode::toCell()
{
	return "";
}

TrafficLight::TrafficLight(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	this->setState(n_tools::createObject<TrafficLightMode>("red"));
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
	t_stateptr state = this->getState();
	if (*state == "red")
		this->setState("green");
	else if (*state == "green")
		this->setState("yellow");
	else if (*state == "yellow")
		this->setState("red");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp TrafficLight::timeAdvance() const
{
	t_stateptr state = this->getState();
	if (*state == "red")
		return t_timestamp(60);
	else if (*state == "green")
		return t_timestamp(50);
	else if (*state == "yellow")
		return t_timestamp(10);
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

void TrafficLight::output(std::vector<n_network::t_msgptr>&) const
{
	//nothing to do here
}

t_stateptr TrafficLight::setState(const std::string& s)
{
	this->Model::setState(n_tools::createObject<TrafficLightMode>(s));
	return this->getState();
}

}

