/*
 * RoadSegment.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "examples/trafficsystem/roadsegment.h"
#include "examples/trafficsystem/queryack.h"
#include <sstream>

namespace n_examples_traffic {
RoadSegmentState::RoadSegmentState(): State()
{
	cars_present = std::vector<std::shared_ptr<Car> >();
	query_buffer = std::vector<int>();
	deny_list = std::vector<std::shared_ptr<QueryAck> >();

	reserved = false;
	send_query_delay = t_timestamp::infinity();
	send_query_id = -1;
	send_ack_delay = t_timestamp::infinity();
	send_ack_id = -1;
	send_car_delay = t_timestamp::infinity();
	send_car_id = -1;
	last_car = -1;
}

RoadSegmentState::RoadSegmentState(const RoadSegmentState& state): RoadSegmentState()
{
	for (auto car : state.cars_present) {
		cars_present.push_back(std::make_shared<Car>(*car));
	}
	for (auto elem : state.deny_list) {
		deny_list.push_back(std::shared_ptr<QueryAck>(elem));
	}
	query_buffer = state.query_buffer;
	reserved = state.reserved;
	send_query_delay = state.send_query_delay;
	send_query_id = state.send_query_id;
	send_ack_delay = state.send_ack_delay;
	send_ack_id = state.send_ack_id;
	send_car_delay = state.send_car_delay;
	send_car_id = state.send_car_id;
	last_car = state.last_car;
}

std::string RoadSegmentState::toString()
{
	std::stringstream ss;
	ss << "Road segment: cars_present = [";
	for (auto car : cars_present) {
		ss << car->getID() << ", ";
	}
	ss << "] , send_query_delay = " << send_query_delay;
	ss << ", send_ack_delay = " << send_ack_delay;
	ss << ", send_car_delay = " << send_car_delay;
	ss << ", send_ack_id = " << send_ack_id;

	return ss.str();
}

bool operator==(const RoadSegmentState& left, const RoadSegmentState& right)
{
	if (left.query_buffer != right.query_buffer)
		return false;
	if (not (left.send_query_delay == right.send_query_delay and
	      left.send_ack_delay == right.send_ack_delay and
	      left.send_car_delay == right.send_car_delay and
	      left.send_query_id == right.send_query_id and
	      left.send_ack_id == right.send_ack_id and
	      left.send_car_id == right.send_car_id))
	    return false;
	if (left.reserved != right.reserved)
	    return false;
	if (left.cars_present.size() != right.cars_present.size())
	    return false;
	for (unsigned int i = 0; i < left.cars_present.size(); ++i) {
		 if (left.cars_present.at(i) != right.cars_present.at(i))
			return false;
	}
	if (left.last_car != right.last_car)
	    return false;
	if (left.deny_list.size() != right.deny_list.size())
	    return false;
	for (unsigned int i = 0; i < left.deny_list.size(); ++i) {
		 if (left.deny_list.at(i) != right.deny_list.at(i))
			return false;
	}
	return true;
}

RoadSegment::RoadSegment(int district, float l, float v_max, t_timestamp observ_delay, std::string name):
		AtomicModel_impl(name), district(district), l(l), v_max(v_max), observ_delay(observ_delay)
{
	setState(n_tools::createObject<RoadSegmentState>());

	// in-ports
	q_rans = addInPort("q_rans");
	q_recv = addInPort("q_recv");
	car_in = addInPort("car_in");
	entries = addInPort("entries");
	q_rans_bs = addInPort("q_rans_bs");
	q_recv_bs = addInPort("q_recv_bs");
	// compatibility bindings...
	Q_recv = q_recv;
	Q_rack = q_rans;

	// out-ports
	q_sans = addOutPort("q_sans");
	q_send = addOutPort("q_send");
	car_out = addOutPort("car_out");
	exits = addOutPort("exits");

	q_sans_bs = addOutPort("q_sans_bs");
	// compatibility bindings...
	Q_send = q_send;
	Q_sack = q_sans;
}

void RoadSegment::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	LOG_DEBUG("ROADSEGMENT: " + getName() + " - enter extTransition");
	getRoadSegmentState()->send_query_delay = getRoadSegmentState()->send_query_delay - m_elapsed;
	getRoadSegmentState()->send_ack_delay = getRoadSegmentState()->send_ack_delay - m_elapsed;
	getRoadSegmentState()->send_car_delay = getRoadSegmentState()->send_car_delay - m_elapsed;

	for (auto car : getRoadSegmentState()->cars_present) {
		car->remaining_x -= m_elapsed.getTime() * car->v;
	}
	for (auto msg : message) {
		if(msg->getDestinationPort() == Q_recv->getFullName()
				or msg->getDestinationPort() == q_recv_bs->getFullName() ) {
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - expect Query");
			Query query = n_network::getMsgPayload<Query>(msg);
		}

		if(msg->getDestinationPort() == car_in->getFullName()
				or msg->getDestinationPort() == entries->getFullName() ) {
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - expect Car");
			std::shared_ptr<Car> car = std::make_shared<Car>(n_network::getMsgPayload<Car>(msg));

			getRoadSegmentState()->last_car = -1;
			car->remaining_x = l;
			getRoadSegmentState()->cars_present.push_back(car);

			if (getRoadSegmentState()->cars_present.size() != 1) {
				for (auto other_car : getRoadSegmentState()->cars_present) {
					other_car->v = 0;
				}

				getRoadSegmentState()->send_query_delay = t_timestamp::infinity();
				getRoadSegmentState()->send_ack_delay = t_timestamp::infinity();
				getRoadSegmentState()->send_car_delay = t_timestamp::infinity();
			}
			else {
				getRoadSegmentState()->send_query_delay = t_timestamp();
				getRoadSegmentState()->send_query_id = car->ID;

				t_timestamp t_to_dep;
				float v_last_car = getRoadSegmentState()->cars_present.back()->v;
				if (v_last_car == 0) {
					t_to_dep = t_timestamp::infinity();
				}
				else if ((l / v_last_car) > 0){
					t_to_dep = l / v_last_car;
				}
				else {
					t_to_dep = 0;
				}

				getRoadSegmentState()->send_car_delay = t_to_dep;
				getRoadSegmentState()->send_car_id = car->ID;
			}
		}

		if(msg->getDestinationPort() == Q_rack->getFullName()
				or msg->getDestinationPort() == q_rans_bs->getFullName() ) {
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - expect QueryAck");
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - from " + msg->getSourcePort());
			std::shared_ptr<QueryAck> ack = n_network::getMsgPayload<std::shared_ptr<QueryAck> >(msg);
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - received QueryAck");

			if(getRoadSegmentState()->cars_present.size() == 1
					and ack->ID == getRoadSegmentState()->cars_present.at(0)->ID) {
				std::shared_ptr<Car> car = getRoadSegmentState()->cars_present.at(0);
				t_timestamp t_no_col = ack->t_until_dep;
				float v_old = car->v;
				float v_pref = car->v_pref;
				int remaining_x = car->remaining_x;

				float v_new;
				t_timestamp t_until_dep;

				if (t_no_col + 1 == t_no_col) {
					v_new = 0;
					t_until_dep = t_timestamp::infinity();
				}
				else {
					float v = (v_pref > v_max)? v_max : v_pref;
					unsigned int divider = remaining_x / v;
					divider = (t_no_col.getTime() > divider)? t_no_col.getTime() : divider;
					float v_ideal = float(remaining_x) / float(divider);

					float diff = v_ideal - v_old;
					if (diff < 0) {
						if (-diff > car->dv_neg_max) {
							diff = -car->dv_neg_max;
						}
					}
					else if (diff > 0) {
						if (diff > car->dv_pos_max) {
							diff = car->dv_pos_max;
						}
					}
					v_new = v_old + diff;
					if (v_new == 0) {
						t_until_dep = t_timestamp::infinity();
					}
					else {
						t_until_dep = t_timestamp(car->remaining_x / v_new);
					}
				}

				car->v = v_new;
				t_until_dep = (t_until_dep > t_timestamp(0))? t_until_dep : t_timestamp(0);

				if (t_until_dep > getRoadSegmentState()->send_car_delay
						and getRoadSegmentState()->last_car != -1) {
					getRoadSegmentState()->send_ack_delay = observ_delay;
					getRoadSegmentState()->send_ack_id = getRoadSegmentState()->last_car;
				}
				getRoadSegmentState()->send_car_delay = t_until_dep;
				getRoadSegmentState()->send_car_id = ack->ID;

				if (t_until_dep == t_timestamp::infinity()) {
					getRoadSegmentState()->send_query_id = ack->ID;
				}
				else {
					getRoadSegmentState()->send_query_id = -1;
				}
				getRoadSegmentState()->send_query_delay = t_timestamp::infinity();
			}
		}
	}
}

void RoadSegment::intTransition()
{
	LOG_DEBUG("ROADSEGMENT: " + getName() + " - enter intTransition");
	t_timestamp mintime = this->mintime();
	getRoadSegmentState()->send_query_delay = getRoadSegmentState()->send_query_delay - mintime;
	getRoadSegmentState()->send_ack_delay = getRoadSegmentState()->send_query_delay - mintime;
	getRoadSegmentState()->send_car_delay = getRoadSegmentState()->send_query_delay - mintime;
	if (getRoadSegmentState()->send_ack_delay == t_timestamp()) {
		getRoadSegmentState()->send_ack_delay = t_timestamp::infinity();
		getRoadSegmentState()->send_ack_id = -1;
	}
	else if (getRoadSegmentState()->send_query_delay == t_timestamp()) {
		// Just sent a query, now deny all other queries and wait until the current car has left
		getRoadSegmentState()->send_query_delay = t_timestamp::infinity();
		getRoadSegmentState()->send_query_id = -1;
	}
	else if (getRoadSegmentState()->send_car_delay == t_timestamp()) {
		getRoadSegmentState()->cars_present = std::vector<std::shared_ptr<Car> >();
		getRoadSegmentState()->send_car_delay = t_timestamp::infinity();
		getRoadSegmentState()->send_car_id = -1;

		// A car has left, so we can answer to the first other query we received
		if (not getRoadSegmentState()->query_buffer.empty()) {
			getRoadSegmentState()->send_ack_delay = observ_delay;
			getRoadSegmentState()->send_ack_id = getRoadSegmentState()->query_buffer.back();
			getRoadSegmentState()->query_buffer.pop_back();
		}
		else {
			// No car is waiting for this segment, so 'unreserve' it
			getRoadSegmentState()->reserved = false;
		}
	}

	if (getRoadSegmentState()->send_ack_id == -1 and not getRoadSegmentState()->deny_list.empty()) {
		getRoadSegmentState()->send_ack_delay = observ_delay;
		getRoadSegmentState()->send_ack_id = getRoadSegmentState()->deny_list.at(0)->ID;
		getRoadSegmentState()->deny_list.erase(getRoadSegmentState()->deny_list.begin());
	}
}

t_timestamp RoadSegment::timeAdvance() const
{
	LOG_DEBUG("ROADSEGMENT: " + getName() + " - enter timeAdvance");
	t_timestamp delay = mintime();
	// Take care of floating point errors
	return (delay > t_timestamp())? delay : t_timestamp();
}

std::vector<n_network::t_msgptr> RoadSegment::output() const
{
	LOG_DEBUG("ROADSEGMENT: " + getName() + " - enter output");
	t_timestamp mintime = this->mintime();
	std::vector<n_network::t_msgptr> container;
	if (getRoadSegmentState()->send_ack_delay == mintime) {
		t_timestamp t_until_dep;
		int ackID = getRoadSegmentState()->send_ack_id;
		if (getRoadSegmentState()->cars_present.size() == 0) {
			t_until_dep = t_timestamp();
		}
		else if (getRoadSegmentState()->cars_present.at(0)->v == 0) {
			t_until_dep = t_timestamp::infinity();
		}
		else {
			t_until_dep = t_timestamp(l / getRoadSegmentState()->cars_present.at(0)->v);
		}
		std::shared_ptr<QueryAck> ack = std::make_shared<QueryAck>(ackID, t_until_dep);
		Q_sack->createMessages(ack, container);
		q_sans_bs->createMessages(ack, container);
	}
	else if (getRoadSegmentState()->send_query_delay == mintime) {
		std::shared_ptr<Query> query = std::make_shared<Query>(getRoadSegmentState()->send_query_id);
		if (not getRoadSegmentState()->cars_present.at(0)->path.empty()) {
			query->direction = getRoadSegmentState()->cars_present.at(0)->path.at(0);
			Q_send->createMessages(query, container);
		}
	}
	else if (getRoadSegmentState()->send_car_delay == mintime) {
		std::shared_ptr<Car> car = getRoadSegmentState()->cars_present.at(0);
		car->distance_travelled += l;
		if (car->path.empty()) {
			exits->createMessages(car, container);
		}
		else {
			car_out->createMessages(car, container);
		}
	}

	return container;
}


t_timestamp RoadSegment::mintime() const
{
	if (getRoadSegmentState()->send_query_delay < getRoadSegmentState()->send_ack_delay) {
		return ((getRoadSegmentState()->send_query_delay < getRoadSegmentState()->send_car_delay)?
				getRoadSegmentState()->send_query_delay : getRoadSegmentState()->send_car_delay);
	}
	else {
		return ((getRoadSegmentState()->send_ack_delay < getRoadSegmentState()->send_car_delay)?
				getRoadSegmentState()->send_ack_delay : getRoadSegmentState()->send_ack_delay);
	}
}

std::shared_ptr<RoadSegmentState> RoadSegment::getRoadSegmentState() const
{
	return std::static_pointer_cast<RoadSegmentState>(getState());
}

}


