/*
 * port.cpp
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter Tim Matthijs Stijn
 */

#include "port.h"
#include "cereal/types/string.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "stringtools.h"
#include "objectfactory.h"

namespace n_model {

Port::Port(std::string name, std::string hostname, bool inputPort)
	: m_name(name), m_hostname(hostname), m_inputPort(inputPort), m_usingDirectConnect(false)
{
}

std::string Port::getName() const
{
	return n_tools::copyString(m_name);
}

std::string Port::getFullName() const
{
	//std::string tmp = m_hostname + "." + m_name;
	// This is ugly, but required. G++ is too smart and sees through
	// what I'm trying to do. Append forces a new copy.
	// note that volatile does not work here.
	std::string tmp = this->getHostName();
	tmp.append(".");
	tmp.append(this->getName());
	return n_tools::copyString(tmp);
}

std::string Port::getHostName() const
{
	return n_tools::copyString(m_hostname);
}

bool Port::isInPort() const
{
	return m_inputPort;
}

t_zfunc Port::getZFunc(const std::shared_ptr<Port>& port) const
{
	return m_outs.at(port);
}

void n_model::Port::removeOutPort(const t_portptr& port)
{
	std::map<t_portptr, t_zfunc>::iterator it = m_outs.find(port);
	if(it != m_outs.end()){
		//there is a connection to this port
		m_outs.erase(it);
	}
}

void Port::removeInPort(const t_portptr& port)
{
	m_ins.erase(std::find(m_ins.begin(), m_ins.end(), port));
}

bool Port::setZFunc(const std::shared_ptr<Port>& port, t_zfunc function)
{
	if (m_outs.find(port) != m_outs.end()) {
		return false;
	}
	m_outs.insert(std::pair<std::shared_ptr<Port>, t_zfunc>(port, function));
	return true;
}

bool Port::setInPort(const std::shared_ptr<Port>& port)
{
	if (std::find(m_ins.begin(), m_ins.end(), port) != m_ins.end())
		return false;
	m_ins.push_back(port);
	return true;
}

void Port::setZFuncCoupled(const std::shared_ptr<Port>& port, t_zfunc function)
{
	std::map<std::shared_ptr<Port>, std::vector<t_zfunc>>::iterator it = m_coupled_outs.find(port);
	if (it == m_coupled_outs.end()) {
		m_coupled_outs.emplace(port, std::vector<t_zfunc>( { function }));
		return;
	}
	it->second.push_back(function);
}

void Port::setUsingDirectConnect(bool dc)
{
	m_usingDirectConnect = dc;
}

const std::vector<std::shared_ptr<Port> >& Port::getIns() const
{
	return m_ins;
}

std::vector<std::shared_ptr<Port> >& Port::getIns()
{
	return m_ins;
}

void Port::resetDirectConnect()
{
	m_usingDirectConnect = false;
	m_coupled_outs.clear();
	m_coupled_ins.clear();
}

bool Port::isUsingDirectConnect() const
{
	return m_usingDirectConnect;
}

void Port::setInPortCoupled(const t_portptr& port)
{
	m_coupled_ins.push_back(port);
}

const std::map<std::shared_ptr<Port>, t_zfunc>& Port::getOuts() const
{
	return m_outs;
}

std::map<std::shared_ptr<Port>, t_zfunc>& Port::getOuts()
{
	return m_outs;
}

const std::vector<t_portptr>& Port::getCoupledIns() const
{
	return m_coupled_ins;
}

void Port::clear()
{
	m_ins.clear();
	m_outs.clear();
	m_coupled_ins.clear();
	m_coupled_outs.clear();
}

const std::map<t_portptr, std::vector<t_zfunc> >& Port::getCoupledOuts() const
{
	return m_coupled_outs;
}

void Port::clearSentMessages()
{
	m_sentMessages.clear();
}

void Port::clearReceivedMessages()
{
	m_receivedMessages.clear();
}

void Port::addMessage(const n_network::t_msgptr& message, bool received)
{
	LOG_DEBUG("Added message ", message->getPayload(), ", we ", received? "RECEIVED":"SENT", " this message.");
	if (received)
		m_receivedMessages.push_back(message);
	else
		m_sentMessages.push_back(message);
}

const std::vector<n_network::t_msgptr>& Port::getSentMessages() const
{
	return m_sentMessages;
}

const std::vector<n_network::t_msgptr>& Port::getReceivedMessages() const
{
	return m_receivedMessages;
}

void Port::serialize(n_serialization::t_oarchive& archive)
{
	archive(m_name, m_hostname, m_inputPort, m_ins, m_outs,
			m_coupled_outs, m_coupled_ins,
			m_sentMessages, m_receivedMessages,
			m_usingDirectConnect);
}

void Port::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_name, m_hostname, m_inputPort, m_ins, m_outs,
			m_coupled_outs, m_coupled_ins,
			m_sentMessages, m_receivedMessages,
			m_usingDirectConnect);
}

void Port::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Port>& construct )
{
	std::string name;
	std::string hostname;
	bool inputPort;

	archive(name, hostname, inputPort);
	construct(name, hostname, inputPort);
}

template<>
n_network::t_msgptr createMsg<std::string>(const std::string& dest, const std::string& destP, const std::string& sourceP,
        const std::string& msg, t_zfunc& func)
{
	n_network::t_msgptr messagetobesend = n_tools::createObject<n_network::Message>(dest,
	        n_network::t_timestamp::infinity(), destP, sourceP, msg);
	messagetobesend = (*func)(messagetobesend);
	return messagetobesend;
}

}
