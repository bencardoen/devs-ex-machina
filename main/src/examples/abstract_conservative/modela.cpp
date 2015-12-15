/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#include "examples/abstract_conservative/modela.h"

namespace n_examples_abstract_c {

ModelA::ModelA(std::string name, std::size_t priority)
	: AtomicModel<int>(name, priority)
{
	m_elapsed = 0;
	this->addOutPort("A");
}

ModelA::~ModelA()
{
}

void ModelA::extTransition(const std::vector<n_network::t_msgptr> & )
{
	// No external transitions available
}

void ModelA::intTransition()
{
	int& st = state();
        LOG_DEBUG("MODEL :: transitioning from : " , st);
	if (st < 7)
		++st;
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp ModelA::timeAdvance() const
{
	const int& st = state();
	if (st == 7)
		return t_timestamp::infinity();
	return t_timestamp(10);
}

void ModelA::output(std::vector<n_network::t_msgptr>& msgs) const
{
	const int& st = state();
	if ((st == 2) || (st == 5)) {
		this->getPort("A")->createMessages("start", msgs);
	}
}

t_timestamp ModelA::lookAhead() const
{
	return t_timestamp::infinity();
}

} /* namespace n_examples_abstract_c */
