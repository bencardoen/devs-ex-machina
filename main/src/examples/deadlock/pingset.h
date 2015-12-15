/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen
 */

#ifndef SRC_EXAMPLES_DEADLOCK_PINGSET_
#define SRC_EXAMPLES_DEADLOCK_PINGSET_

#include "examples/deadlock/ping.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_deadlock {

using n_model::CoupledModel;
using n_network::t_msgptr;


class Pingset: public CoupledModel
{
public:
	Pingset() = delete;
	Pingset(std::string name);
	~Pingset() {}
};

}

#endif
