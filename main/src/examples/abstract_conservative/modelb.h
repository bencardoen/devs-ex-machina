/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#ifndef SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_
#define SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_

#include "model/atomicmodel.h"
#include "model/state.h"

namespace n_examples_abstract_c {

using n_model::State;
using n_model::t_stateptr;
using n_model::AtomicModel_impl;
using n_network::t_msgptr;
using n_network::t_timestamp;
using n_model::t_atomicmodelptr;

class ModelB: public n_model::AtomicModel<int>
{
public:
	ModelB(std::string name, std::size_t priority = 0);
	virtual ~ModelB();

	void extTransition(const std::vector<n_network::t_msgptr> & message) override;
	void intTransition() override;
	t_timestamp timeAdvance() const override;
	void output(std::vector<n_network::t_msgptr>& msgs) const override;
	t_timestamp lookAhead() const override;
};

} /* namespace n_examples_abstract_c */



#endif /* SRC_EXAMPLES_ABSTRACT_CONSERVATIVE_MODELB_H_ */
