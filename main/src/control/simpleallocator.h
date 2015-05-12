/*
 * simpleallocator.h
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_SIMPLEALLOCATOR_H_
#define SRC_CONTROL_SIMPLEALLOCATOR_H_

#include "allocator.h"

namespace n_control {

/**
 * @brief Simple, dumb allocator that tries to spread models evenly, but gets overruled by the models' configuration
 */
class SimpleAllocator: public Allocator
{
private:
	size_t m_i;
	size_t m_cores;

public:
	SimpleAllocator(size_t c)
		: m_i(0), m_cores(c)
	{
	}

	virtual ~SimpleAllocator()
	{
	}

	/**
	 * @brief Returns on which Core to place a Model
	 * @param model the AtomicModel to be allocated
	 */
	size_t allocate(n_model::t_atomicmodelptr& m)
	{
		int corenum = m->getCorenumber();
		if(corenum == -1) {	// The user didn't specify a particular core, so we pick one ourselves
			int i = m_i;
			m_i = (m_i + 1) % m_cores;
			m->setCorenumber(i); // We let the model know which core we decided to place it on
			return i;
		}
		else return corenum;	// The user already specified a core for this model, so we better pick that one!
	}
};

} /* namespace n_control */

#endif /* SRC_CONTROL_SIMPLEALLOCATOR_H_ */
