/*
 * port.cpp
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#include "port.h"

namespace n_model {
/*
 * Constructor of Port
 *
 * @param name the local name of the port
 * @param host pointer to the host of the port
 * @param inputPort whether or not this is an input port
 */
Port::Port(std::string name, std::string hostname, bool inputPort)
	: m_name(name), m_hostname(hostname), m_inputPort(inputPort), m_usingDirectConnect(false)
{
}

/*
 * Returns the name of the port
 *
 * @return local name of the port
 */
std::string Port::getName() const
{
	return m_name;
}

/*
 * Returns the complete name of the port
 *
 * @return fully qualified name of the port
 */
std::string Port::getFullName() const
{
	return m_hostname + "." + m_name;
}

/*
 * Returns the hostname of the port
 *
 * @return Hostname of the port
 */
std::string Port::getHostName() const
{
	return m_hostname;
}

/*
 * Returns whether or not this is an input port
 *
 * @return whether or not this is an input port
 */
bool Port::isInPort() const
{
	return m_inputPort;
}

/*
 * Returns the function matching with the given port.
 *
 * @return the matching function
 */
t_zfunc Port::getZFunc(const std::shared_ptr<Port>& port) const
{
	return m_outs.at(port);
}

/*
 * Sets an output port with a matching function to this port
 *
 * @param port new output port
 * @param function function matching with the new output port
 *
 * @return whether or not the port wasn't already added
 */
bool Port::setZFunc(const std::shared_ptr<Port>& port, t_zfunc function)
{
	if (m_outs.find(port) == m_outs.end())
		return false;
	m_outs.insert(std::pair<std::shared_ptr<Port>, t_zfunc>(port, function));
	return true;
}

/*
 * Sets an input port to this port
 *
 * @param port new input port
 *
 * @return whether or not the port was already added
 */
bool Port::setInPort(const std::shared_ptr<Port>& port)
{
	if (std::find(m_ins.begin(), m_ins.end(), port) == m_ins.end())
		return false;
	m_ins.push_back(port);
	return true;
}

/*
 * Sets a coupled input port to this port (for direct connect usage)
 *
 * @param port New input port
 *
 * @return whether or not the port wasn already added
 */
bool Port::setZFuncCoupled(const std::shared_ptr<Port>& port, t_zfunc function)
{
	if (m_coupled_outs.find(port) == m_coupled_outs.end())
		return false;
	m_coupled_outs.insert(std::pair<std::shared_ptr<Port>, t_zfunc>(port, function));
	return true;
}

/*
 * Sets the if whether the port is currently using direct connect or not
 *
 * @param dc True or false, depending of the port is currently using direct connect
 */
void Port::setUsingDirectConnect(bool dc)
{
	m_usingDirectConnect = dc;
}

/*
 * Function that creates messages with a give payload.
 * These messages are addressed to all out-ports that are currently connected
 * Note that these out-ports can differ if you are using direct connect!
 * Zfunctions that apply will be called upon the messages
 *
 * @param message The payload of the message that is to be sent
 */
std::vector<n_network::t_msgptr> Port::createMessages(std::string message)
{
	std::vector<n_network::t_msgptr> returnarray;

	// We want to iterate over the correct ports (whether we use direct connect or not)
	std::map<std::shared_ptr<Port>, t_zfunc>& ports = m_outs;
	if (m_usingDirectConnect)
		ports = m_coupled_outs;

	for (auto& pair : ports) {

		t_zfunc& zFunction = pair.second;
		std::string model_destination = pair.first->getHostName();;
		std::string sourcePort = this->getFullName();
		std::string destPort = pair.first->getFullName();
		n_network::t_timestamp dummytimestamp(n_network::t_timestamp::infinity());

		// We now know everything, we create the message, apply the zFunction and push it on the vector
		n_network::t_msgptr messagetobesend = std::make_shared<n_network::Message>(model_destination,
		        dummytimestamp, destPort, sourcePort, message);
		messagetobesend = (*zFunction)(messagetobesend);
		returnarray.push_back(messagetobesend);
	}

	return returnarray;
}

}
