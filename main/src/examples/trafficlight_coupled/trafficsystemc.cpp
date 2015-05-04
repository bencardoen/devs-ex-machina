/*
 * trafficsystemc.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#include "trafficsystemc.h"

namespace n_examples_coupled {


TrafficSystem::TrafficSystem(std::string name) : CoupledModel(name) {
	t_atomicmodelptr policeman = n_tools::createObject<Policeman>("policeman");
	t_atomicmodelptr trafficlight = n_tools::createObject<TrafficLight>("trafficLight");

	this->addSubModel(policeman);
	this->addSubModel(trafficlight);

	this->connectPorts(policeman->getPort("OUT"), trafficlight->getPort("INTERRUPT"));

}

}

