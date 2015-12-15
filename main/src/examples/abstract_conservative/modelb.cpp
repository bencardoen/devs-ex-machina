/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Tim Tuijn
 */

#include <examples/abstract_conservative/modelb.h>

namespace n_examples_abstract_c {

ModelB::ModelB(std::string name, std::size_t priority)
	: AtomicModel<int>(name, priority)
{
	m_elapsed = 0;
	this->addInPort("B");
}

ModelB::~ModelB()
{
}

void ModelB::extTransition(const std::vector<n_network::t_msgptr> & message)
{
	int& st = state();
	if ((st == 2) && (message.at(0)->getPayload() == "start")){
                LOG_DEBUG("MODEL :: transitioning from : " , st);
		st = 3;
        }
	else if ((st == 5) && (message.at(0)->getPayload() == "start")){
                LOG_DEBUG("MODEL :: transitioning from : " , st);
		st = 6;
        }
}

void ModelB::intTransition()
{
	int& st = state();
        LOG_DEBUG("MODEL :: transitioning from : " , st);
	if (st < 7)
		++st;
	else
		assert(false); // You shouldn't come here...
	return;
}

t_timestamp ModelB::timeAdvance() const
{
	const int& st = state();
	if (st == 7 || (st == 2) || (st == 5))
		return t_timestamp::infinity();
	return t_timestamp(10);
}

void ModelB::output(std::vector<n_network::t_msgptr>&) const
{
	//nothing to do here
}

t_timestamp ModelB::lookAhead() const
{
	const int& st = state();
	if ((st == 0) || (st == 3)){
		return t_timestamp(30);
	}
	else if ((st == 1) || (st == 4)){
                assert(false);
		return t_timestamp(20);
	}
	else if ((st == 2) || (st == 5)){
                assert(false);
		return t_timestamp(10);
	}

	return t_timestamp::infinity();
}

} /* namespace n_examples_abstract_c */


