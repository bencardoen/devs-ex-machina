/*
 * coupledmodel.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "coupledmodel.h"

namespace n_model {

CoupledModel::CoupledModel(std::string name)
	: Model(name)
{

}

void CoupledModel::addSubModel(const t_modelptr& model)
{
	this->m_components.push_back(model);
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
}

std::vector<t_modelptr> CoupledModel::getComponents() const
{
	return m_components;
}

}
