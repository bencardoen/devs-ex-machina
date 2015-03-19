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
	: m_name(name), m_hostname(hostname), m_inputPort(inputPort)
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
std::function<void(const n_network::t_msgptr&)> Port::getZFunc(std::shared_ptr<Port> port) const
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
bool Port::setZFunc(std::shared_ptr<Port> port, t_zfunc function)
{
	if (m_outs.find(port) == m_outs.end())
		return false;
	m_outs.insert(std::pair<std::shared_ptr<Port>, t_zfunc > (port, function));
	return true;
}

/*
 * Sets an input port to this port
 *
 * @param port new input port
 *
 * @return whether or not the port wasn't already added
 */
bool Port::setInPort(std::shared_ptr<Port> port)
{
	if (std::find(m_ins.begin(), m_ins.end(), port) == m_ins.end())
		return false;
	m_ins.push_back(port);
	return true;
}
}
