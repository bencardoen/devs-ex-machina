/*
 * policemands.cpp
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "examples/trafficlight_ds/policemands.h"

namespace n_examples_ds {


Policeman::Policeman(std::string name, std::size_t priority)
	: AtomicModel<PolicemanMode>(name, PolicemanMode("idle_at_1"), priority)
{
//	this->setState(n_tools::createObject<PolicemanMode>("idle_at_1"));
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

	if (mode.m_value == "idle_at_1")
		mode.m_value = "working_at_1";
	else if (mode.m_value == "working_at_1")
		mode.m_value = "moving_from_1_to_2";
	else if (mode.m_value == "moving_from_1_to_2")
		mode.m_value = "idle_at_2";
	else if (mode.m_value == "idle_at_2")
		mode.m_value = "working_at_2";
	else if (mode.m_value == "working_at_2")
		mode.m_value = "moving_from_2_to_1";
	else if (mode.m_value == "moving_from_2_to_1")
		mode.m_value = "idle_at_1";
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp Policeman::timeAdvance() const
{
	const PolicemanMode& mode = state();
	std::string substr = mode.m_value.substr(0, 4);
	LOG_DEBUG("Policeman::timeAdvance substr = ", substr);
	if (substr == "idle")
		return t_timestamp(50);
	else if (substr == "work")
		return t_timestamp(100);
	else if (substr == "movi")
		return t_timestamp(150);
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

	std::string substr = mode.m_value.substr(0, 4);
	if (substr == "idle")
		message = "toManual";
	else if (substr == "work")
		message = "toAutonomous";
	else // nothing happens
		return;

	this->getPort("OUT")->createMessages(message, msgs);
}

bool Policeman::modelTransition(n_model::DSSharedState* shared)
{
	LOG_DEBUG("Policeman::parent = ", m_parent);
	PolicemanMode& mode = state();
	std::string val = "";
	if (mode.m_value == "moving_from_1_to_2"){
		val = "2";
	}
	else if (mode.m_value == "moving_from_2_to_1"){
		val = "1";
	}
	LOG_DEBUG("Policeman::modelTransition with destination = ", val);
	if(!val.empty()){
		if(!shared->values.empty())
			shared->values.clear();
		shared->values.emplace("destination", val);
		return true;
	}
	return false;
}

}

