/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_coupled {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;


class PolicemanMode
{
public:
	std::string m_value;
	PolicemanMode(std::string state): m_value(state)
	{}
};


} /* namespace n_examples */

template<>
struct ToString<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return s.m_value;
	}
};
template<>
struct ToXML<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return "<policeman activity =\"" + s.m_value + "\"/>";
	}
};
template<>
struct ToJSON<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode& s){
		return "{ \"activity\": \"" + s.m_value + "\" }";
	}
};
template<>
struct ToCell<n_examples_coupled::PolicemanMode>
{
	static std::string exec(const n_examples_coupled::PolicemanMode&){
		return "";
	}
};

namespace n_examples_coupled {

class Policeman: public AtomicModel<PolicemanMode>
{
public:
	Policeman() = delete;
	Policeman(std::string, std::size_t priority = 0);
	~Policeman()
	{
	}

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
	t_timestamp lookAhead() const override;
};

}

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_POLICEMANC_H_ */
