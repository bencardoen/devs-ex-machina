/*
 * Building.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "examples/trafficsystem/building.h"
#include <sstream>
#include <stdlib.h>
#include <time.h>
#include "examples/trafficsystem/car.h"
#include "examples/trafficsystem/query.h"
#include "examples/trafficsystem/queryack.h"

namespace n_examples_traffic {

BuildingState::BuildingState(int IAT_min, int IAT_max, std::shared_ptr<std::vector<std::string> > path, std::string name):
		State(), name(name), path(path)
{
	LOG_DEBUG("BUILDING: Creating building state " + name);
	currentTime = 0;

	srand (time(NULL));
	float seed = rand();
	randomGenerator = std::mt19937(seed);
	setSendQueryDelay(IAT_min, IAT_max);

	send_car_delay = t_timestamp::infinity();

	if (path->empty()) {
		send_query_delay = t_timestamp::infinity();
	}

	int part1, part2;
	std::string temp = name.substr(name.find("_") + 1, name.length());
	std::istringstream buffer1(temp.substr(0, temp.find("_")));
	std::istringstream buffer2(temp.substr(temp.find("_") + 1, name.length()));
	buffer1 >> part1;
	buffer2 >> part2;
	send_query_id = part1 * 1000 + part2;

	send_car_id = send_query_id;
	next_v_pref = 0;
	sent = 0;
	LOG_DEBUG("BUILDING: Done creating building state " + name);
}

BuildingState::BuildingState(const BuildingState& state): BuildingState(0, 0, state.path, state.name)
{
	currentTime = state.currentTime;
	send_query_delay = state.send_query_delay;
	send_car_delay = state.send_car_delay;
	send_query_id = state.send_query_id;
	send_car_id = state.send_car_id;
	next_v_pref = state.next_v_pref;
	randomGenerator = state.randomGenerator;
	sent = state.sent;
}

std::string BuildingState::toString()
{
	if (not path->empty()) {
		std::stringstream ss;
		ss << "Residence: send_query_delay = " << send_query_delay;
		ss << ", send_query_id = " << send_query_id;
		ss << ", send_car_delay = " << send_car_delay;
		ss << ", send_car_id = " << send_car_id;
		return ss.str();
	}
	else {
		 return "Commercial: waiting...";
	}
}

void BuildingState::setNextVPref(int v_pref_min, int v_pref_max)
{
	float random = (float) randomGenerator() / (float) randomGenerator.max();
	next_v_pref = v_pref_min + (random * (v_pref_max - v_pref_min));
}

void BuildingState::setSendQueryDelay(int IAT_min, int IAT_max)
{
	float random = (float) randomGenerator() / (float) randomGenerator.max();
	send_query_delay = IAT_min + (random * (IAT_max - IAT_min));
}

Building::Building(bool, int district, std::shared_ptr<std::vector<std::string> > path, int IAT_min = 100, int IAT_max = 100,
			 	   int v_pref_min = 15, int v_pref_max = 15, int dv_pos_max = 15, int dv_neg_max = 150, std::string name = "Building"):
				AtomicModel(name), IAT_min(IAT_min), IAT_max(IAT_max),
				v_pref_min(v_pref_min), v_pref_max(v_pref_max),
				dv_pos_max(dv_pos_max), dv_neg_max(dv_neg_max), district(district)
{
	LOG_DEBUG("BUILDING: Creating building " + name);
	setState(n_tools::createObject<BuildingState>(IAT_min, IAT_max, path, name));
	getBuildingState()->setNextVPref(v_pref_min, v_pref_max);
	send_max = 1;

	// output ports
	q_sans = addOutPort("q_sans");
	q_send = addOutPort("q_send");
	exit = addOutPort("exit");
	Q_send = q_send;
	car_out = exit;

	// input ports
	q_rans = addInPort("q_rans");
	Q_rack = q_rans;
	entry = addInPort("entry");

	LOG_DEBUG("BUILDING: Done creating building " + name);
}

void Building::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	LOG_DEBUG("BUILDING: " + getName() + " - enter extTransition");
	getBuildingState()->currentTime = getBuildingState()->currentTime + m_elapsed;
	getBuildingState()->send_query_delay = getBuildingState()->send_query_delay - m_elapsed;
	getBuildingState()->send_car_delay = getBuildingState()->send_car_delay - m_elapsed;

	for (auto msg : message) {
		if(msg->getDestinationPort() == Q_rack->getFullName()) {
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - expect QueryAck");
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - from " + msg->getSourcePort());
			std::shared_ptr<QueryAck> queryAck = n_network::getMsgPayload<std::shared_ptr<QueryAck> >(msg);
			LOG_DEBUG("ROADSEGMENT: " + getName() + " - received QueryAck");
			if (getBuildingState()->send_car_id == queryAck->ID and (getBuildingState()->sent < send_max)) {
				getBuildingState()->send_car_delay = queryAck->t_until_dep;
				if (queryAck->t_until_dep < t_timestamp(20000,0)) {
					getBuildingState()->sent += 1;
					// Generate the next situation
					if (getBuildingState()->sent < send_max) {
						getBuildingState()->setSendQueryDelay(IAT_min, IAT_max);
						getBuildingState()->send_query_delay = getBuildingState()->send_query_delay + t_timestamp(1000000, 0);
					}
				}
			}
		}
	}
}

void Building::intTransition()
{
	LOG_DEBUG("BUILDING: " + getName() + " - enter intTransition");
	t_timestamp mintime = timeAdvance();
	getBuildingState()->currentTime = getBuildingState()->currentTime + mintime;
	getBuildingState()->send_query_delay = getBuildingState()->send_query_delay - mintime;
	getBuildingState()->send_car_delay = getBuildingState()->send_car_delay - mintime;

	if (getBuildingState()->send_car_delay == t_timestamp(0,0)) {
		getBuildingState()->send_car_delay = t_timestamp::infinity();
		getBuildingState()->send_car_id = getBuildingState()->send_query_id;
		getBuildingState()->setNextVPref(v_pref_min, v_pref_max);
	}
	else if (getBuildingState()->send_query_delay == t_timestamp(0,0)) {
		getBuildingState()->send_query_delay = t_timestamp::infinity();
	}
}

t_timestamp Building::timeAdvance() const
{
	LOG_DEBUG("BUILDING: " + getName() + " - enter timeAdvance");
	if (getBuildingState()->send_query_delay < getBuildingState()->send_car_delay) {
		return getBuildingState()->send_query_delay;
	}
	return getBuildingState()->send_car_delay;
}

std::vector<n_network::t_msgptr> Building::output() const
{
	LOG_DEBUG("BUILDING: " + getName() + " - enter output");
	t_timestamp mintime = timeAdvance();
	t_timestamp currentTime = getBuildingState()->currentTime + timeAdvance();

	std::cout << "(" << getBuildingState()->send_car_delay << ", " << mintime << ")";
	if (getBuildingState()->send_car_delay == mintime) {
		float v_pref = getBuildingState()->next_v_pref;
		Car car (getBuildingState()->send_car_id, 0, v_pref, dv_pos_max, dv_neg_max, currentTime);
		car.path = *getBuildingState()->path;
		return car_out->createMessages(car);
	}
	else if (getBuildingState()->send_query_delay == mintime) {
		Query query (getBuildingState()->send_query_id);
		return Q_send->createMessages(query);
	}
	return std::vector<n_network::t_msgptr>();
}

std::shared_ptr<BuildingState> Building::getBuildingState() const
{
	return std::static_pointer_cast<BuildingState>(getState());
}

}


