/*
 * Configuration.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONFIGURATION_H_
#define SRC_CONTROL_CONFIGURATION_H_

#include "../network/timestamp.h"

namespace n_control {

enum Formalism {};

class Configuration
{
public:
	Configuration(Formalism formalism);
	virtual ~Configuration();

	Formalism getFormalism();

	// TODO void setScheduler...
	// TODO void setTracer...
	// TODO void setAllocator...
	void setCheckpointing(std::string filename, n_network::Time interval) = delete;

private:
	Formalism m_formalism;
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONFIGURATION_H_ */
