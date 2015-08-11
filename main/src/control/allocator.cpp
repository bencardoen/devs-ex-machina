/*
 * Allocator.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: DEVS Ex Machina
 */

#include "control/allocator.h"

namespace n_control {

Allocator::Allocator()
	: m_numcores(1), m_simtype(SimType::CLASSIC)
{

}

void Allocator::setCoreAmount(std::size_t amount)
{
	assert(amount > 0 && "Can't set the amount of simulation cores to 0.");
	m_numcores = amount;
}

void Allocator::setSimType(SimType type)
{
	assert(((type & SimType::CLASSIC)
		^ (type & SimType::CONSERVATIVE)
		^ (type & SimType::DYNAMIC)
		^ (type & SimType::OPTIMISTIC)) && "Simulation type is not an enum value of SimType.");
	m_simtype = type;
}

Allocator::~Allocator()
{

}

} /* namespace n_control */
