/*
 * Intersection.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "intersection.h"
#include "constants.h"
#include <stdexcept>
#include <algorithm>    // std::find

namespace n_examples_traffic {

IntersectionState::IntersectionState(t_timestamp switch_signal):
		State(), send_query(), send_ack(), send_car(), queued_queries(), id_locations(),
		block(VERTICAL), switch_signal(switch_signal), ackDir()
{
	id_locations.push_back(-1);
	id_locations.push_back(-1);
	id_locations.push_back(-1);
	id_locations.push_back(-1);
}

IntersectionState::IntersectionState(const IntersectionState& state):
		IntersectionState(state.switch_signal)
{
	for (auto query : state.send_query) {
		send_query.push_back(std::make_shared<Query>(*query));
	}
	for (auto ack : state.send_ack) {
		send_ack.push_back(std::make_shared<QueryAck>(*ack));
	}
	for (auto car : state.send_car) {
		send_car.push_back(std::make_shared<Car>(*car));
	}
	for (auto query : state.queued_queries) {
		queued_queries.push_back(std::make_shared<Query>(*query));
	}
	id_locations = state.id_locations;
	block = state.block;
	switch_signal = state.switch_signal;
	ackDir = std::map<int, int >(state.ackDir);
}

std::string IntersectionState::toString()
{
	return "ISECT blocking " + block;
	//return "Intersection: send_query = " + str(self.send_query) + ", send_ack = " + str(self.send_ack) + ", send_car = " + str(self.send_car) + ", block = " + str(self.block)
}

Intersection::Intersection(int district, std::string name, t_timestamp switch_signal):
		AtomicModel(name), district(district), switch_signal_delay(switch_signal)
{
	setState(n_tools::createObject<IntersectionState>(switch_signal));

	q_send.push_back(addOutPort("q_send_to_n"));
	q_send.push_back(addOutPort("q_send_to_e"));
	q_send.push_back(addOutPort("q_send_to_s"));
	q_send.push_back(addOutPort("q_send_to_w"));

	q_rans.push_back(addInPort("q_rans_from_n"));
	q_rans.push_back(addInPort("q_rans_from_e"));
	q_rans.push_back(addInPort("q_rans_from_s"));
	q_rans.push_back(addInPort("q_rans_from_w"));

	q_recv.push_back(addInPort("q_recv_from_n"));
	q_recv.push_back(addInPort("q_recv_from_e"));
	q_recv.push_back(addInPort("q_recv_from_s"));
	q_recv.push_back(addInPort("q_recv_from_w"));

	q_sans.push_back(addOutPort("q_sans_to_n"));
	q_sans.push_back(addOutPort("q_sans_to_e"));
	q_sans.push_back(addOutPort("q_sans_to_s"));
	q_sans.push_back(addOutPort("q_sans_to_w"));

	car_in.push_back(addInPort("car_in_from_n"));
	car_in.push_back(addInPort("car_in_from_e"));
	car_in.push_back(addInPort("car_in_from_s"));
	car_in.push_back(addInPort("car_in_from_w"));

	car_out.push_back(addOutPort("car_out_to_n"));
	car_out.push_back(addOutPort("car_out_to_e"));
	car_out.push_back(addOutPort("car_out_to_s"));
	car_out.push_back(addOutPort("car_out_to_w"));
}

void Intersection::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	std::map<std::string, int> dir_to_int;
	dir_to_int["n"] = 0;
	dir_to_int["e"] = 1;
	dir_to_int["s"] = 2;
	dir_to_int["w"] = 3;

	std::map<int, std::string> int_to_dir;
	int_to_dir[0] = "n";
	int_to_dir[1] = "e";
	int_to_dir[2] = "s";
	int_to_dir[3] = "w";

	LOG_DEBUG("INTERSECTION: " + getName() + " - enter extTransition");
	// Simple forwarding of all messages
    // Unless the direction in which it is going is blocked
	getIntersectionState()->switch_signal = getIntersectionState()->switch_signal - m_elapsed;
	char direction_str [4] = {'0', '1', '2', '3'};
	for (unsigned int direction = 0; direction < 4; ++direction) {
		bool blocked = getIntersectionState()->block.find(direction_str[direction]);
		for (auto msg : message) {
			if(msg->getDestinationPort() == car_in[direction]->getFullName()) {
				LOG_DEBUG("INTERSECTION: " + getName() + " - expect Car");
				LOG_DEBUG("INTERSECTION: " + getName() + " - from " + msg->getSourcePort());
				std::shared_ptr<Car> car = std::make_shared<Car>(n_network::getMsgPayload<Car>(msg));
				LOG_DEBUG("INTERSECTION: " + getName() + " - received Car");
				getIntersectionState()->send_car.push_back(car);
				// Got a car, so remove its ID location entry
				getIntersectionState()->ackDir.erase(car->ID);
				auto first = getIntersectionState()->id_locations.begin();
				auto last = getIntersectionState()->id_locations.end();
				auto it = std::find(first, last, car->ID);
				if (it != last) (*it) = -1;

				auto isToRemove = [car](const std::shared_ptr<Query> query){
				    return (query->ID == car->ID);
				};



				auto first_qq = getIntersectionState()->queued_queries.begin();
				auto last_qq = getIntersectionState()->queued_queries.end();
				getIntersectionState()->queued_queries.erase(std::remove_if(first_qq, last_qq, isToRemove ), last_qq);
			}

			if(msg->getDestinationPort() == q_recv[direction]->getFullName()) {
				LOG_DEBUG("INTERSECTION: " + getName() + " - expect Query");
				LOG_DEBUG("INTERSECTION: " + getName() + " - from " + msg->getSourcePort());
				std::shared_ptr<Query> query = std::make_shared<Query>(n_network::getMsgPayload<Query>(msg));
				LOG_DEBUG("INTERSECTION: " + getName() + " - received Query");

				getIntersectionState()->id_locations.at(direction) = query->ID;
				getIntersectionState()->ackDir.at(query->ID) = dir_to_int[query->direction];
				if (blocked) {
					getIntersectionState()->send_ack.push_back(std::make_shared<QueryAck>(query->ID, t_timestamp::infinity()));
					getIntersectionState()->queued_queries.push_back(query);
				}
				else {
					getIntersectionState()->send_query.push_back(query);
				}
			}

			if(msg->getDestinationPort() == q_rans[direction]->getFullName()) {
				LOG_DEBUG("INTERSECTION: " + getName() + " - expect QueryAck");
				LOG_DEBUG("INTERSECTION: " + getName() + " - from " + msg->getSourcePort());
				std::shared_ptr<QueryAck> ack = n_network::getMsgPayload<std::shared_ptr<QueryAck> >(msg);
				LOG_DEBUG("INTERSECTION: " + getName() + " - received QueryAck");

				getIntersectionState()->ackDir.at(ack->ID) = direction;
				bool found = false;
				auto first = getIntersectionState()->id_locations.begin();
				auto last = getIntersectionState()->id_locations.end();
				auto it = std::find(first, last, ack->ID);
				if (it != last) {
					found = getIntersectionState()->block.find(direction_str[*it]);
				}
				t_timestamp t_until_dep;
				if ((ack->t_until_dep > getIntersectionState()->switch_signal) or found) {
					auto first = getIntersectionState()->id_locations.begin();
					auto last = getIntersectionState()->id_locations.end();
					auto it = std::find(first, last, ack->ID);
					if (it == last) continue;

					t_until_dep = t_timestamp::infinity();
					std::shared_ptr<Query> nquery = std::make_shared<Query>(ack->ID);
					nquery->direction = int_to_dir[direction];
					getIntersectionState()->queued_queries.push_back(nquery);
				}
				else {
					t_until_dep = ack->t_until_dep;
				}
				getIntersectionState()->send_ack.push_back(std::make_shared<QueryAck>(ack->ID, t_until_dep));
			}
		}
	}
}

void Intersection::intTransition()
{
	LOG_DEBUG("INTERSECTION: " + getName() + " - enter intTransition");
	getIntersectionState()->switch_signal = getIntersectionState()->switch_signal - timeAdvance();
	if (getIntersectionState()->switch_signal <= t_timestamp(0,000001)) {
		// We switched our traffic lights
		getIntersectionState()->switch_signal = switch_signal_delay;
		getIntersectionState()->queued_queries = std::vector<std::shared_ptr<Query> >();
		getIntersectionState()->block = (getIntersectionState()->block == HORIZONTAL)? VERTICAL : HORIZONTAL;

		for (unsigned int loc = 0; loc < getIntersectionState()->id_locations.size(); ++loc) {
			// Notify all cars that got 'green' that they should not continue
			int car_id = getIntersectionState()->id_locations.at(loc);
			if (car_id == -1) continue;
			try {
				if (getIntersectionState()->block == HORIZONTAL
						and (loc == 1 or loc == 3)) {
					std::shared_ptr<Query> query = std::make_shared<Query>(car_id);
					query->direction = Direction(getIntersectionState()->ackDir.at(0));
					getIntersectionState()->queued_queries.push_back(query);
				}
				else if (getIntersectionState()->block == VERTICAL
						and (loc == 0 or loc == 2)) {
					std::shared_ptr<Query> query = std::make_shared<Query>(car_id);
					query->direction = Direction(getIntersectionState()->ackDir.at(0));
					getIntersectionState()->queued_queries.push_back(query);
				}
			}
			catch (const std::out_of_range& oor) {
			}
		}
	}

	getIntersectionState()->send_car = std::vector<std::shared_ptr<Car> >();
	getIntersectionState()->send_query = std::vector<std::shared_ptr<Query> >();
	getIntersectionState()->send_ack = std::vector<std::shared_ptr<QueryAck> >();
}

t_timestamp Intersection::timeAdvance() const
{
	LOG_DEBUG("INTERSECTION: " + getName() + " - enter timeAdvance");
	if (getIntersectionState()->send_car.size() + getIntersectionState()->send_query.size()
			+ getIntersectionState()->send_ack.size() > 0) {
		return t_timestamp();
	}
	return (getIntersectionState()->switch_signal > t_timestamp())? getIntersectionState()->switch_signal : t_timestamp();
}

std::vector<n_network::t_msgptr> Intersection::output() const
{
	LOG_DEBUG("INTERSECTION: " + getName() + " - enter output");
	std::vector<n_network::t_msgptr> container;
	//TODO
	return container;
}


std::shared_ptr<IntersectionState> Intersection::getIntersectionState() const
{
	return std::static_pointer_cast<IntersectionState>(getState());
}

}


