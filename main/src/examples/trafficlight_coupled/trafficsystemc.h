/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author:Tim Tuijn, Pieter Lauwers
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/state.h"
#include <assert.h>
#include "examples/trafficlight_coupled/policemanc.h"
#include "examples/trafficlight_coupled/trafficlightc.h"

namespace n_examples_coupled {

using n_model::CoupledModel;
using n_network::t_msgptr;

class TrafficSystem: public CoupledModel
{
public:
	TrafficSystem() = delete;
	TrafficSystem(std::string name);
	~TrafficSystem() {}
};

}

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_ */
