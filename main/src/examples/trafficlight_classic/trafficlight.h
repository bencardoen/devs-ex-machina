/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_EXAMPLES_TRAFFICLIGHT_H_
#define SRC_EXAMPLES_TRAFFICLIGHT_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples {

using namespace n_model;
using n_network::t_msgptr;

class TrafficLightMode
{
public:
	std::string m_value;
	TrafficLightMode(std::string state = "red");
};


} /* namespace n_examples */

template<>
struct ToString<n_examples::TrafficLightMode>
{
	static std::string exec(const n_examples::TrafficLightMode& s){
		return s.m_value;
	}
};
template<>
struct ToXML<n_examples::TrafficLightMode>
{
	static std::string exec(const n_examples::TrafficLightMode& s){
		return "<state color =\"" + s.m_value + "\"/>";
	}
};
template<>
struct ToJSON<n_examples::TrafficLightMode>
{
	static std::string exec(const n_examples::TrafficLightMode& s){
		return "{ \"state\": \"" + s.m_value + "\" }";
	}
};
template<>
struct ToCell<n_examples::TrafficLightMode>
{
	static std::string exec(const n_examples::TrafficLightMode&){
		return "";
	}
};

namespace n_examples {

class TrafficLight: public AtomicModel<TrafficLightMode>
{
public:
	TrafficLight() = delete;
	TrafficLight(std::string, std::size_t priority = 0);
	~TrafficLight() {}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
};

}

#endif /* SRC_EXAMPLES_TRAFFICLIGHT_H_ */
