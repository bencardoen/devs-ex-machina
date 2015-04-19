/*
 * trafficsystemds.h
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_TRAFFICSYSTEMDS_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_TRAFFICSYSTEMDS_H_

#include "atomicmodel.h"
#include "coupledmodel.h"
#include "state.h"
#include <assert.h>
#include "policemands.h"
#include "trafficlightds.h"

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
