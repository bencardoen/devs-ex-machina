/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
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
        m_heap.push_back(model.get());
        //this->validateModels();
}

void DynamicCore::removeModelDS(std::size_t id) {
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " DS :: got request to remove model : ", id);
        Core::removeModel(id);
        //this->validateModels();
}

void DynamicCore::validateModels(){
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " DS ::  revalidating models : ");
        this->initializeModels();
        if(m_heap.dirty())
        	m_heap.updateAll();
        if(m_indexed_models.size() != m_heap.size()){
        	//shouldn't happen, but you never know.
        	LOG_DEBUG("Clearing scheduler.");
        	m_heap.clear();
        	for(t_atomicmodelptr& m: m_indexed_models)
        		m_heap.push_back(m.get());
        }
        this->rescheduleAll();
}
} /* namespace n_model */
