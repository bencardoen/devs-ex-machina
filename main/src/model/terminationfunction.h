/*
 * terminationfunction.h
 *
 *  Created on: 28 Mar 2015
 *      Author: Ben Cardoen
 */

#include "atomicmodel.h"

#ifndef SRC_MODEL_TERMINATIONFUNCTION_H_
#define SRC_MODEL_TERMINATIONFUNCTION_H_

namespace n_model{

/**
 * Evaluation functor.
 * Subclass should override evaluateModel to provide user defined termination behaviour.
 */
class TerminationFunctor{

	TerminationFunctor() = default;
	virtual ~TerminationFunctor(){;}

	bool
	operator()(const t_atomicmodelptr& model)const{
		return evaluateModel(model);
	}

	/**
	 * Evaluate model, test if a user specified condition is met to terminate simulation.
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
