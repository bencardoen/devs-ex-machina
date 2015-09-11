/*
 * @author Ben
 */

#ifndef SRC_EXAMPLES_DEADLOCK_PINGSET_
#define SRC_EXAMPLES_DEADLOCK_PINGSET_

#include "examples/deadlock/ping.h"
#include "model/atomicmodel.h"
#include "model/coupledmodel.h"
#include "model/state.h"
#include <assert.h>

namespace n_examples_deadlock {

using n_model::CoupledModel;
using n_network::t_msgptr;


class Pingset: public CoupledModel
{
public:
	Pingset() = delete;
	Pingset(std::string name);
	~Pingset() {}
};

}

#endif
