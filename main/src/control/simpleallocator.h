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

/*
 * Simple, dumb allocator
 * Spreads models evenly
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
	size_t allocate(const n_model::t_atomicmodelptr&)
	{
		int i = m_i;
		m_i = (m_i + 1) % m_cores;
		return i;
	}
};

} /* namespace n_control */


#endif /* SRC_CONTROL_SIMPLEALLOCATOR_H_ */
