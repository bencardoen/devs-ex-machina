/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve
 */

#ifndef SRC_MODEL_DSSHAREDSTATE_H_
#define SRC_MODEL_DSSHAREDSTATE_H_

#include <map>
#include <string>

namespace n_model {

/**
 * @brief Container for the shared state used by dynamic structured DEVS
 */
struct DSSharedState
{
	std::multimap<std::string, std::string> values;
};

} /* namespace n_model */

#endif /* SRC_MODEL_DSSHAREDSTATE_H_ */
