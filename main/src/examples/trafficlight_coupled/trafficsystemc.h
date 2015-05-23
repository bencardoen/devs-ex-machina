/*
 * trafficsystemc.h
 *
 *  Created on: Mar 29, 2015
 *      Author: tim
 */

#ifndef MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_
#define MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_

#include "atomicmodel.h"
#include "coupledmodel.h"
#include "state.h"
#include <assert.h>
#include "policemanc.h"
#include "trafficlightc.h"
#include "cereal/archives/binary.hpp"

namespace n_examples_coupled {

using n_model::CoupledModel;
using n_network::t_msgptr;

class TrafficSystem: public CoupledModel
{
public:
	TrafficSystem() = delete;
	TrafficSystem(std::string name);
	~TrafficSystem() {}

	/**
	 * Serialize this object to the given archive
	 *
	 * @param archive A container for the desired output stream
	 */
	void serialize(n_serialization::t_oarchive& archive);

	/**
	 * Unserialize this object to the given archive
	 *
	 * @param archive A container for the desired input stream
	 */
	void serialize(n_serialization::t_iarchive& archive);

	/**
	 * Helper function for unserializing smart pointers to an object of this class.
	 *
	 * @param archive A container for the desired input stream
	 * @param construct A helper struct for constructing the original object
	 */
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficSystem>& construct);
};

}

CEREAL_REGISTER_TYPE(n_examples_coupled::TrafficSystem)

#endif /* MAIN_SRC_EXAMPLES_TRAFFICLIGHT_COUPLED_TRAFFICSYSTEMC_H_ */
