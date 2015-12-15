/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manheave
 */

#ifndef SRC_CONTROL_SIMTYPE_H_
#define SRC_CONTROL_SIMTYPE_H_

namespace n_control
{

/**
 * @brief Enumerator for the simulation type.
 */
enum SimType{
	CLASSIC = 1,		///non-parallel simulation.
	OPTIMISTIC = 2,		///optimistic parallel simulation
	CONSERVATIVE = 4,	///conservative parallel simulation
	DYNAMIC = 8		///dynamic structured non-parallel simulation
};


/**
 * @brief Tests if the simulation type is parallel.
 * @param s A SimType enum value.
 * Parallel simulation types are SimType::OPTIMISTIC and SimType::CONSERVATIVE.
 */
inline bool isParallel(SimType s){
	return (s & (OPTIMISTIC | CONSERVATIVE));
}

} /* namespace n_control */

#endif /* SRC_CONTROL_SIMTYPE_H_ */
