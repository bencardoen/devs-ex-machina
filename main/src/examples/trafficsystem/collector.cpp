/*
 * Collector.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "examples/trafficsystem/collector.h"
#include <sstream>

namespace n_examples_traffic {

CollectorState::CollectorState::CollectorState(): cars()
{
}

CollectorState::CollectorState(const CollectorState& state): CollectorState()
{
	cars = state.cars;
}

std::string CollectorState::toString()
{
	std::stringstream ss;
	ss << "All cars collected:";
	for (auto car : cars) {
		ss << "\n\t\t\t" << car;
	}
	return ss.str();
}

Collector::Collector(): AtomicModel("Collector")
{
	setState(n_tools::createObject<CollectorState>());
	car_in = addInPort("car_in");
	district = 0;
}

void Collector::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	t_msgptr msg = message.at(0);
	if (msg->getDestinationPort() == car_in->getFullName()) {
		std::shared_ptr<Car> car = n_network::getMsgPayload<std::shared_ptr<Car> >(msg);
		getCollectorState()->cars.push_back(car);
	}
}

std::shared_ptr<CollectorState> Collector::getCollectorState() const
{
	return std::static_pointer_cast<CollectorState>(getState());
}

void Collector::intTransition()
{
	return;
}

t_timestamp Collector::timeAdvance() const
{
	return t_timestamp::infinity();
}

std::vector<n_network::t_msgptr> Collector::output() const
{
	return std::vector<n_network::t_msgptr> ();
}

}
