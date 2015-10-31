/*
 * heapscheduleradapter.h
 *
 *  Created on: Oct 19, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_MODELHEAPSCHEDULER_H_
#define SRC_SCHEDULER_MODELHEAPSCHEDULER_H_


#include <scheduler/heapscheduler.h>
#include <scheduler/scheduler.h>
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

/**
 * @brief A model scheduler based on the Heap data structure
 *
 * Items are kept in a min heap, where the order is based on the next internal transition time.
 */
template<>
class ModelHeapScheduler<n_model::t_raw_atomic>: public n_tools::HeapScheduler<n_model::AtomicModel_impl, ModelComparator>
{
private:
        /**
         * Working vector for getting the imminent models
         */
        mutable std::vector<std::size_t> m_imminentIndexes;

        typedef n_tools::HeapScheduler<n_model::AtomicModel_impl, ModelComparator> t_base;

        bool m_updateSingle;

public:
	/**
	 * Default constructor. Constructs an empty model heap scheduler.
	 */
        ModelHeapScheduler():
		t_base(), m_updateSingle(true)
	{ }

	/**
	 * Constructs a new, empty heap scheduler
	 * @param size	Reserves this amount of size, so that subsequent pushes up until that size
	 * 		do not result in the heap becoming dirty.
	 */
        ModelHeapScheduler(std::size_t size):
		t_base(size), m_updateSingle(true)
	{ }

        /**
         * @brief Finds all items with a next internal transition time smaller than or equal to some timestamp.
         * @param container The found items will be collected in this container
         * @param mark All items with next internal transition time less than or equal to this timestamp will be collected.
         *
         * Complexity: O(k) where k is the amount of found items. Therefore, at most O(N).
         */
	void
	findUntil(std::vector<n_model::t_raw_atomic> &container, const n_network::t_timestamp& mark) const
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
			if(itemTime == mark.getTime()){
				container.push_back(ptr);
				ptr->markInternal();
				m_imminentIndexes.push_back(i*2u+2u);
				m_imminentIndexes.push_back(i*2u+1u);
			}
		}
		assert(m_imminentIndexes.empty() && "no longer imminent indexes to check.");
	}

	/**
	 * @brief Returns the next internal transition time of the min element, or infinity if the scheduler is empty.
	 */
	n_network::t_timestamp
	topTime() const
	{ return size()? front()->getTimeNext(): n_network::t_timestamp::infinity(); }

	/**
	 * @brief Returns whether or not this item is present in the scheduler.
	 *
	 * Complexity: O(1)
	 */
	bool
	contains(n_model::t_raw_atomic elem) const
	{ return (elem->getLocalID() < size() && elem == at(elem->getLocalID()).m_ptr); }

	/**
	 * @brief prints a textural representation of the internals of the scheduler to the global log file.
	 * @note If logging is disabled, this method is reduced to a no op.
	 * @param vals... Any value(s) passed as arguments will be put in front of every logged line.
	 * 		  This may be used to differentiate between multiple instances of the scheduler in the same log file.
	 */
	template<typename... T>
	void
	printScheduler(T... vals)
	{
#if LOGGING
		LOG_DEBUG(vals..., " Scheduler state:");
		LOG_DEBUG(vals..., "    heap models size: ", size());
		for(std::size_t i = 0; i < size(); ++i){
			auto m = heapAt(i);
			LOG_DEBUG(vals..., "    ", i,"\t", operator[](m->getLocalID()), "\t:  model: ", m->getName(), ", time: ", m->getTimeNext());
		}
#endif
	}

	/**
	 * @brief Tests if the contents of the scheduler are valid.
	 * @warning If debugging is turned off, this method is reduced to a no op and nothing is tested.
	 * @note The test is implemented as an assertion and will therefore quit the application if it fails.
	 * @see isHeap
	 */
	void
	testInvariant()
	{ assert(isHeap() && "Heap scheduler heap property is violated"); }

	inline
	std::size_t indexSize() const
	{ return t_base::size(); }

	using t_base::update;	//keeps the void update(std::size_t) overload available.
	using t_base::updateAll;
	using t_base::clear;

	/**
	 * @brief Updates a single item.
	 * @param elem An item in this scheduler.
	 * @precondition The item is already part of this scheduler.
	 * @precondition The item to be updated is the only item that may violate the heap constraint.
	 *
	 * complexity: O(log(N)) with N the amount of items in the scheduler.
	 * @see push_back
	 * @see updateAll
	 */
	void
        update(n_model::t_raw_atomic elem)
        {
		assert(contains(elem) && "Can't update an item that the scheduler doesn't have.");
		update(elem->getLocalID());
        }

	/**
	 * @brief Signals the size of the next batch of updates.
	 *
	 * Signaling the size of the next batch of updates helps the scheduler in deciding
	 * whether a 1 by 1 update approach is better than rebuilding the entire scheduler.
	 * @param k The amount of updates in the next batch of operations.
	 * @see doSingleUpdate
	 * @note Some scheduler implementations may use this method to fix the internal state.
	 * 	The correct order of calls to findUntil, update/updateAll and signalUpdateSize is the following:
	 * 		1. findUntil
	 * 		2. signalUpdateSize
	 * 		3. update/updateAll
	 */
	inline
	void
	signalUpdateSize(std::size_t k)
	{
		recalcKValue();
		m_updateSingle = singleReschedule(k);
	}

	/**
	 * If true, the scheduler has determined that subsequent calls to update
	 * will have a lower complexity than one call to updateAll.
	 * Otherwise, it is advised to use one call to updateAll to reschedule all items.
	 * @see signalUpdateSize
	 * @see update
	 * @see updateAll
	 */
	inline
	bool
	doSingleUpdate() const
	{ return m_updateSingle; }
};

//specialization for if no additional message is added to printScheduler
template<>
inline void
ModelHeapScheduler<n_model::t_raw_atomic>::printScheduler<>()
{
	//just cheat and add an empty string instead of copying the entire implementation.
	printScheduler("");
}

} /* namespace n_tools */



#endif /* SRC_SCHEDULER_MODELHEAPSCHEDULER_H_ */
