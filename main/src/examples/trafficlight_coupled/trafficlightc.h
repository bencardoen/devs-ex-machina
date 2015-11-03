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
};

}

#endif /* SRC_EXAMPLES_TRAFFICLIGHTC_H_ */
