/*
 * trafficlight.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "trafficlight.h"

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
	: AtomicModel(name, 0, priority)
{
	this->setState(std::make_shared<TrafficLightMode>("Red"));
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
	if (*state == "Red")
		this->setState("Green");
	else if(*state == "Green")
		this->setState("Yellow");
	else if (*state == "Yellow")
		this->setState("Red");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp TrafficLight::timeAdvance()
{
	t_stateptr state = this->getState();
	if (*state == "Red")
		return t_timestamp(60);
	else if(*state == "Green")
		return t_timestamp(50);
	else if (*state == "Yellow")
		return t_timestamp(10);
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

std::vector<n_network::t_msgptr> TrafficLight::output()
{
	return std::vector<n_network::t_msgptr>();
}

t_stateptr TrafficLight::setState(std::string s) {
	this->Model::setState(std::make_shared<TrafficLightMode>(s));
	return this->getState();
}

}

