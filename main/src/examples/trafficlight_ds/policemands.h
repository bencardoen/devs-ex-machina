/*
 * policemands.h
 *
 *  Created on: Apr 19, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_

#include "atomicmodel.h"
#include "state.h"
#include "dssharedstate.h"
#include <assert.h>

namespace n_examples_ds {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class PolicemanMode: public State
{
public:
	PolicemanMode(std::string state);
	std::string toXML() override;
	std::string toJSON() override;
	std::string toCell() override;
	~PolicemanMode() {}
};

class Policeman: public AtomicModel
{
public:
	Policeman() = delete;
	Policeman(std::string, std::size_t priority = 0);
	~Policeman() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);

	bool modelTransition(n_model::DSSharedState* shared) override;
};

}




#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_DS_POLICEMANDS_H_ */
