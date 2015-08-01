/*
 * constants.cpp
 *
 *  Created on: May 2, 2015
 *      Author: Stijn Manhaeve - Devs Ex Machina
 */

#include "examples/forestfire/constants.h"
#include <cassert>

namespace n_examples{

FirePhase getNext(FirePhase phase, double temp)
{
	if (temp > T_IGNITE || (temp > T_BURNED && phase == FirePhase::BURNING))
		return FirePhase::BURNING;
	else if (temp < T_BURNED && phase == FirePhase::BURNING)
		return FirePhase::BURNED;
	else
		return FirePhase::UNBURNED;
}

std::string to_string(FirePhase phase)
{
	switch (phase) {
	case FirePhase::INACTIVE:
		return "inactive";
	case FirePhase::UNBURNED:
		return "unburned";
	case FirePhase::BURNING:
		return "burning";
	case FirePhase::BURNED:
		return "burned";
	default:
		assert(false && "to_string(FirePhase) somehow managed to create a non-existing phase.");
		return "";
	}
}

} /* namespace n_examples */

