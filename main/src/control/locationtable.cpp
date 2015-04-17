/*
 * LocationTable.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#include <locationtable.h>
#include <cassert>

namespace n_control {

LocationTable::LocationTable(std::size_t amountCores)
	: m_amountCores(amountCores)
{
}

LocationTable::~LocationTable()
{
}

std::size_t LocationTable::operator [](const std::string& modelName)
{
	assert(m_locTable.find(modelName)!= m_locTable.end() && "model not in locationtable");
	return m_locTable[modelName];
}

std::size_t LocationTable::lookupModel(const std::string& modelName)
{
	assert(m_locTable.find(modelName)!= m_locTable.end() && "model not in locationtable");
	return m_locTable[modelName];
}

void LocationTable::registerModel(const t_atomicmodelptr& model, std::size_t core)
{
	m_locTable.insert(std::pair<std::string, std::size_t>(model->getName(), core));
}

} /* namespace n_control */


