/*
 * trafficlightds.cpp
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "trafficlightds.h"

namespace n_examples_ds {

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
	this->setState(std::make_shared<TrafficLightMode>("red"));
	// Initialize elapsed attribute if required
	m_elapsed = 2;

	this->addInPort("INTERRUPT");
	this->addOutPort("OBSERVED");
}

void TrafficLight::extTransition(const std::vector<n_network::t_msgptr> & inputs)
{
	auto input = inputs.at(0);

	t_stateptr state = this->getState();

	if (input->getPayload() == "toManual") {
		if (*state == "manual")
			this->setState("manual"); // Keep light on manual
		else if (*state == "red" || *state == "green" || *state == "yellow")
			this->setState("manual"); // Set light to manual
	} else if (input->getPayload() == "toAutonomous") {
		if (*state == "manual")
			this->setState("red"); // Restart with a red light
		else if (*state == "red" || *state == "green" || *state == "yellow")
			this->setState(state->m_state); // Keep the same light
	}
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
	else if (*state == "manual")
		return t_timestamp::infinity();
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

std::vector<n_network::t_msgptr> TrafficLight::output() const
{
	// BEFORE USING THIS FUNCTION
	// READ EXAMPLE @ PYTHONPDEVS trafficlight_classic/model.py line 117 ( def outputFnc(self):)
	t_stateptr state = this->getState();
	std::string message = "";

	if (*state == "red")
		message = "grey";
	else if (*state == "green")
		message = "yellow";
	else if (*state == "yellow")
		message = "grey";
	else  // nothing happens
		return std::vector<n_network::t_msgptr>();
	LOG_DEBUG("TrafficLight::output message = ", message);
	return this->getPort("OBSERVED")->createMessages(message);
}

t_stateptr TrafficLight::setState(std::string s)
{
	this->Model::setState(std::make_shared<TrafficLightMode>(s));
	return this->getState();
}

}

