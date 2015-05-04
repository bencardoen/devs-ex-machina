/*
 * policemanc.cpp
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#include "policemanc.h"

namespace n_examples_coupled {

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
	this->setState(n_tools::createObject<PolicemanMode>("idle"));
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
	if (*state == "idle")
		this->setState("working");
	else if (*state == "working")
		this->setState("idle");
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp Policeman::timeAdvance() const
{
	t_stateptr state = this->getState();
	if (*state == "idle")
		return t_timestamp(200);
	else if (*state == "working")
		return t_timestamp(100);
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

	if (*state == "idle")
		message = "toManual";
	else if (*state == "working")
		message = "toAutonomous";
	else
		// nothing happens
		return std::vector<n_network::t_msgptr>();

	return this->getPort("OUT")->createMessages(message);
}

t_stateptr Policeman::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<PolicemanMode>(s));
	return this->getState();
}

}

