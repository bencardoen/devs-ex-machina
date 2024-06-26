/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn, Matthijs Van Os
 */

#include "model/port.h"
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
#ifdef SAFETY_CHECKS
	std::vector<t_outconnect>::iterator it = m_outs.begin();
	while(it != m_outs.end()){
		if(it->first == port)
			return false;
		++it;
	}
#endif /* SAFETY_CHECKS */
	m_outs.push_back(t_outconnect(port, function));
	return true;
}

bool Port::setInPort(const t_portptr_raw port)
{
	LOG_DEBUG("current amount of ports: ", m_ins.size());
#ifdef SAFETY_CHECKS
	if (std::find(m_ins.begin(), m_ins.end(), port) != m_ins.end())
		return false;
#endif /* SAFETY_CHECKS */
	m_ins.push_back(port);
	LOG_DEBUG("new amount of ports: ", m_ins.size());
	return true;
}

void Port::setZFuncCoupled(const t_portptr_raw port, t_zfunc function)
{
	m_coupled_outs.push_back(t_outconnect(port, function));
#ifdef USE_STAT
	std::string statname = getHostName() + "/" + getName() + "->" + port->getHostName() + "/" + port->getName();
	m_sendstat.emplace(port,n_tools::t_uintstat(statname, "messages"));
#endif
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

Port::~Port(){
        // Don't delete here, creator pool is no longer valid (neither are the ptrs)
        // This means that for Message<string> we get leaks at the moment for local (trace) messages only.
        // The only workable alternative is a GCCollect like function BEFORE the simthread exits.
        //this->clearSentMessages();
}

void Port::clearSentMessages()
{
        LOG_DEBUG("PORT:: in model : ", this->getHost()->getName() , " deleting ", m_sentMessages.size(), " sent messages");
        for(auto& msg : m_sentMessages){
                LOG_DEBUG("PORT:: in model : ", this->getHost()->getName() , " deleting ", msg);
                msg->releaseMe();
#ifdef SAFETY_CHECKS
                msg=nullptr;
#endif
        }
	m_sentMessages.clear();
}

void Port::clearReceivedMessages()
{
	m_receivedMessages.clear();
}

void Port::addMessage(const n_network::t_msgptr& message)
{
#ifndef  NO_TRACER
	LOG_DEBUG("Added message ", message->getPayload(), ", port receiving message.");
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

void Port::addInfluencees(std::vector<std::size_t>& influences) const
{
	/*
	 * Note: we can't simply request the modelUUID because that may not have been set yet.
	 * The models are required to have been allocated already, so we can use that instead
	 */
	for (auto& port : this->m_coupled_ins){
#ifdef SAFETY_CHECKS
		AtomicModel_impl* impl = dynamic_cast<AtomicModel_impl*>(port->getHost());
		assert(impl != nullptr && "Requested getModelUUID from a non-atomic model.");
		std::size_t value =  impl->getCorenumber();
#else /* no SAFETY_CHECKS */
		std::size_t value =  reinterpret_cast<AtomicModel_impl*>(port->getHost())->getCorenumber();
#endif /* SAFETY_CHECKS */
		influences[value] = 1;
		LOG_INFO("port ", getHostName(), "/", getName(), " influenced by ", port->getHostName(), "/", port->getHostName(), " @ core ", value);
	}
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

}
