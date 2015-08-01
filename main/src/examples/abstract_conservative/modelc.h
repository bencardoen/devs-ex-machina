/*
 * modelc.h
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELC_H_
#define SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELC_H_

#include "examples/abstract_conservative/modela.h"
#include "examples/abstract_conservative/modelb.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_abstract_c {

using n_model::CoupledModel;
using n_network::t_msgptr;

/*
 * This coupled model is written to test the conservative parallel simulation
 *
 *  ------------ Coupled Model ------------
 * |                                       |
 * |    ModelA        -->      modelB      |
 * |                                       |
 *  =======================================
 *
 * Internal transitions occur after 10 time units, except for state
 * 2 and 5 of model B. These need an external transition
 *
 *  A:					B:
 *
 *  time	: state			time	: state
 *   0		: 0			0 	: 0
 *  		| intTransition 		| intTransition
 *  10		: 1			10	: 1
 *  		| intTransition			| intTransition
 *  20		: 2			20	: 2
 *  		| intTransition			| extTransition ('start' message from A at time 30)
 *  30		: 3			30	: 3
 *  		| intTransition			| intTransition
 *  40		: 4			40	: 4
 *  		| intTransition			| intTransition
 *  50		: 5			50	: 5
 *  		| intTransition			| extTransition ('start' message from A at time 60)
 *  60		: 6			60	: 6
 *  		| intTransition			| intTransition
 *  70		: 7			70	: 7
 *
 */

class ModelC: public CoupledModel
{
public:
	ModelC() = delete;
	ModelC(std::string name);
	~ModelC() {}
};

}

#endif /* SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELC_H_ */
