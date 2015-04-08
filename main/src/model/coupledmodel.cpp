/*
 * coupledmodel.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "coupledmodel.h"

namespace n_model {

/*
 * Constructor for CoupledModel
 *
 * @param name The name of the coupled model
 */
CoupledModel::CoupledModel(std::string name)
	: Model(name)
{

}

/*
 * Adds a submodel to this coupled model
 *
 * @param model The submodel that is to be added
 */
void CoupledModel::addSubModel(const t_modelptr& model)
{
	this->m_components.push_back(model);
}

/*
 * Connects the given ports with eachother (with a zFunction)
 * The zFunction will not change the message by default
 *
 * @param p1 pointer to output port number
 * @param p2 pointer to input port number
 * @param zFunction the zFunction that has to be executed on the message before transmitting it
 */
void CoupledModel::connectPorts(const t_portptr& p1, const t_portptr& p2, t_zfunc zFunction)
{
	//TODO make sure that the connection is not illegal!
	//e.g. connectPorts(portA, portA)
	p1->setZFunc(p2, zFunction);
	p2->setInPort(p1);
}

/*
 * Get all the components (submodels)
 *
 * @return a vector with pointers to these models
 */
std::vector<t_modelptr> CoupledModel::getComponents() const
{
	return m_components;
}

}
