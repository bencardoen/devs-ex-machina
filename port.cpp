/*
 * port.cpp
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter
 */

#include "port.h"

namespace model {
	/*
	 * Constructor of Port
	 *
	 * @param name the local name of the port
	 * @param host pointer to the host of the port
	 * @param inputPort whether or not this is an input port
	 */
	Port::Port(std::string name, std::shared_ptr<Model> host, bool inputPort) {
		m_name = name;
		m_host = host;
		m_inputPort = inputPort;
	}

	/*
	 * Returns the name of the port
	 *
	 * @return local name of the port
	 */
	std::string Port::getName() {
		return m_name;
	}

	/*
	 * Returns the complete name of the port
	 *
	 * @return fully qualified name of the port
	 */
	std::string Port::getFullName() {
		return m_host->getName() + "." + m_name;
	}

	/*
	 * Returns whether or not this is an input port
	 *
	 * @return whether or not this is an input port
	 */
	bool Port::isInPort() {
		return m_inputPort;
	}

	/*
	 * Returns the function matching with the given port.
	 *
	 * @return the matching function
	 */
	std::function<void> Port::getZFunc(std::shared_ptr<Port> port) {
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
	bool Port::setZFunc(std::shared_ptr<Port> port, std::function<void> function) {
		std::pair<std::map<char,int>::iterator,bool> ret;
		ret = m_outs.insert(std::pair<std::shared_ptr<Port>, std::function<void> >(port, function));
		if (ret.second == false) { // port was already used, replace function
			ret.first->second = function;
			return false;
		}
		return true;
	}

	/*
	 * Sets an input port to this port
	 *
	 * @param port new input port
	 *
	 * @return whether or not the port wasn't already added
	 */
	bool Port::setInPort(std::shared_ptr<Port> port) {
		if (m_ins.contains(port)) return false;

		m_ins.append(port);
		return true;
	}
}
