/*
 * rootmodel.cpp
 *
 *  Created on: 30 Mar 2015
 *      Author: matthijs
 */
#include "rootmodel.h"

namespace n_model {

n_model::RootModel::RootModel() : Model("_ROOT"), m_directConnected(false)
{
}

n_model::RootModel::~RootModel()
{
}

std::vector<t_atomicmodelptr> n_model::RootModel::directConnect(const t_coupledmodelptr&)
{
	throw std::logic_error("RootModel : directConnect not implemented");

	std::vector<t_atomicmodelptr> connectedAtomics;

	// TODO implement directConnect

	return connectedAtomics;
}

void n_model::RootModel::undoDirectConnect()
{
	throw std::logic_error("RootModel : undoDirectConnect not implemented");
	// TODO implement undoDirectConnect
}

} /* namespace n_model */
