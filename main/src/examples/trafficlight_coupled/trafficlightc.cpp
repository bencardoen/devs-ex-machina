/*
 * trafficlightc.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "examples/trafficlight_coupled/trafficlightc.h"

namespace n_examples_coupled {

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

void TrafficLightMode::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Traffic Light Mode '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
	archive(cereal::virtual_base_class<State>( this ));
}

void TrafficLightMode::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<State>( this ));
	LOG_INFO("SERIALIZATION: Loaded Traffic Light Mode '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
}

void TrafficLightMode::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficLightMode>& construct)
{
	construct("");
	construct->serialize(archive);
}

TrafficLight::TrafficLight(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	this->setState(n_tools::createObject<TrafficLightMode>("red"));
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

void TrafficLight::output(std::vector<n_network::t_msgptr>& msgs) const
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
		return;

	this->getPort("OBSERVED")->createMessages(message, msgs);
}

t_timestamp TrafficLight::lookAhead() const
{
	// Lookahead of this model is 0, because the policeman can interrupt the traffic light at any
	// given time
	return t_timestamp();
}


t_stateptr TrafficLight::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<TrafficLightMode>(s));
	return this->getState();
}

void TrafficLight::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Traffic Light '", getName(), "' with timeNext = ", m_timeNext);
	archive(cereal::virtual_base_class<AtomicModel>( this ));
}

void TrafficLight::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<AtomicModel>( this ));
	LOG_INFO("SERIALIZATION: Loaded Traffic Light '", getName(), "' with timeNext = ", m_timeNext);
}


void TrafficLight::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficLight>& construct)
{
	LOG_DEBUG("TrafficLight: Load and Construct");
	construct("");
	construct->serialize(archive);
}

}

