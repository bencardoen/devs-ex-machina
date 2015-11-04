/*
 * policemanc.cpp
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#include "examples/trafficlight_coupled/policemanc.h"

namespace n_examples_coupled {


Policeman::Policeman(std::string name, std::size_t priority)
	: AtomicModel(name, PolicemanMode("idle"), priority)
{
//	this->setState(n_tools::createObject<PolicemanMode>("idle"));
	// Initialize elapsed attribute if required
	m_elapsed = 0;
	this->addOutPort("OUT");
}

void Policeman::extTransition(const std::vector<n_network::t_msgptr> &)
{
	// No external transitions available yet...
}

void Policeman::intTransition()
{
	PolicemanMode& mode = state();
	if (mode.m_value == "idle")
		mode.m_value = "working";
	else if (mode.m_value == "working")
		mode.m_value = "idle";
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp Policeman::timeAdvance() const
{
	const PolicemanMode& mode = state();
	if (mode.m_value == "idle")
		return t_timestamp(200);
	else if (mode.m_value == "working")
		return t_timestamp(100);
	else
		assert(false); // You shouldn't come here...
	return t_timestamp();
}

void Policeman::output(std::vector<n_network::t_msgptr>& msgs) const
{
	// BEFORE USING THIS FUNCTION
	// READ EXAMPLE @ PYTHONPDEVS trafficlight_classic/model.py line 117 ( def outputFnc(self):)
	const PolicemanMode& mode = state();
	std::string message = "";

	if (mode.m_value == "idle")
		message = "toManual";
	else if (mode.m_value == "working")
		message = "toAutonomous";
	else
		// nothing happens
		return;

	this->getPort("OUT")->createMessages(message, msgs);
}

t_timestamp Policeman::lookAhead() const
{
	// Nothing can interrupt the police
	return t_timestamp::infinity();
}

}

