/*
 * Configuration.h
 *
 *  Created on: 11 Mar 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_CONFIGURATION_H_
#define SRC_CONTROL_CONFIGURATION_H_

namespace n_control {

enum e_formalism {};

class Configuration
{
public:
	Configuration();
	virtual ~Configuration();

	e_formalism getFormalism();
};

} /* namespace n_control */

#endif /* SRC_CONTROL_CONFIGURATION_H_ */
