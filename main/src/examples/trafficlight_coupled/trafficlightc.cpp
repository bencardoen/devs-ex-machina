/*
 * trafficlightc.cpp
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#include "examples/trafficlight_coupled/trafficlightc.h"

namespace n_examples_coupled {

TrafficLight::TrafficLight(std::string name, std::size_t priority)
	: AtomicModel(name, TrafficLightMode("red"), priority)
{
//	this->setState(n_tools::createObject<TrafficLightMode>("red"));
	// Initialize elapsed attribute if required
	m_elapsed = 2;

	this->addInPort("INTERRUPT");
	this->addOutPort("OBSERVED");
}

void TrafficLight::extTransition(const std::vector<n_network::t_msgptr> & inputs)
{
	auto input = inputs.at(0);

	TrafficLightMode& mode = state();

	if (input->getPayload() == "toManual") {
		if (mode.m_value == "manual")
			mode.m_value = "manual"; // Keep light on manual
		else if (mode.m_value == "red" || mode.m_value == "green" || mode.m_value == "yellow")
			mode.m_value = "manual"; // Set light to manual
		else
			assert(false);
	} else if (input->getPayload() == "toAutonomous") {
		if (mode.m_value == "manual")
			mode.m_value = "red"; // Restart with a red light
		else if (mode.m_value == "red" || mode.m_value == "green" || mode.m_value == "yellow")
			{} // Keep the same light
		else
			assert(false);
	}
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
	else if (mode.m_value == "manual")
		return t_timestamp::infinity();
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

void TrafficLight::output(std::vector<n_network::t_msgptr>& msgs) const
{
	// BEFORE USING THIS FUNCTION
	// READ EXAMPLE @ PYTHONPDEVS trafficlight_classic/model.py line 117 ( def outputFnc(self):)
	const TrafficLightMode& mode = state();
	std::string message = "";

	if (mode.m_value == "red")
		message = "grey";
	else if (mode.m_value == "green")
		message = "yellow";
	else if (mode.m_value == "yellow")
		message = "grey";
	else  // nothing happens
		return;

	this->getPort("OBSERVED")->createMessages(message, msgs);
}

t_timestamp TrafficLight::lookAhead() const
{
	// Lookahead of this model is 0, because the policeman can interrupt the traffic light at any
	// given time
	return t_timestamp::epsilon();
}

void TrafficLight::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Traffic Light '", getName());
	archive(cereal::virtual_base_class<AtomicModel_impl>( this ));
}

void TrafficLight::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<AtomicModel_impl>( this ));
	LOG_INFO("SERIALIZATION: Loaded Traffic Light '", getName());
}


void TrafficLight::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficLight>& construct)
{
	LOG_DEBUG("TrafficLight: Load and Construct");
	construct("");
	construct->serialize(archive);
}

}

