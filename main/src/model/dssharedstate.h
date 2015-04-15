/*
 * dssharedstate.h
 *
 *  Created on: Apr 10, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_MODEL_DSSHAREDSTATE_H_
#define SRC_MODEL_DSSHAREDSTATE_H_

#include <map>
#include <string>

namespace n_model {

/**
 * @brief Container for the shared state used by dynamic structured DEVS
 */
struct DSScharedState
{
	std::multimap<std::string, std::string> values;
};

} /* namespace n_model */

#endif /* SRC_MODEL_DSSHAREDSTATE_H_ */
