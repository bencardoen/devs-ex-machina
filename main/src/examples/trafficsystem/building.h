/*
 * Building.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_

#include "atomicmodel.h"
#include "state.h"

namespace n_examples_traffic {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_network::t_timestamp;

class BuildingState: public State
{
public:
	BuildingState(std::string state);
	std::string toXML() override;
	std::string toJSON() override;
	std::string toCell() override;
	~BuildingState() {}
};

class Building: public AtomicModel
{
public:
	Building(std::string, std::size_t priority = 0);
	~Building() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;
	t_timestamp lookAhead() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_BUILDING_H_ */
