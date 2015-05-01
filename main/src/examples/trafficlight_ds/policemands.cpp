/*
 * policemands.cpp
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "policemands.h"

namespace n_examples_ds {

PolicemanMode::PolicemanMode(std::string state)
	: State(state)
{

}

std::string PolicemanMode::toXML()
{
	return "<policeman activity =\"" + this->toString() + "\"/>";
}

std::string PolicemanMode::toJSON()
{
	return "{ \"activity\": \"" + this->toString() + "\" }";
}

std::string PolicemanMode::toCell()
{
	return "";
}

Policeman::Policeman(std::string name, std::size_t priority)
	: AtomicModel(name, priority)
{
	this->setState(n_tools::createObject<PolicemanMode>("idle_at_1"));
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
	t_stateptr state = this->getState();

	if (*state == "idle_at_1")
	    this->setState("working_at_1");
	else if (*state == "working_at_1")
	    this->setState("moving_from_1_to_2");
	else if (*state == "moving_from_1_to_2")
	    this->setState("idle_at_2");
	else if (*state == "idle_at_2")
	    this->setState("working_at_2");
	else if (*state == "working_at_2")
	    this->setState("moving_from_2_to_1");
	else if (*state == "moving_from_2_to_1")
	    this->setState("idle_at_1");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp Policeman::timeAdvance() const
{
	t_stateptr state = this->getState();
	std::string substr = state->m_state.substr(0, 4);
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

std::vector<n_network::t_msgptr> Policeman::output() const
{
	// BEFORE USING THIS FUNCTION
	// READ EXAMPLE @ PYTHONPDEVS trafficlight_classic/model.py line 117 ( def outputFnc(self):)
	t_stateptr state = this->getState();
	std::string message = "";

	std::string substr = state->m_state.substr(0, 4);
	if (substr == "idle")
		message = "toManual";
	else if (substr == "work")
		message = "toAutonomous";
	else // nothing happens
		return std::vector<n_network::t_msgptr>();

	return this->getPort("OUT")->createMessages(message);
}

t_stateptr Policeman::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<PolicemanMode>(s));
	return this->getState();
}

bool Policeman::modelTransition(n_model::DSSharedState* shared)
{
	LOG_DEBUG("Policeman::parent = ", m_parent.expired());
	t_stateptr state = this->getState();
	std::string val = "";
	if (*state == "moving_from_1_to_2"){
		val = "2";
	}
	else if (*state == "moving_from_2_to_1"){
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

