/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
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
	return t_timestamp::epsilon();
}

}

