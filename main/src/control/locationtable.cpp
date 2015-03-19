/*
 * LocationTable.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include <locationtable.h>

namespace n_control {

LocationTable::LocationTable(std::size_t amountCores)
	: m_amountCores(amountCores)
{
}

LocationTable::~LocationTable()
{
}

std::size_t LocationTable::lookupModel(std::string modelName)
{
	return m_locTable[modelName];
}

void LocationTable::registerModel(const t_modelptr& model, std::size_t core)
{
	m_locTable.insert(std::pair<std::string, std::size_t>(model->getName(), core));
}

} /* namespace n_control */
