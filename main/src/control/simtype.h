/*
 * simtype.h
 *
 *  Created on: Aug 11, 2015
 *      Author: DEVS Ex Machina
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
