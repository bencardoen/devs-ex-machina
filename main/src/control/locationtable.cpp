/*
 * LocationTable.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include <control/locationtable.h>

namespace n_control {

LocationTable::LocationTable(uint amountCores)
	: m_amountCores(amountCores)
{
}

LocationTable::~LocationTable()
{
}

n_core::t_coreID LocationTable::lookupModel(std::string modelName)
{
	return m_locTable[modelName];
}

void LocationTable::registerModel(t_modelPtr model, n_core::t_coreID core)
{
	m_locTable.insert(std::pair<std::string, n_core::t_coreID>(model->getName(), core));
}

} /* namespace n_control */
