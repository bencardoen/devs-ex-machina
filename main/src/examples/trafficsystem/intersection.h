/*
 * Intersection.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_INTERSECTION_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_INTERSECTION_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include "examples/trafficsystem/car.h"
#include "examples/trafficsystem/query.h"
#include "examples/trafficsystem/queryack.h"
#include <vector>
#include <map>

namespace n_examples_traffic {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_model::t_portptr;
using n_network::t_timestamp;

class IntersectionState: public State
{
	friend class Intersection;

private:
	std::vector<std::shared_ptr<Query> > send_query;
	std::vector<std::shared_ptr<QueryAck> > send_ack;
	std::vector<std::shared_ptr<Car> > send_car;
	std::vector<std::shared_ptr<Query> > queued_queries;
	std::vector<int> id_locations;
	std::string block;
	t_timestamp switch_signal;
	std::map<int, int> ackDir;


public:
	IntersectionState(t_timestamp switch_signal);
	IntersectionState(const IntersectionState&);
	std::string toString();

	~IntersectionState() {}
};

class Intersection: public AtomicModel
{
	friend class City;

private:
	int district;
	t_timestamp switch_signal_delay;

	std::vector<t_portptr> q_send, q_rans, q_recv, q_sans, car_in, car_out;

public:
    Intersection(int district, std::string name = "Intersection", t_timestamp switch_signal = t_timestamp(30));
	~Intersection() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;

	std::shared_ptr<IntersectionState> getIntersectionState() const;
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_INTERSECTION_H_ */
