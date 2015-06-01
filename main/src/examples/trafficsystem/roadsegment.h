/*
 * RoadSegment.h
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#ifndef SRC_EXAMPLES_TRAFFICSYSTEM_ROADSEGMENT_H_
#define SRC_EXAMPLES_TRAFFICSYSTEM_ROADSEGMENT_H_

#include "atomicmodel.h"
#include "state.h"
#include "car.h"
#include "query.h"
#include <vector>

namespace n_examples_traffic {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_model::t_portptr;
using n_network::t_timestamp;

class RoadSegmentState: public State
{
private:
	std::vector<std::shared_ptr<Car> > cars_present;
	std::vector<std::shared_ptr<Query> > query_buffer;
	//std::vector<std::shared_ptr<?> > deny_list;
	bool reserved;
	t_timestamp send_query_delay;
	int send_query_id;
	t_timestamp send_ack_delay;
	int send_ack_id;
	t_timestamp send_car_delay;
	int send_car_id;
	std::shared_ptr<Car> last_car;


public:
	RoadSegmentState();
	RoadSegmentState(const RoadSegmentState&);
	std::string toString();

	~RoadSegmentState() {}

	friend
	bool operator==(const RoadSegmentState& left, const RoadSegmentState& right);
};

class RoadSegment: public AtomicModel
{
private:
	int district;
	float l;
	float v_max;
	t_timestamp observ_delay;

	t_portptr q_rans, q_recv, car_in, entries, q_rans_bs, q_recvs_bs, q_recv_bs, Q_recv, Q_rack;
	t_portptr q_sans, q_send, car_out, exists, _q_sans_bs, Q_send, Q_sack;

public:
    RoadSegment(int district, float l, float v_max, t_timestamp observ_delay, std::string name);
	~RoadSegment() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;
	t_timestamp lookAhead() const override;

	t_timestamp mintime();
	std::shared_ptr<RoadSegmentState> getRoadSegmentState() const;
};

} // end namespace



#endif /* SRC_EXAMPLES_TRAFFICSYSTEM_ROADSEGMENT_H_ */
