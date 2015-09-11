/*
 * Deadlock Model
 */

#ifndef SRC_EXAMPLES_DEADLOCK_H_
#define SRC_EXAMPLES_DEADLOCK_H_

#include "model/atomicmodel.h"
#include "model/state.h"

namespace n_examples_deadlock {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class Ping: public n_model::AtomicModel
{
public:
	Ping(std::string name, std::size_t priority = 0);
	virtual ~Ping();

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
	t_timestamp lookAhead() const override;


	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

} /* namespace n_examples_abstract_c */

#endif /* SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELA_H_ */
