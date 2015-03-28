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
Model::Model(std::string name, int corenumber)
	: m_name(name), m_coreNumber(corenumber), m_state(nullptr)
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
 * Set the current state of the model to a new state and pushes this new state on
 * the list of all oldStates.
 *
 * @param newState the new state the model should switch to (as a State object)
 */
void Model::setState(const t_stateptr& newState)
{
	if (newState == nullptr)
		return;
	m_state = newState;
	m_oldStates.push_back(m_state);
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

/*
 * Get the current core number
 *
 * @return current core number
 */
int Model::getCoreNumber() const
{
	return m_coreNumber;
}

/*
 * Set the current core number
 *
 * @param core current core number
 */
void Model::setCoreNumber(int core)
{
	m_coreNumber = core;
}

/*
 * Return all current input ports
 *
 * @return current input ports
 */
const std::map<std::string, t_portptr>& Model::getIPorts() const
{
	return m_iPorts;
}

/*
 * Return all current output ports
 *
 * @return current output ports
 */
const std::map<std::string, t_portptr>& Model::getOPorts() const
{
	return m_oPorts;
}

/*
 * Return all current send messages
 *
 * @return current send messages
 */
const std::deque<n_network::t_msgptr>& Model::getSendMessages() const
{
	return m_sendMessages;
}

/*
 * Return all current received messages
 *
 * @return current received messages
 */
const std::deque<n_network::t_msgptr>& Model::getReceivedMessages() const
{
	return m_receivedMessages;
}

}

