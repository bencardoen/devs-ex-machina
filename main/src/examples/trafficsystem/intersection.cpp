/*
 * Intersection.cpp
 *
 *  Created on: May 22, 2015
 *      Author: pieter
 */

#include "intersection.h"

namespace n_examples_traffic {

IntersectionState::IntersectionState(int switch_signal): State()
{

}

IntersectionState::IntersectionState(const IntersectionState&)
{

}

std::string IntersectionState::toString()
{

}

Intersection::Intersection(int district, std::string name, int switch_signal):
		AtomicModel(name), district(district), switch_signal_delay(switch_signal)
{

}

void Intersection::extTransition(const std::vector<n_network::t_msgptr> & message)
{

}

void Intersection::intTransition()
{

}

t_timestamp Intersection::timeAdvance() const
{

}

std::vector<n_network::t_msgptr> Intersection::output() const
{

}


std::shared_ptr<IntersectionState> Intersection::getIntersectionState() const
{

}

}


