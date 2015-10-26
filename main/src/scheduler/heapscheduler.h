/*
 * HeapScheduler.h
 *
 *  Created on: Oct 4, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_HEAPSCHEDULER_H_
#define SRC_SCHEDULER_HEAPSCHEDULER_H_

#include "tools/heap.h"
#include "tools/misc.h"
#include <assert.h>

namespace n_tools {

/**
 * A scheduler based on a heap. This scheduler is best used when a limited number of items must be kept
 * in a heap structure and when the ordering of those items varies a lot throughout the program execution.
 * @tparam Item The type of the items that will be stored here. Note that the heap will actually keep track of Item*.
 * @tparam Comp A comparator type. This type must implement operator()(Item*, Item*) const and must be default constructable.
 * @tparam AlwaysRecalc If true, the scheduler will always recalculate the heuristic that it
 * 			uses to determine whether multiple individual updates are better
 * 			than one general update. Depending on how the scheduler is used,
 * 			it may be better to turn this value off.
 * 			The default value is true. @see recalcKValue, singleReschedule
 */
template<typename Item, typename Comp, bool AlwaysRecalc = true>
class HeapScheduler
{
private:
	/**
	 * Actual element that is kept around.
	 * Contains an index that points to it's place in the actual heap.
	 */
	struct HeapElement
	{
		Item* m_ptr;
		std::size_t m_index;

		constexpr HeapElement(Item* ptr = nullptr, std::size_t i = 0u)
			: m_ptr(ptr), m_index(i)
		{}

		Item* operator->() const
		{ return m_ptr; }
		Item& operator*() const
		{ return *m_ptr; }
		operator Item*() const
		{ return m_ptr; }

		friend
		constexpr
		bool operator==(const HeapElement& a, Item* b){
			return a.m_ptr == b;
		}
	};

	/**
	 * Actual comparator object.
	 */
	struct HeapComparator: public Comp
	{
		bool operator()(HeapElement* a, HeapElement* b) const
		{
			return Comp::operator()(a->m_ptr, b->m_ptr);
		}
	} m_comp;

	/**
	 * Updator object. Will update the swapped objects during the fix_heap calls.
	 */
	struct HeapUpdator
	{
		void operator()(HeapElement* a, std::size_t b) const
		{
			// need to test for greater than, because std::make_heap constructs a max heap
			// and we need a min heap.
			a->m_index = b;
		}
	} m_upd;

	std::vector<HeapElement> m_index;
	std::vector<HeapElement*> m_heap;
	bool m_dirty;
	// if less than this amount of models must be rescheduled, do them 1by1.
	// Otherwise, all in one go.
	std::size_t m_kValue;

public:
	/**
	 * Default constructor. Constructs an empty heap scheduler.
	 */
	HeapScheduler():
		m_dirty(false), m_kValue(0)
	{ }

	/**
	 * Constructs a new, empty heap scheduler
	 * @param size	Reserves this amount of size, so that subsequent pushes up until that size
	 * 		do not result in the heap becoming dirty.
	 */
	HeapScheduler(std::size_t size):
		m_dirty(false), m_kValue(0)
	{
		reserve(size);
	}

	/**
	 * Reserves up to size spaces in the heap.
	 * @param size The new amount of space for elements.
	 * @note If the operation results into a relocation,
	 * 	the heap becomes dirty and must be cleaned before further use.
	 * @see updateAll
	 */
	inline
	void reserve(std::size_t size)
	{
		if(size > m_index.capacity() && !m_index.empty())
			m_dirty = true;
		m_index.reserve(size);
		m_heap.reserve(size);
	}

	/**
	 * Returns the current size of the heap.
	 */
	inline
	std::size_t size() const
	{
		assert((!m_dirty || m_index.size() == m_heap.size()) && "heapscheduler sizes not the same.");
		return m_index.size();
	}

	/**
	 * Indicates whether the heap is dirty or not.
	 * If it is dirty, the only way to make it clean again is to call updateAll() or clear().
	 * @see updateAll
	 * @see clear
	 */
	inline
	bool dirty() const
	{
		return m_dirty;
	}

	/**
	 * Unchecked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	HeapElement& operator[](std::size_t i)
	{
		return m_index[i];
	}


	/**
	 * Unchecked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	const HeapElement& operator[](std::size_t i) const
	{
		return m_index[i];
	}


	/**
	 * Checked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	HeapElement& at(std::size_t i)
	{
		return m_index.at(i);
	}

	/**
	 * Checked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	const HeapElement& at(std::size_t i) const
	{
		return m_index.at(i);
	}


	/**
	 * Unchecked access operator.
	 * Returns the ith item in heap order.
	 */
	inline
	Item* heapAt(std::size_t i) const
	{
		return m_heap[i]->m_ptr;
	}

	/**
	 * Removes all items from the scheduler so that it is empty.
	 * The heap will not be dirty when this operation is finished.
	 * @see dirty
	 */
	inline
	void clear()
	{
		m_index.clear();
		m_heap.clear();
		m_dirty = false;
		m_kValue = 0;
	}

