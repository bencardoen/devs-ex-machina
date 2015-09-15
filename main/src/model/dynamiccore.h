/*
 * DynamicCore.h
 *
 *  Created on: 7 Apr 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_MODEL_DYNAMICCORE_H_
#define SRC_MODEL_DYNAMICCORE_H_

#include "model/core.h"

namespace n_model {

/**
 * Core implementation for dynamic structured devs.
 */
class DynamicCore: public Core
{
private:
	std::vector<t_atomicmodelptr> m_lastimminents;
protected:
	/**
	 * Called during Simulation Step.
	 * Parameter contains names of models transitioned in the last step,
	 * atomicmodelptrs are retrieved and stored in m_lastimminents;
	 */
	void
	signalImminent(const std::vector<t_atomicmodelptr>& imm)override;

public:
	DynamicCore();
	virtual ~DynamicCore();

	/**
	 * Called in case of Dynamic structured Devs.
	 * Stores imminent models into parameter.
	 * Controller calls this to check which models have transitioned (int||conf).
	 */
	virtual
	void
	getLastImminents(std::vector<t_atomicmodelptr>& imms);
        
        void
        addModelDS(const t_atomicmodelptr& model)override;
        
        void
        removeModelDS(const std::string& name)override;
        
        /**
         * Whenever at least 1 model is altered in DS, call this
         * function to reset all models. 
         */
        void
        validateModels()override;
};

} /* namespace n_model */

#endif /* SRC_MODEL_DYNAMICCORE_H_ */
