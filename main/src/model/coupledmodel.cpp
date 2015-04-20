/*
 * coupledmodel.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "coupledmodel.h"
#include "controller.h"

namespace n_model {

CoupledModel::CoupledModel(std::string name)
	: Model(name)
{

}

void CoupledModel::addSubModel(const t_modelptr& model)
{
	assert(this->allowDS() && "CoupledModel::addSubModel: Dynamic structured DEVS is not allowed in this phase.");
	//precondition: not simulating or DSDEVS
	this->m_components.push_back(model);
	if(m_control){
		model->setController(m_control);
		m_control->dsScheduleModel(model);
	}
}


void CoupledModel::removeSubModel(t_modelptr& model)
{
	assert(this->allowDS() && "CoupledModel::removeSubModel: Dynamic structured DEVS is not allowed in this phase.");
	//precondition: not simulating or DSDEVS
	//remove the model itself
	std::vector<t_modelptr>::iterator it = std::find(m_components.begin(), m_components.end(), model);
	if(it != m_components.end())
		throw std::logic_error("CoupledModel::removeSubModel Tried to remove a model that is not mine.");

	m_components.erase(it);
	model->resetParents();
	//remove all connections to this model
	// loop over all models & remove their connections with this model
	//   assumes out -> in
	for(t_modelptr& mod: m_components){
		removeConnectionsBetween(mod, model);
	}
	// remove connections between model and the ports of this model
	//   assumes out -> out or in -> in
	for(std::map<std::string, t_portptr>::value_type& port1 : m_iPorts){
		for(std::map<std::string, t_portptr>::value_type& port2 : model->getIPorts()){
			disconnectPorts(port1.second, port2.second);
		}
	}
	for(std::map<std::string, t_portptr>::value_type& port1 : model->getOPorts()){
		for(std::map<std::string, t_portptr>::value_type& port2 : m_oPorts){
			disconnectPorts(port1.second, port2.second);
		}
	}

	//remove the model itself from the controller
	if(m_control) {
		t_atomicmodelptr adevs = std::dynamic_pointer_cast<AtomicModel>(model);
		if(adevs != nullptr){
			m_control->dsUnscheduleModel(adevs);
			return;
		}
		t_coupledmodelptr cdevs = std::dynamic_pointer_cast<CoupledModel>(model);
		if(cdevs != nullptr){
			cdevs->unscheduleChildren();
			return;
		}
		throw std::logic_error("removeSubModel::unscheduleChildren found child that is not an atomic nor a coupled model.");
	}
}

void CoupledModel::resetParents()
{
	m_parent.reset();
	for (auto& child : m_components)
		child->resetParents();
}

void CoupledModel::connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction)
{
	assert(this->allowDS() && "CoupledModel::connectPorts: Dynamic structured DEVS is not allowed in this phase.");
	//precondition: not simulating or DSDEVS
	assert(isLegalConnection(p1, p2) && "CoupledModel::connectPorts: Illegal connection between ports.");
	//e.g. connectPorts(portA, portA)
	p1->setZFunc(p2, zFunction);
	p2->setInPort(p1);
	if(m_control)
		m_control->dsAddConnection(p1, p2, zFunction);
}

void CoupledModel::disconnectPorts(const t_portptr& p1, const t_portptr& p2)
{
	assert(this->allowDS() && "CoupledModel::disconnectPorts: Dynamic structured DEVS is not allowed in this phase.");
	//precondition: not simulating or DSDEVS
	p1->removeOutPort(p2);
	p2->removeInPort(p1);
	if(m_control)
		m_control->dsRemoveConnection(p1, p2);
}

void CoupledModel::removeConnectionsBetween(t_modelptr& mod1, t_modelptr& mod2)
{
	for(std::map<std::string, t_portptr>::value_type& port1 : mod1->getOPorts()){
		for(std::map<std::string, t_portptr>::value_type& port2 : mod2->getIPorts()){
			disconnectPorts(port1.second, port2.second);
		}
	}
	for(std::map<std::string, t_portptr>::value_type& port1 : mod2->getOPorts()){
		for(std::map<std::string, t_portptr>::value_type& port2 : mod1->getIPorts()){
			disconnectPorts(port1.second, port2.second);
		}
	}
}

bool CoupledModel::isLegalConnection(const t_portptr& p1, const t_portptr& p2) const
{
	//make sure that the connection is not illegal
	//figure out whose port these actually are
	std::size_t isOwner = 0;
	if(p1->getHostName() == getName()) isOwner = 1;
	else {
		bool found = false;
		for(const t_modelptr& mod : m_components)
			if(mod->getName() == p1->getHostName())
				found = true;
		if(!found) return false;
	}
	if(p2->getHostName() == getName()) isOwner |= 2;
	else {
		bool found = false;
		for(const t_modelptr& mod : m_components)
			if(mod->getName() == p2->getHostName())
				found = true;
		if(!found) return false;
	}

	switch(isOwner){
	case 0u:	//both of a submodel
	case 3u:	//both of this coupled model
		return (!p1->isInPort() && p2->isInPort());
		break;
	case 1:		//left is ours, right is not
		return p2->isInPort();
		break;
	case 2:		//right is ours, left is not
		return !p1->isInPort();
	}
	return true;
}

void CoupledModel::unscheduleChildren()
{
	if(!m_control)
		return;
	for(t_modelptr& model: m_components){
		t_atomicmodelptr adevs = std::dynamic_pointer_cast<AtomicModel>(model);
		if(adevs != nullptr){
			m_control->dsUnscheduleModel(adevs);
			continue;
		}
		t_coupledmodelptr cdevs = std::dynamic_pointer_cast<CoupledModel>(model);
		if(cdevs != nullptr){
			cdevs->unscheduleChildren();
			continue;
		}
		throw std::logic_error("CoupledModel::unscheduleChildren found child that is not an atomic nor a coupled model.");
	}
}

std::vector<t_modelptr> CoupledModel::getComponents() const
{
	return m_components;
}


void CoupledModel::setController(n_control::Controller* newControl)
{
	m_control = newControl;
	for(t_modelptr& model: m_components)
		model->setController(newControl);
}

}
