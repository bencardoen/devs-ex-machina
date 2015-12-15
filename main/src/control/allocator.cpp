/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Matthijs Van Os, Stijn Manhaeve. 
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
