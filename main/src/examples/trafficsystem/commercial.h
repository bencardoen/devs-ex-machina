/*
 * Commercial.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_COMMERCIAL_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_COMMERCIAL_H_

#include "building.h"
#include "car.h"

namespace n_examples_traffic {

class CommercialState: public State
{
	friend class Commercial;

private:
	std::shared_ptr<Car> car;

public:
	CommercialState(std::shared_ptr<Car> car);
	CommercialState();
	CommercialState(const CommercialState&);
	std::string toString() const;

	~CommercialState() {}
};

class Commercial: public Building
{
private:
    t_portptr toCollector;

public:
    Commercial(int district, std::string name = "Commercial");
	~Commercial() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	std::shared_ptr<CommercialState> getCommercialState() const;
};

} // end namespace

#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_COMMERCIAL_H_ */
