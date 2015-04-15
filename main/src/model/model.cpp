/*
 * model.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "model.h"

namespace n_model {

Model::Model(std::string name)
	: m_name(name), m_state(nullptr)
{

}

std::string Model::getName() const
{
	return m_name;
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
	m_oldStates.push_back(m_state);
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
	// Find new name for port if name was empty
	std::string n = name;
	if (n == "") {
		n = "port";
		int number = (int) m_iPorts.size() + (int) m_oPorts.size();
		std::stringstream ss;
		ss << n << number;
		n = ss.str();
	}

	t_portptr port(new Port(name, this->m_name, isIn));

	if (isIn)
		m_iPorts.insert(std::pair<std::string, t_portptr>(name, port));
	else
		m_oPorts.insert(std::pair<std::string, t_portptr>(name, port));

	return port;
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

}

