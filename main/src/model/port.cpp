/*
 * port.cpp
 *
 *  Created on: 9-mrt.-2015
 *      Author: Pieter Tim Matthijs Stijn
 */

#include "model/port.h"
#include "cereal/types/string.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/polymorphic.hpp"
#include "tools/stringtools.h"
#include "tools/objectfactory.h"
#include "model.h"
#include "atomicmodel.h"
#include <algorithm>

namespace n_model {

Port::Port(const std::string& name, Model* host, std::size_t portid, bool inputPort)
	: m_name(name), m_hostname(host->getName()),
	  m_portid(portid), m_inputPort(inputPort),
	  m_usingDirectConnect(false),
	  m_hostmodel(host)
{
}

std::string Port::getName() const
{
	return n_tools::copyString(m_name);
}

std::string Port::getHostName() const
{
	return n_tools::copyString(m_hostname);
}

bool Port::isInPort() const
{
	return m_inputPort;
}

void n_model::Port::removeOutPort(const t_portptr_raw port)
{
	std::vector<t_outconnect>::iterator it = m_outs.begin();
	while(it != m_outs.end()){
		if(it->first == port)
			break;
		++it;
	}

	if(it != m_outs.end()){
		//there is a connection to this port
		m_outs.erase(it);
	}
}

void Port::removeInPort(const t_portptr_raw port)
{
	LOG_DEBUG("current amount of ports: ", m_ins.size());
	auto it = std::find(m_ins.begin(), m_ins.end(), port);
	if(it != m_ins.end())
		m_ins.erase(it);
	else
		LOG_ERROR("tried to remove port that we don't have!");
	LOG_DEBUG("new amount of ports: ", m_ins.size());
}

bool Port::setZFunc(const t_portptr_raw port, t_zfunc function)
{
	std::vector<t_outconnect>::iterator it = m_outs.begin();
	while(it != m_outs.end()){
		if(it->first == port)
			return false;
		++it;
	}
	m_outs.push_back(t_outconnect(port, function));
	return true;
}

bool Port::setInPort(const t_portptr_raw port)
{
	LOG_DEBUG("current amount of ports: ", m_ins.size());
	if (std::find(m_ins.begin(), m_ins.end(), port) != m_ins.end())
		return false;
	m_ins.push_back(port);
	LOG_DEBUG("new amount of ports: ", m_ins.size());
	return true;
}

void Port::setZFuncCoupled(const t_portptr_raw port, t_zfunc function)
{
	m_coupled_outs.push_back(t_outconnect(port, function));
//#ifdef USE_STAT
	std::string statname = getHostName() + "/" + getName() + "->" + port->getHostName() + "/" + port->getName();
	m_sendstat.emplace(port,n_tools::t_uintstat(statname, "messages"));
//#endif
}

void Port::setUsingDirectConnect(bool dc)
{
	m_usingDirectConnect = dc;
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

void Port::setInPortCoupled(const t_portptr_raw port)
{
	m_coupled_ins.push_back(port);
}

void Port::clearSentMessages()
{
	m_sentMessages.clear();
}

void Port::clearReceivedMessages()
{
	m_receivedMessages.clear();
}

void Port::addMessage(const n_network::t_msgptr& message)
{
#ifndef  NO_TRACER
	LOG_DEBUG("Added message ", message->getPayload(), ", we RECEIVED this message.");
	m_receivedMessages.push_back(message);
#endif
}

const std::vector<n_network::t_msgptr>& Port::getSentMessages() const
{
	return m_sentMessages;
}

const std::vector<n_network::t_msgptr>& Port::getReceivedMessages() const
{
	return m_receivedMessages;
}

void Port::addInfluencees(std::set<std::string>& influences) const
{
	for (auto& port : this->m_coupled_ins)
		influences.insert(port->getHostName());
}

const uuid& Port::getModelUUID() const
{
#ifdef SAFETY_CHECKS
	AtomicModel_impl* impl = dynamic_cast<AtomicModel_impl*>(m_hostmodel);
	assert(impl != nullptr && "Requested getModelUUID from a non-atomic model.");
	return impl->getUUID();
#else /* no SAFETY_CHECKS */
        return reinterpret_cast<AtomicModel_impl*>(m_hostmodel)->getUUID();
#endif /* SAFETY_CHECKS */
}

t_timestamp Port::imminentTime()const 
{
#ifdef SAFETY_CHECKS
	AtomicModel_impl* impl = dynamic_cast<AtomicModel_impl*>(m_hostmodel);
	assert(impl != nullptr && "Requested getModelUUID from a non-atomic model.");
	return impl->getTimeNext();
#else /* no SAFETY_CHECKS */
        return reinterpret_cast<AtomicModel_impl*>(m_hostmodel)->getTimeNext();
#endif /* SAFETY_CHECKS */
}

void Port::clearConnections()
{
	for(t_portptr_raw& ptr: m_ins){
		ptr->removeOutPort(this);
	}
	m_ins.clear();
	for(t_outconnect& ptr: m_outs){
		ptr.first->removeInPort(this);
	}
	m_outs.clear();
}

void Port::serialize(n_serialization::t_oarchive& archive)
{
	archive(m_name, m_hostname, m_inputPort, m_portid,
//			m_ins, m_outs,
//			m_coupled_outs, m_coupled_ins,
//			m_sentMessages, m_receivedMessages,
			m_usingDirectConnect);
}

void Port::serialize(n_serialization::t_iarchive& archive)
{
	archive(m_name, m_hostname, m_inputPort, m_portid,
//			m_ins, m_outs,
//			m_coupled_outs, m_coupled_ins,
//			m_sentMessages, m_receivedMessages,
			m_usingDirectConnect);
}

void Port::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<Port>& construct )
{
	/*std::string name;
	std::string hostname;
	bool inputPort;

	archive(name, hostname, inputPort);
	construct(name, hostname, inputPort);*/

	construct("", nullptr, 0, false);
	construct->serialize(archive);
}

}
