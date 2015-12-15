/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include "model/dssharedstate.h"
#include "examples/trafficlight_coupled/policemanc.h"
#include <assert.h>

namespace n_examples_ds {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

typedef n_examples_coupled::PolicemanMode PolicemanMode;

class Policeman: public AtomicModel<PolicemanMode>
{
public:
	Policeman() = delete;
	Policeman(std::string, std::size_t priority = 0);
	~Policeman() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;

	bool modelTransition(n_model::DSSharedState* shared) override;
};

}




#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_ */
