/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_TRAFFICSYSTEMDS_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_TRAFFICSYSTEMDS_H_

#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/state.h"
#include <assert.h>
#include "examples/trafficlight_ds/policemands.h"
#include "examples/trafficlight_ds/trafficlightds.h"

namespace n_examples_ds {

using n_model::CoupledModel;
using n_network::t_msgptr;

class TrafficSystem: public CoupledModel
{
private:
	t_atomicmodelptr m_policeman;
	t_atomicmodelptr m_trafficlight1;
	t_atomicmodelptr m_trafficlight2;
public:
	TrafficSystem() = delete;
	TrafficSystem(std::string name);
	~TrafficSystem() {}

	bool modelTransition(n_model::DSSharedState* shared) override;
};

}

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_TRAFFICSYSTEMDS_H_ */
