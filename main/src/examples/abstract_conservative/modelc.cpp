/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Tim Tuijn
 */

#include "examples/abstract_conservative/modelc.h"

namespace n_examples_abstract_c {


ModelC::ModelC(std::string name) : CoupledModel(name) {
	t_atomicmodelptr modela = n_tools::createObject<ModelA>("modelA");
	t_atomicmodelptr modelb = n_tools::createObject<ModelB>("modelB");

	this->addSubModel(modela);
	this->addSubModel(modelb);

	this->connectPorts(modela->getPort("A"), modelb->getPort("B"));

}

}


