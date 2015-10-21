/*
 * modelscheduler.h
 *
 *  Created on: Oct 21, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_MODELSCHEDULER_H_
#define SRC_SCHEDULER_MODELSCHEDULER_H_

#include "scheduler/modelheapscheduler.h"
#include "model/atomicmodel.h"

namespace n_tools {


//template<>
//struct ModelScheduler{};
//
//struct RawAtomicComparator
//{
//	inline static
//	bool comp(n_model::t_raw_atomic a, n_model::t_raw_atomic b) const
//	{
//		// need to test for greater than, because std::make_heap constructs a max heap
//		// and we need a min heap.
//		n_network::t_timestamp aTime = a->getTimeNext();
//		n_network::t_timestamp bTime = b->getTimeNext();
//		return aTime > bTime;
//	}
//};
//
typedef ModelHeapScheduler<n_model::t_raw_atomic> t_ModelHeap;

} /* namespace n_tools */




#endif /* SRC_SCHEDULER_MODELSCHEDULER_H_ */
