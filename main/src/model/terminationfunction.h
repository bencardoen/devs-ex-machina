/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#include "model/atomicmodel.h"

#ifndef SRC_MODEL_TERMINATIONFUNCTION_H_
#define SRC_MODEL_TERMINATIONFUNCTION_H_

namespace n_model{

/**
 * Evaluation functor.
 * Subclass should override evaluateModel to provide user defined termination behaviour.
 */
class TerminationFunctor{
public:
	TerminationFunctor() = default;
	virtual ~TerminationFunctor(){;}

	/**
	 * Operator calls virtual function evaluateModel for each model in the simkernel.
	 */
	bool
	operator()(const t_atomicmodelptr& model)const{
		return evaluateModel(model);
	}

	/**
	 * Evaluate model, test if a user specified condition is met to terminate simulation.
	 * Default implementation returns false always
	 * @return false to continue simulation, true to signal termination.
	 */
	virtual
	bool
	evaluateModel(const t_atomicmodelptr&)const{
		return false;
	}
};

typedef std::shared_ptr<TerminationFunctor> t_terminationfunctor;

}//E namespace



#endif /* SRC_MODEL_TERMINATIONFUNCTION_H_ */
