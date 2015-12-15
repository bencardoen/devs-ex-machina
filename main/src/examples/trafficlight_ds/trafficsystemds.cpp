/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#include "examples/trafficlight_ds/trafficsystemds.h"

namespace n_examples_ds {


TrafficSystem::TrafficSystem(std::string name) : CoupledModel(name) {
	m_policeman = n_tools::createObject<Policeman>("policeman");
	m_trafficlight1 = n_tools::createObject<TrafficLight>("trafficLight1");
	m_trafficlight2 = n_tools::createObject<TrafficLight>("trafficLight2");

	this->addSubModel(m_policeman);
	this->addSubModel(m_trafficlight1);
	this->addSubModel(m_trafficlight2);

	this->connectPorts(m_policeman->getPort("OUT"), m_trafficlight1->getPort("INTERRUPT"));

}

bool TrafficSystem::modelTransition(n_model::DSSharedState* shared)
{
	std::string val = "";
	const auto it = shared->values.find("destination");
	if(it != shared->values.end())
		val = it->second;

	LOG_DEBUG("TrafficSystem::modelTransition with destination = ", val);
	if (val == "1"){
		disconnectPorts(m_policeman->getPort("OUT"), m_trafficlight2->getPort("INTERRUPT"));
		connectPorts(m_policeman->getPort("OUT"), m_trafficlight1->getPort("INTERRUPT"));
	}
	else if (val == "2"){
		disconnectPorts(m_policeman->getPort("OUT"), m_trafficlight1->getPort("INTERRUPT"));
		connectPorts(m_policeman->getPort("OUT"), m_trafficlight2->getPort("INTERRUPT"));
	}
	return false;
}

}

