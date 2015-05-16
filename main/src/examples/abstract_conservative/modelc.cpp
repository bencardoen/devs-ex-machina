/*
 * modelc.cpp
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#include "modelc.h"

namespace n_examples_abstract_c {


ModelC::ModelC(std::string name) : CoupledModel(name) {
	t_atomicmodelptr modela = n_tools::createObject<ModelA>("modelA");
	t_atomicmodelptr modelb = n_tools::createObject<ModelB>("modelB");

	this->addSubModel(modela);
	this->addSubModel(modelb);

	this->connectPorts(modela->getPort("A"), modelb->getPort("B"));

}

}


