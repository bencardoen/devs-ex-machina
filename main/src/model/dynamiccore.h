/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
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
	std::vector<t_raw_atomic> m_lastimminents;
protected:
	/**
	 * Called during Simulation Step.
	 * Parameter contains names of models transitioned in the last step,
	 * atomicmodelptrs are retrieved and stored in m_lastimminents;
	 */
	void
	signalImminent(const std::vector<t_raw_atomic>& imm)override;

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
	getLastImminents(std::vector<t_raw_atomic>& imms);
        
        void
        addModelDS(const t_atomicmodelptr& model)override;
        
        void
        removeModelDS(std::size_t id)override;
        
        /**
         * Whenever at least 1 model is altered in DS, call this
         * function to reset all models. 
         */
        void
        validateModels()override;
};

} /* namespace n_model */

#endif /* SRC_MODEL_DYNAMICCORE_H_ */
