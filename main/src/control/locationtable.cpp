/*
 * LocationTable.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include "control/locationtable.h"
#include <cassert>

namespace n_control {

LocationTable::LocationTable(std::size_t amountCores)
	: m_amountCores(amountCores)
{
}

LocationTable::~LocationTable()
{
}

std::size_t LocationTable::lookupModel(const std::string& modelName)
{
	return m_locTable.at(modelName);
}

void LocationTable::registerModel(const t_atomicmodelptr& model, std::size_t core)
{
	assert(core < this->m_amountCores && "Invalid core ID");
	m_locTable.insert(std::pair<std::string, std::size_t>(model->getName(), core));
}

} /* namespace n_control */


