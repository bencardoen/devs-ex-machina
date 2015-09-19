/*
 * coupledmodel.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "model/coupledmodel.h"
#include "control/controller.h"
#include "cereal/types/vector.hpp"
#include "cereal/types/base_class.hpp"

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
	if(it == m_components.end()) {

		LOG_ERROR("removing model with name ", model->getName(), " (", model.get(), ')');
		LOG_ERROR(" > current models:");
		for(const auto& i:m_components) {
			LOG_ERROR("    > ", i->getName(), " (", i.get(), ')');
		}
		LOG_FLUSH;
		throw std::logic_error("CoupledModel::removeSubModel Tried to remove a model that is not mine.");
	}

	m_components.erase(it);

	model->resetParents();

	model->clearConnections();

	//remove the model itself from the controller
	if(m_control) {
		t_atomicmodelptr adevs = std::dynamic_pointer_cast<AtomicModel_impl>(model);
		if(adevs != nullptr){
			LOG_DEBUG("Removed atomic model '", model->getName(), "' while in DSDEVS");
			m_control->dsUnscheduleModel(adevs);
			return;
		}
		t_coupledmodelptr cdevs = std::static_pointer_cast<CoupledModel>(model);
		if(cdevs != nullptr){
			LOG_DEBUG("Removed coupled model '", model->getName(), "' while in DSDEVS");
			cdevs->unscheduleChildren();
			return;
		}
		throw std::logic_error("removeSubModel::unscheduleChildren found child that is not an atomic nor a coupled model.");
	} else {
		LOG_DEBUG("Removed model while not in simulation");
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
	p1->setZFunc(p2.get(), zFunction);
	p2->setInPort(p1.get());
	if(m_control)
		m_control->dsAddConnection(p1, p2, zFunction);
}

void CoupledModel::disconnectPorts(const t_portptr& p1, const t_portptr& p2)
{
	assert(this->allowDS() && "CoupledModel::disconnectPorts: Dynamic structured DEVS is not allowed in this phase.");
	//precondition: not simulating or DSDEVS
	LOG_DEBUG("CoupledModel::disconnectPorts, control: ", m_control, " ports: ", p1->getFullName(), " ->", p2->getFullName());
	p1->removeOutPort(p2.get());
	p2->removeInPort(p1.get());
	if(m_control)
		m_control->dsRemoveConnection(p1, p2);
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
		t_atomicmodelptr adevs = std::dynamic_pointer_cast<AtomicModel_impl>(model);
		if(adevs != nullptr){
			m_control->dsUnscheduleModel(adevs);
			continue;
		}
		t_coupledmodelptr cdevs = std::static_pointer_cast<CoupledModel>(model);
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

void CoupledModel::serialize(n_serialization::t_oarchive& archive)
{
	LOG_INFO("SERIALIZATION: Saving Coupled Model '", getName(), "'");
	archive(cereal::virtual_base_class<Model>(this), m_components);
}

void CoupledModel::serialize(n_serialization::t_iarchive& archive)
{
	archive(cereal::virtual_base_class<Model>(this), m_components);
	LOG_INFO("SERIALIZATION: Loaded Coupled Model '", getName(), "'");
}

void CoupledModel::load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<CoupledModel>& construct)
{
	LOG_DEBUG("COUPLEDMODEL: Load and Construct");

	construct("temp");
	construct->serialize(archive);
}

}