	/*
	 * Adds a new item to the heap. This item may not be present in the heap already.
	 * @note If this operation results in internal relocations, the heap becomes dirty.
	 * @see dirty
	 * @see update if you want to reschedule an item instead.
	 */
	inline
	void push_back(Item* item)
	{
		if(m_index.size() == m_index.capacity()){
			m_index.push_back(HeapElement(item, m_index.size()));
			m_heap.clear();
			m_dirty = true;
		}else{
			m_index.push_back(HeapElement(item, m_index.size()));
			m_heap.push_back(&(m_index.back()));
		}
		if(AlwaysRecalc)
			recalcKValue();
	}

	/**
	 * Updates the internal heap so that the heap property is met.
	 * If the heap was dirty before, it becomes clean.
	 *
	 * Complexity: O(3N) with N the total amount of items in the scheduler.
	 * @see dirty
	 */
	inline
	void updateAll()
	{
		if(m_dirty)
			this->recalcKValue();
		if(m_heap.size() != m_index.size()){
			m_heap.clear();
			m_heap.reserve(m_index.size());
			for(typename std::vector<HeapElement>::iterator it = m_index.begin(); it != m_index.end(); ++it){
				m_heap.push_back(&(*it));
			}
		}
		std::make_heap(m_heap.begin(), m_heap.end(), m_comp);
		for(std::size_t i = 0; i < m_heap.size(); ++i){
			m_heap[i]->m_index = i;
		}
		m_dirty = false;
	}

	/**
	 * Updates a single item.
	 * @precondition The heap is not dirty.
	 * @see dirty
	 * @see singleReschedule
	 * @see updateAll
	 *
	 * Complexity: O(log(N)) with N the total amount of items in the scheduler.
	 */
	inline
	void update(std::size_t index)
	{
		assert(!m_dirty && "The heapscheduler is dirty.");
		n_tools::fix_heap(m_heap.begin(), m_heap.end(), m_heap.begin()+m_index[index].m_index, m_comp, m_upd);
	}

	/**
	 * Returns an iterator pointing to the first item in indexed order.
	 */
	inline
	typename std::vector<HeapElement>::iterator begin()
	{
		return m_index.begin();
	}

	/**
	 * Returns a const iterator pointing to the first item in indexed order.
	 */
	inline
	typename std::vector<HeapElement>::const_iterator begin() const
	{
		return m_index.begin();
	}

	/**
	 * Returns an iterator pointing to the end of the sequence of items in indexed order.
	 */
	inline
	typename std::vector<HeapElement>::iterator end()
	{
		return m_index.end();
	}

	/**
	 * Returns a const iterator pointing to the end of the sequence of items in indexed order.
	 */
	inline
	typename std::vector<HeapElement>::const_iterator end() const
	{
		return m_index.end();
	}

	/**
	 * Returns a reference to the first element in heap order, if it exists.
	 */
	inline
	HeapElement& front()
	{
		return *m_heap.front();
	}

	/**
	 * Returns a reference to the first element in heap order, if it exists.
	 */
	inline
	const HeapElement& front() const
	{
		return *m_heap.front();
	}

	/**
	 * Removes an item from the heap.
	 * @note The heap will become dirty.
	 * @see dirty
	 */
	inline
	void remove(std::size_t index)
	{
		HeapElement item = m_index[index];
		LOG_DEBUG("Removing item ", index, " at heap index ", item.m_index);
		std::swap(m_index[index], m_index.back());
		m_index.pop_back();
		if(AlwaysRecalc)
			recalcKValue();
		if(m_dirty)
			return;
	        std::swap(*(m_heap.begin() + item.m_index), m_heap.back());	//swap with last one and remove from the heap
	        m_heap.pop_back();
	        m_heap[item.m_index]->m_index = item.m_index;
	}

	/**
	 * Tests whether te heap condition is met.
	 * Note that this is never the case if the heap is dirty.
	 * @see update
	 * @see updateAll
	 * @see dirty
	 */
	inline
	bool isHeap() const
	{
		return (!m_dirty && std::is_heap(m_heap.begin(), m_heap.end(), m_comp));
	}

	/**
	 * Depending on the total amount of elements, one call to updateAll() can have a better performance
	 * than a series of calls to update(). Given an amount of items to reschedule, this method
	 * will tell whether individual updates are better than one call to updateAll.
	 *
	 * @see update
	 * @see updateAll
	 * @see racalcKValue
	 */
	inline
	bool singleReschedule(std::size_t amount) const
	{
		return amount <= m_kValue;
	}

	/**
	 * Forces the scheduler to recalculate the heuristic used in singleReschedule.
	 * If the class template boolean parameter AlwaysRecalc was set to true, the scheduler will
	 * always update this value itself.
	 */
	inline
	void recalcKValue()
	{
        	const std::size_t N = m_index.size();
        	m_kValue = N>1?(3*N/(n_tools::intlog2(N))):0;
	};
};
} /* namespace n_tools */

#endif /* SRC_SCHEDULER_HEAPSCHEDULER_H_ */
