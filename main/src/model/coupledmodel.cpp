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
	this->m_components.push_back(model);
	if(m_control)
		m_control->dsScheduleModel(model);
}


void CoupledModel::removeSubModel(t_modelptr& model)
{
	//TODO CoupledModel::removeSubModel
	//remove all to this model
	//remove the model itself
	if(m_control)
		m_control->dsUnscheduleModel(model);
}

void CoupledModel::resetParents()
{
	m_parent.reset();
	for (auto& child : m_components)
		child->resetParents();
}

void CoupledModel::connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction)
{
	//TODO make sure that the connection is not illegal!
	//e.g. connectPorts(portA, portA)
	p1->setZFunc(p2, zFunction);
	p2->setInPort(p1);
	if(m_control)
		m_control->dsAddConnection(p1, p2, zFunction);
}

void CoupledModel::disconnectPorts(const t_portptr& p1, const t_portptr& p2)
{
	p1->removeOutPort(p2);
	p2->removeInPort(p1);
	if(m_control)
		m_control->dsRemoveConnection(p1, p2);
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
