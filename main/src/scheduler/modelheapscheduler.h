/*
 * heapscheduleradapter.h
 *
 *  Created on: Oct 19, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_MODELHEAPSCHEDULER_H_
#define SRC_SCHEDULER_MODELHEAPSCHEDULER_H_


#include <scheduler/heapscheduler.h>
#include "tools/scheduler.h"
#include "model/atomicmodel.h"
#include "tools/globallog.h"

namespace n_tools {

template<typename S>
class ModelHeapScheduler: public Scheduler<S> {
	static_assert(std::is_same<S, n_model::t_raw_atomic>::value,
		"The heap scheduler is currently only implemented for n_model::t_raw_atomic.");
};

struct ModelComparator
{
	inline
	bool operator()(n_model::t_raw_atomic a, n_model::t_raw_atomic b) const
	{
		// need to test for greater than, because std::make_heap constructs a max heap
		// and we need a min heap.
		n_network::t_timestamp aTime = a->getTimeNext();
		n_network::t_timestamp bTime = b->getTimeNext();
		return aTime > bTime;
	}
};

template<>
class ModelHeapScheduler<n_model::t_raw_atomic>: public n_tools::HeapScheduler<n_model::AtomicModel_impl, ModelComparator>
{
private:
        /**
         * Working vector for getting the imminent models
         */
        std::vector<std::size_t> m_imminentIndexes;

        typedef n_tools::HeapScheduler<n_model::AtomicModel_impl, ModelComparator> t_base;

public:
        using t_base::HeapScheduler;

	n_model::t_raw_atomic pop()
	{
		n_model::t_raw_atomic fr = front().m_ptr;
		remove(fr->getLocalID());
		return fr;
	}

	void
	unschedule_until(std::vector<n_model::t_raw_atomic> &container, const n_network::t_timestamp& mark)
	{
		std::size_t heapsize = size();
		m_imminentIndexes.push_back(0);

		while(m_imminentIndexes.size()){
			std::size_t i = m_imminentIndexes.back();
			m_imminentIndexes.pop_back();
			if(i >= heapsize)
				continue;
			const n_model::t_raw_atomic ptr = heapAt(i);
			const n_network::t_timestamp::t_time itemTime = ptr->getTimeNext().getTime();
			assert(itemTime >= mark && "An item may not have a smaller next time than the calculated next time of the core.");
			if(itemTime == mark){
				container.push_back(ptr);
				ptr->nextType() |= n_model::INT;
				m_imminentIndexes.push_back(i*2u+2u);
				m_imminentIndexes.push_back(i*2u+1u);
			}
		}
		assert(m_imminentIndexes.empty() && "no longer imminent indexes to check.");
	}

	bool
	contains(n_model::t_raw_atomic elem) const
	{ return (elem->getLocalID() < size() && elem == at(elem->getLocalID()).m_ptr); }

	void
	printScheduler()
	{
#ifdef LOGGING
		LOG_DEBUG(" Scheduler state:");
		LOG_DEBUG("    heap models size: ", size());
		for(std::size_t i = 0; i < size(); ++i){
			auto m = heapAt(i);
			LOG_DEBUG("    ", i,"\t", operator[](m->getLocalID()), "\t:  model: ", m->getName(), ", time: ", m->getTimeNext());
		}
#endif
	}

	void
	testInvariant()
	{ assert(isHeap() && "Heap scheduler heap property is violated"); }

	using t_base::update;

	void
        update(n_model::t_raw_atomic elem)
        { update(elem->getLocalID()); }
};

} /* namespace n_tools */



#endif /* SRC_SCHEDULER_MODELHEAPSCHEDULER_H_ */
