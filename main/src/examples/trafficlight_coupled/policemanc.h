/*
 * policemanc.h
 *
 *  Created on: Mar 28, 2015
 *      Author: tim
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_

#include "atomicmodel.h"
#include "state.h"
#include <assert.h>

namespace n_examples_coupled {

using namespace n_model;
using n_network::t_msgptr;

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
	t_timestamp timeAdvance() override;
	std::vector<n_network::t_msgptr> output() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

}




#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_ */
