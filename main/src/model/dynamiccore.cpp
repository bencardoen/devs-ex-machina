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

void DynamicCore::getLastImminents(std::vector<t_raw_atomic>& imms)
{
	imms = this->m_lastimminents;
}

void DynamicCore::signalImminent(const std::vector<t_raw_atomic>& imminents)
{
	this->m_lastimminents.clear();
	m_lastimminents=imminents;
}

void DynamicCore::addModelDS(const t_atomicmodelptr& model){
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " DS ::  got model : ", model->getName());
        Core::addModel(model);
        model->setTime(this->getTime().getTime());
        //this->validateModels();
}

void DynamicCore::removeModelDS(const std::string& name){
        const t_atomicmodelptr model = this->getModel(name);
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " DS :: got request to remove model : ", model->getName());
        Core::removeModel(name);
        //this->validateModels();
}

void DynamicCore::validateModels(){
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " DS ::  revalidating models : ");
        this->initializeModels();
        this->rescheduleAll();
}
} /* namespace n_model */
