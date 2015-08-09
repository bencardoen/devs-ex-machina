/*
 * simpleallocator.h
 *
 *  Created on: 25 Apr 2015
 *      Author: matthijs
 */

#ifndef SRC_CONTROL_SIMPLEALLOCATOR_H_
#define SRC_CONTROL_SIMPLEALLOCATOR_H_

#include "control/allocator.h"

namespace n_control {

/**
 * @brief Simple, dumb allocator that tries to spread models evenly, but gets overruled by the models' configuration by default
 */
class SimpleAllocator: public Allocator
{
private:
	size_t m_i;
	size_t m_cores;
	bool m_allowUserOverride;

public:
	SimpleAllocator(size_t c, bool allowOverride = true)
		: m_i(0), m_cores(c), m_allowUserOverride(allowOverride)
	{
	}

	virtual ~SimpleAllocator()
	{
	}

	/**
	 * @brief Returns on which Core to place a Model
	 * @param model the AtomicModel to be allocated
	 */
	size_t allocate(const n_model::t_atomicmodelptr& m)override
	{
		int corenum = m->getCorenumber();
		if(!m_allowUserOverride || corenum == -1) {	// If user override is not allowed, or if it is but the
			corenum = m_i;				//  user didn't specify a particular core, we pick a
			m_i = (m_i + 1) % m_cores;		//  destination ourselves
		} else {
			corenum %= m_cores;	// Make absolutely sure the model always ends up on a real core
						//  e.g. if the user lowered the amount of cores on a loaded simulation
		}
		m->setCorenumber(corenum);
		return corenum;
	}
};

} /* namespace n_control */

#endif /* SRC_CONTROL_SIMPLEALLOCATOR_H_ */
