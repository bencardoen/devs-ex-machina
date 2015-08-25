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
	bool m_allowUserOverride;

public:
	SimpleAllocator(size_t c, bool allowOverride = true)
		: m_i(0), m_allowUserOverride(allowOverride)
	{
		setCoreAmount(c);
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
			m_i = (m_i + 1) % coreAmount();		//  destination ourselves
		} else {
			corenum %= coreAmount();	// Make absolutely sure the model always ends up on a real core
						//  e.g. if the user lowered the amount of cores on a loaded simulation
		}
		m->setCorenumber(corenum);
		return corenum;
	}

	void allocateAll(const std::vector<n_model::t_atomicmodelptr>& atomics)override{
		LOG_INFO("Allocator processing ", atomics.size(), " atomics over ", coreAmount(), " cores");
		for(const auto& atomicp : atomics){
			const int stalenr = atomicp->getCorenumber();
			int corenr = 0;
			if(!m_allowUserOverride || stalenr == -1) {	// If user override is not allowed, or if it is but the
				corenr = m_i;				//  user didn't specify a particular core, we pick a
				m_i = (m_i + 1) % coreAmount();		//  destination ourselves
			} else {
				corenr %= coreAmount();	// Make absolutely sure the model always ends up on a real core
							//  e.g. if the user lowered the amount of cores on a loaded simulation
			}
			atomicp->setCorenumber(corenr);
			LOG_INFO("Allocator assigned ", atomicp->getName(), " from ", stalenr, " to ", corenr);
		}
	}
};

} /* namespace n_control */

#endif /* SRC_CONTROL_SIMPLEALLOCATOR_H_ */
