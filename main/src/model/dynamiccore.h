/*
 * DynamicCore.h
 *
 *  Created on: 7 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_DYNAMICCORE_H_
#define SRC_MODEL_DYNAMICCORE_H_

#include "core.h"

namespace n_model {

/**
 * Core implementation for dynamic structured devs.
 */
class DynamicCore: public Core
{
private:
	std::vector<t_atomicmodelptr>	m_lastimminents;
protected:
	/**
	 * Called during Simulation Step.
	 * Parameters contains names of models transitioned in the last step,
	 * atomicmodelptrs are retrieved and stored in m_lastimminents;
	 */
	void
	signalImminent(const std::set<std::string>& immnames);

public:
	DynamicCore();
	virtual ~DynamicCore();

	/**
	 * Called in case of Dynamic structured Devs.
	 * Stores imminent models into parameter.
	 * @attention : noop in superclass
	 */
	virtual
	void
	getLastImminents(std::vector<t_atomicmodelptr>& imms);
};

} /* namespace n_model */

#endif /* SRC_MODEL_DYNAMICCORE_H_ */
