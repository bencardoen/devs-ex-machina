/*
 * constants.h
 *
 *  Created on: May 2, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#ifndef SRC_EXAMPLES_FORESTFIRE_CONSTANTS_H_
#define SRC_EXAMPLES_FORESTFIRE_CONSTANTS_H_

#include <string>

namespace n_examples {


enum class FirePhase {
	INACTIVE, UNBURNED, BURNING, BURNED
};

constexpr double T_AMBIENT = 27.0;
constexpr double T_IGNITE = 300.0;
constexpr double T_GENERATE = 500.0;
constexpr double T_BURNED = 60.0;
constexpr double TIMESTEP = 0.01;
constexpr std::size_t RADIUS = 1;
constexpr double TMP_DIFF = 1.0;

/**
 * @brief Calculates the next phase based on the current phase and temperature
 */
FirePhase getNext(FirePhase phase, double temp);
/**
 * @brief Converts a phase to a string
 */
std::string to_string(FirePhase phase);

} /* namespace n_examples */



#endif /* SRC_EXAMPLES_FORESTFIRE_CONSTANTS_H_ */
