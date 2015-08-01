/*
 * Collector.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_COLLECTOR_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_COLLECTOR_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include "examples/trafficsystem/car.h"
#include <vector>

namespace n_examples_traffic {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_model::t_portptr;
using n_network::t_timestamp;

class CollectorState: public State
{
	friend class Collector;

private:
	std::vector<std::shared_ptr<Car> > cars;


public:
	CollectorState();
	CollectorState(const CollectorState&);
	std::string toString() override;

	~CollectorState() {}
};

class Collector: public AtomicModel
{
	friend class City;

private:
    int district;

    t_portptr car_in;

public:
    Collector();
	~Collector() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	std::shared_ptr<CollectorState> getCollectorState() const;
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_COLLECTOR_H_ */
