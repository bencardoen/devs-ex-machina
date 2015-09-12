/*
 * DynamicCore.cpp
 *
 *  Created on: 7 Apr 2015
 *      Author: Ben Cardoen
 */

#include "model/dynamiccore.h"

namespace n_model {

DynamicCore::DynamicCore()
{
}

DynamicCore::~DynamicCore()
{
}

void DynamicCore::getLastImminents(std::vector<t_atomicmodelptr>& imms)
{
	imms = this->m_lastimminents;
}

void DynamicCore::signalImminent(const std::set<std::string>& imminents)
{
	this->m_lastimminents.clear();
	for (const auto& immname : imminents) {
		assert(this->containsModel(immname) && "imminent model not in core ??");
		this->m_lastimminents.push_back(this->getModel(immname));
	}
}
} /* namespace n_model */
