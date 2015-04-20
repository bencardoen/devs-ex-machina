/*
 * trafficsystemds.cpp
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "trafficsystemds.h"

namespace n_examples_ds {


TrafficSystem::TrafficSystem(std::string name) : CoupledModel(name) {
	m_policeman = std::make_shared<Policeman>("policeman");
	m_trafficlight1 = std::make_shared<TrafficLight>("trafficLight1");
	m_trafficlight2 = std::make_shared<TrafficLight>("trafficLight2");

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

