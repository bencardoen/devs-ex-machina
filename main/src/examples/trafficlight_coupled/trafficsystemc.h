/*
 * trafficsystemc.h
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
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
