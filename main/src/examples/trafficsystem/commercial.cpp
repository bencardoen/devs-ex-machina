/*
 * Commercial.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "commercial.h"

namespace n_examples_traffic {

CommercialState::CommercialState(): car()
{
}

CommercialState::CommercialState(std::shared_ptr<Car> car): car(car)
{
}

CommercialState::CommercialState(const CommercialState& state): CommercialState(state.car)
{
}

std::string CommercialState::toString() const
{
	return "CommercialState";
}

Commercial::Commercial(int district, std::string name):
		Building(false, district, std::vector<std::string>(), 100, 100, 15, 15, 15, 150, name)
{
	setState(n_tools::createObject<CommercialState>());
	toCollector = addOutPort("toCollector");
}

void Commercial::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	auto msg = message.at(0);
	setState(n_tools::createObject<CommercialState>(n_network::getMsgPayload<CommercialState>(msg)));
}

void Commercial::intTransition()
{
	setState(n_tools::createObject<CommercialState>());
}

t_timestamp Commercial::timeAdvance() const
{
	if (getCommercialState()->car) {
		return t_timestamp::infinity();
	}
	else {
		return t_timestamp();
	}
}

std::vector<n_network::t_msgptr> Commercial::output() const
{
	return toCollector->createMessages(getCommercialState()->car);
}

std::shared_ptr<CommercialState> Commercial::getCommercialState() const
{
	return std::static_pointer_cast<CommercialState>(getState());
}


}


