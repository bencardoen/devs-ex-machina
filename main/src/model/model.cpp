/*
 * model.cpp
 *
 *  Created on: Mar 17, 2015
 *      Author: tim
 */

#include "model/model.h"
#include "control/controller.h"
#include <algorithm>

namespace n_model {

Model::Model(std::string name)
	: m_name(name), removedInPort(false), removedOutPort(false), m_control(nullptr), m_parent(nullptr)
{
}

std::string Model::getName() const
{
	return n_tools::copyString(m_name);
}

const t_portptr& Model::getPort(std::string name) const
{
	for(const t_portptr& ptr: m_iPorts)
		if(ptr->getName() == name)
			return ptr;
	for(const t_portptr& ptr: m_oPorts)
		if(ptr->getName() == name)
			return ptr;
#ifdef SAFETY_CHECKS
        LOG_ERROR("Port with name not found :: ", name , " in model ", this->m_name);
        LOG_FLUSH;
#endif
        throw std::logic_error("Port not found!");
}

void Model::setParent(Model* parent)
{
	m_parent = parent;
}

const Model* Model::getParent() const
{
	return m_parent;
}

Model* Model::getParent()
{
	return m_parent;
}

void Model::resetParent()
{
	m_parent = nullptr;
}

t_portptr Model::addPort(std::string name, bool isIn)
{
	LOG_DEBUG("adding port with name ", name, ", input? ", isIn, " to model ", m_name);
	LOG_DEBUG("> this model has a controller?", (m_control? 1:0));
	assert(allowDS() && "Model::addPort: Dynamic structured DEVS is not allowed in this phase.");
	// Find new name for port if name was empty
	std::size_t id = isIn? m_iPorts.size() : m_oPorts.size();
	t_portptr port(n_tools::createObject<Port>(name, this, id, isIn));

	if (isIn)
		m_iPorts.push_back(port);
	else
		m_oPorts.push_back(port);

	if (m_control) {
		m_control->dsUndoDirectConnect();
	}
        
	return port;
}

void Model::removePort(t_portptr& port)
{
	//remove the port itself
	assert(allowDS() && "Dynamic structured DEVS is not allowed in this phase.");
	port->clearConnections();
	if (port->isInPort()) {
		m_iPorts[port->getPortID()] = nullptr;
		removedInPort = true;
	} else {
		m_oPorts[port->getPortID()] = nullptr;
		removedOutPort = true;
	}

	if (m_control)
		m_control->dsRemovePort(port);

}

void Model::clearConnections()
{
	assert(allowDS() && "Dynamic structured DEVS is not allowed in this phase.");
	for(auto& ptr: m_iPorts)
		ptr->clearConnections();
	for(auto& ptr: m_oPorts)
		ptr->clearConnections();
}

t_portptr Model::addInPort(const std::string& name)
{
	return this->addPort(name, true);
}

t_portptr Model::addOutPort(const std::string& name)
{
	return this->addPort(name, false);
}

const std::vector<t_portptr>& Model::getIPorts() const
{
	return m_iPorts;
}

const std::vector<t_portptr>& Model::getOPorts() const
{
	return m_oPorts;
}

std::vector<t_portptr>& Model::getIPorts()
{
	return m_iPorts;
}

std::vector<t_portptr>& Model::getOPorts()
{
	return m_oPorts;
}

bool Model::doModelTransition(DSSharedState* st)
{
	bool result = modelTransition(st);
	if(removedInPort){
		//first, remove all nullptr
		m_oPorts.erase(
			std::remove_if(m_iPorts.begin(), m_iPorts.end(),
				[](const t_portptr& x){return (x == nullptr);}),
			m_iPorts.end());
		//then, fix all id's so that they are their vector index again
		for(std::size_t i = 0; i < m_iPorts.size(); ++i){
			assert(m_iPorts[i] != nullptr && "Did not remove a nullptr");
			m_iPorts[i]->setPortID(i);
		}
		removedInPort = false;
	}
	if(removedOutPort){
		//first, remove all nullptr
		m_oPorts.erase(
			std::remove_if(m_oPorts.begin(), m_oPorts.end(),
				[](const t_portptr& x){return (x == nullptr);}),
			m_oPorts.end());
		//then, fix all id's so that they are their vector index again
		for(std::size_t i = 0; i < m_oPorts.size(); ++i){
			assert(m_oPorts[i] != nullptr && "Did not remove a nullptr");
			m_oPorts[i]->setPortID(i);
		}
		removedOutPort = false;
	}
	return result;
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

void Model::clearState()
{
        for (const auto& port : m_oPorts){
		port->clearSentMessages();
	}
}

}
