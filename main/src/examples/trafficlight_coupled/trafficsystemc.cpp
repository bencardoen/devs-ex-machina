/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Tim Tuijn
 */

#include "examples/trafficlight_coupled/trafficsystemc.h"

namespace n_examples_coupled {


TrafficSystem::TrafficSystem(std::string name) : CoupledModel(name) {
	t_atomicmodelptr policeman = n_tools::createObject<Policeman>("policeman");
	t_atomicmodelptr trafficlight = n_tools::createObject<TrafficLight>("trafficLight");

	this->addSubModel(policeman);
	this->addSubModel(trafficlight);

	this->connectPorts(policeman->getPort("OUT"), trafficlight->getPort("INTERRUPT"));

}

}

