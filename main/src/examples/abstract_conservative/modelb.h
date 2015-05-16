/*
 * modelb.h
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_
#define SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_

#include "atomicmodel.h"
#include "state.h"

namespace n_examples_abstract_c {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class ModelB: public n_model::AtomicModel
{
public:
	ModelB(std::string name, std::size_t priority = 0);
	virtual ~ModelB();

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	std::vector<n_network::t_msgptr> output() const override;
	t_timestamp lookAhead() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel::setState;
	t_stateptr setState(std::string);
};

} /* namespace n_examples_abstract_c */



#endif /* SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_ */
