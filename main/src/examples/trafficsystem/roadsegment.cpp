/*
 * RoadSegment.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "roadsegment.h"

namespace n_examples_traffic {
RoadSegmentState::RoadSegmentState(): State()
{
	cars_present = std::vector<std::shared_ptr<Car> >();
	query_buffer = std::vector<std::shared_ptr<Query> >();
	// deny_list = std::vector<std::shared_ptr<?> >();

	reserved = false;
	send_query_delay = t_timestamp::infinity();
	int send_query_id = -1;
	t_timestamp send_ack_delay = t_timestamp::infinity();
	int send_ack_id = -1;
	t_timestamp send_car_delay = t_timestamp::infinity();
	int send_car_id = -1;
	last_car = std::shared_ptr<Car>();
}

RoadSegmentState::RoadSegmentState(const RoadSegmentState&): RoadSegmentState()
{

}

std::string RoadSegmentState::toString()
{

}

bool operator==(const RoadSegmentState& left, const RoadSegmentState& right)
{

}

RoadSegment::RoadSegment(int district, float l, float v_max, t_timestamp observ_delay, std::string name):
		AtomicModel(name), district(district), l(l), v_max(v_max), observ_delay(observ_delay)
{
}

void RoadSegment::extTransition(const std::vector<n_network::t_msgptr> & message)
{

}

void RoadSegment::intTransition()
{

}

t_timestamp RoadSegment::timeAdvance() const
{

}

std::vector<n_network::t_msgptr> RoadSegment::output() const
{

}

t_timestamp RoadSegment::lookAhead() const
{

}

t_timestamp RoadSegment::mintime()
{

}

std::shared_ptr<RoadSegmentState> RoadSegment::getRoadSegmentState() const
{
	return std::static_pointer_cast<RoadSegmentState>(getState());
}

}


