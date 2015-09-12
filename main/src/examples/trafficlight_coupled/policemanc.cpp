/*
 * policemanc.cpp
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#include "examples/trafficlight_coupled/policemanc.h"

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

void PolicemanMode::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Policeman Mode '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
	archive(cereal::virtual_base_class<State>( this ));
}

void PolicemanMode::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<State>( this ));
	LOG_INFO("SERIALIZATION: Loaded Policeman Mode '", m_state, "' with timeNext = ", m_timeNext, " and timeLast = ", m_timeLast);
}

void PolicemanMode::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<PolicemanMode>& construct)
{
	construct("");
	construct->serialize(archive);
}

Policeman::Policeman(std::string name, std::size_t priority)
	: AtomicModel_impl(name, priority)
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

void Policeman::output(std::vector<n_network::t_msgptr>& msgs) const
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
		return;

	this->getPort("OUT")->createMessages(message, msgs);
}

t_timestamp Policeman::lookAhead() const
{
	// Nothing can interrupt the police
	return t_timestamp::infinity();
}


t_stateptr Policeman::setState(std::string s)
{
	this->Model::setState(n_tools::createObject<PolicemanMode>(s));
	return this->getState();
}

void Policeman::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Policeman '", getName(), "' with timeNext = ", m_timeNext);
	archive(cereal::virtual_base_class<AtomicModel_impl>( this ));
}

void Policeman::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<AtomicModel_impl>( this ));
	LOG_INFO("SERIALIZATION: Loaded Policeman '", getName(), "' with timeNext = ", m_timeNext);
}

void Policeman::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Policeman>& construct)
{
	construct("");
	construct->serialize(archive);
}

}

