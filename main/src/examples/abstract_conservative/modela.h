/*
 * modela.h
 *
 *  Created on: May 16, 2015
 *      Author: tim
 */

#ifndef SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELA_H_
#define SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELA_H_

#include "model/atomicmodel.h"
#include "model/state.h"

namespace n_examples_abstract_c {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel_impl;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class ModelA: public n_model::AtomicModel_impl
{
public:
	ModelA(std::string name, std::size_t priority = 0);
	virtual ~ModelA();

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
	t_timestamp lookAhead() const override;

	/*
	 * The following function has been created to easily
	 * create states using a string
	 */
	using AtomicModel_impl::setState;
	t_stateptr setState(std::string);
};

} /* namespace n_examples_abstract_c */

#endif /* SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELA_H_ */
