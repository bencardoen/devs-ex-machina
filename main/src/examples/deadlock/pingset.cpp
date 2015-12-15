/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#include "examples/deadlock/pingset.h"

namespace n_examples_deadlock {


Pingset::Pingset(std::string name) : CoupledModel(name) {
    t_atomicmodelptr p1 = n_tools::createObject<Ping>("P1");
    t_atomicmodelptr p2 = n_tools::createObject<Ping>("P2");
    t_atomicmodelptr p3 = n_tools::createObject<Ping>("P3");

    this->addSubModel(p1);
    this->addSubModel(p2);
    this->addSubModel(p3);

    this->connectPorts(p1->getPort("_out"), p2->getPort("_in"));
    this->connectPorts(p2->getPort("_out"), p3->getPort("_in"));
    this->connectPorts(p3->getPort("_out"), p1->getPort("_in"));

}

}


