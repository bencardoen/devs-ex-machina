/*
 * model.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "model.h"
#include "controller.h"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/deque.hpp"

namespace n_model {

Model::Model(std::string name)
	: m_name(name), m_state(nullptr), m_control(nullptr), m_keepOldStates(true)
{
}

std::string Model::getName() const
{
	//return m_name;  // Do not enable, threadrace on COW implementation of std string
	return n_tools::copyString(m_name);
}

t_portptr Model::getPort(std::string name) const
{
	auto ptr1 = m_iPorts.find(name);
	auto ptr2 = m_oPorts.find(name);
	if (ptr1 == m_iPorts.end()) {
		if (ptr2 == m_oPorts.end())
			return nullptr;
		else
			return ptr2->second;
	} else
		return ptr1->second;
	return nullptr;
}

t_stateptr Model::getState() const
{
	return m_state;
}

void Model::setState(const t_stateptr& newState)
{
	if (newState == nullptr)
		return;
	m_state = newState;
	if (m_keepOldStates)
		m_oldStates.push_back(m_state);
	else {
		if (m_oldStates.size() != 0)
			m_oldStates.at(0) = m_state;
		else
			m_oldStates.push_back(m_state);
	}
}

void Model::setParent(const std::shared_ptr<Model>& parent)
{
	m_parent = parent;
}

const std::weak_ptr<Model>& Model::getParent() const
{
	return m_parent;
}

void Model::resetParents()
{
	m_parent.reset();
}

t_portptr Model::addPort(std::string name, bool isIn)
{
	assert(allowDS() && "Model::addPort: Dynamic structured DEVS is not allowed in this phase.");
	// Find new name for port if name was empty
	std::string n = name;
	if (n == "") {
		n = "port";
		int number = (int) m_iPorts.size() + (int) m_oPorts.size();
		std::stringstream ss;
		ss << n << number;
		n = ss.str();
	}

	t_portptr port(n_tools::createObject<Port>(name, this->m_name, isIn));

	if (isIn)
		m_iPorts.insert(std::pair<std::string, t_portptr>(name, port));
	else
		m_oPorts.insert(std::pair<std::string, t_portptr>(name, port));

	if (m_control) {
		m_control->dsUndoDirectConnect();
	}

	return port;
}

void Model::removePort(t_portptr& port)
{
	//remove the port itself
	assert(allowDS() && "Model::removePort: Dynamic structured DEVS is not allowed in this phase.");
	if (port->isInPort())
		m_iPorts.erase(port->getName());
	else
		m_oPorts.erase(port->getName());

	if (m_control)
		m_control->dsRemovePort(port);
}

t_portptr Model::addInPort(std::string name)
{
	return this->addPort(name, true);
}

t_portptr Model::addOutPort(std::string name)
{
	return this->addPort(name, false);
}

const std::map<std::string, t_portptr>& Model::getIPorts() const
{
	return m_iPorts;
}

const std::map<std::string, t_portptr>& Model::getOPorts() const
{
	return m_oPorts;
}

std::map<std::string, t_portptr>& Model::getIPorts()
{
	return m_iPorts;
}

std::map<std::string, t_portptr>& Model::getOPorts()
{
	return m_oPorts;
}

bool Model::modelTransition(DSSharedState*)
{
	return false;
}

void Model::setController(n_control::Controller* newControl)
{
	m_control = newControl;
}

bool Model::allowDS() const
{
	if (m_control)
		return m_control->isInDSPhase();
	return true;
}

t_timestamp Model::getTimeNext() const
{
	return m_timeNext;
}

bool Model::getKeepOldStates() const
{
	return m_keepOldStates;
}

void Model::setKeepOldStates(bool b)
{
	m_keepOldStates = b;
}

void Model::serialize(n_serialization::t_oarchive& archive)
{
	archive(m_name, m_timeLast, m_timeNext, m_state, m_oldStates,
			m_iPorts, m_oPorts, m_sendMessages, m_receivedMessages,
			m_keepOldStates, m_parent);
}

void Model::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_name, m_timeLast, m_timeNext, m_state, m_oldStates,
			m_iPorts, m_oPorts, m_sendMessages, m_receivedMessages,
			m_keepOldStates, m_parent);
}

void Model::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Model>& construct)
{
	std::string name;
	archive(name);
	construct(name);
}

}
