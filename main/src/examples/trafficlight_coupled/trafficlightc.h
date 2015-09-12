/*
 * trafficlightc.h
 *
 *  Created on: Mar 19, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_TRAFFICLIGHTC_H_
#define SRC_EXAMPLES_TRAFFICLIGHTC_H_

#include "model/atomicmodel.h"
#include "model/state.h"
#include "examples/trafficlight_classic/trafficlight.h"
#include <assert.h>

namespace n_examples_coupled {

using n_network::t_msgptr;
using n_model::AtomicModel;
using n_model::State;
using n_model::t_stateptr;
using n_model::t_modelptr;
using n_network::t_timestamp;

typedef n_examples::TrafficLightMode TrafficLightMode;

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
	t_timestamp lookAhead() const override;

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
	static void load_and_construct(n_serialization::t_iarchive& archive, cereal::construct<TrafficLight>& construct);
};

}

CEREAL_REGISTER_TYPE(n_examples_coupled::TrafficLight)

#endif /* SRC_EXAMPLES_TRAFFICLIGHTC_H_ */
