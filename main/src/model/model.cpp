/*
 * model.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "model.h"

namespace n_model {

/*
 * Constructor for Model
 *
 * @param name of model
 */
Model::Model(std::string name)
	: m_name(name), m_receivedExt(false)
{

}

/*
 * Returns the name of the model
 *
 * @return name of model
 */
std::string Model::getName() const
{
	return m_name;
}

/*
 * Returns the port corresponding with the given name
 *
 * @param name The name of the port
 * @return a shared pointer to the port
 */
t_portptr Model::getPort(std::string name)
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
}

/*
 * Returns the current state of the model
 *
 * @return current state of model
 */
t_stateptr Model::getState() const
{
	return m_state;
}

/*
 * Set the current state of the model to a new state
 *
 * @param newState the new state the model should switch to
 */
void Model::setState(t_stateptr newState)
{
	if (newState == nullptr)
		return;
	m_state = newState;
}

/*
 * Utility function to create a new port and add it
 *
 * @param name The name of the port
 * @param isIn Whether or not this port is an input port
 */
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

/*
 * Add an input port to the model
 *
 * @param name The name of the port
 */
t_portptr Model::addInPort(std::string name)
{
	return this->addPort(name, true);
}

/*
 * Add an output port to the model
 *
 * @param name The name of the port
 */
t_portptr Model::addOutPort(std::string name)
{
	return this->addPort(name, false);
}

}

