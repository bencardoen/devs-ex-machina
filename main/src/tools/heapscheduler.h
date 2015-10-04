/*
 * HeapScheduler.h
 *
 *  Created on: Oct 4, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_TOOLS_HEAPSCHEDULER_H_
#define SRC_TOOLS_HEAPSCHEDULER_H_

#include "tools/heap.h"
#include <assert.h>

namespace n_tools {

/**
 * A scheduler based on a heap.
 * @tparam Item The type of the items that will be stored here. Note that the heap will actually keep track of Item*.
 * @tparam Comp A comparator type. This type must implement operator()(Item*, Item*) const and must be default constructable.
 */
template<typename Item, typename Comp>
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
	std::vector<HeapElement> m_items;
	std::vector<HeapElement*> m_indexed;
	bool m_dirty;

public:
	/**
	 * Default constructor. Creates an empty heap scheduler.
	 */
	HeapScheduler():
		m_dirty(false)
	{ }

	/**
	 * Constructs a new heap scheduler
	 * @param size Reserves this amount of size, so that subsequent pushes up until that size do not result in the heap becoming dirty.
	 */
	HeapScheduler(std::size_t size):
		m_dirty(false)
	{
		reserve(size);
	}

	/**
	 * Reserves up to size spaces in the heap.
	 * @param size The new amount of space for elements.
	 * @note If the operation results into a relocation, the heap becomes dirty.
	 */
	inline
	void reserve(std::size_t size)
	{
		if(size > m_items.capacity() && !m_items.empty())
			m_dirty = true;
		m_items.reserve(size);
		m_indexed.reserve(size);
	}

	/**
	 * Returns the current size of the heap.
	 */
	inline
	std::size_t size() const
	{
		LOG_DEBUG("Getting size of scheduler: items size = ", m_items.size(), ", index size = ", m_indexed.size());
		if(!m_dirty)
			assert(m_items.size() == m_indexed.size() && "heapscheduler sizes not the same.");
		return m_items.size();
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
		return m_items[i];
	}


	/**
	 * Unchecked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	const HeapElement& operator[](std::size_t i) const
	{
		return m_items[i];
	}


	/**
	 * Checked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	HeapElement& at(std::size_t i)
	{
		return m_items.at(i);
	}

	/**
	 * Checked access operator.
	 * Returns the ith item in non-heap order.
	 */
	inline
	const HeapElement& at(std::size_t i) const
	{
		return m_items.at(i);
	}


	/**
	 * Unchecked access operator.
	 * Returns the ith item in heap order.
	 */
	inline
	Item* heapAt(std::size_t i) const
	{
		return m_indexed[i]->m_ptr;
	}

	/**
	 * Removes all items from the scheduler so that it is empty.
	 * The heap will not be dirty when this operation is finished.
	 */
	inline
	void clear()
	{
		m_items.clear();
		m_indexed.clear();
		m_dirty = false;
	}

	/*
	 * Adds a new item to the heap.
	 * @note If this results in internal relocations, the heap becomes dirty.
	 */
	inline
	void push_back(Item* item)
	{
		if(m_items.size() == m_items.capacity()){
			m_items.push_back(HeapElement(item, m_items.size()));
			m_indexed.clear();
			m_dirty = true;
		}else{
			m_items.push_back(HeapElement(item, m_items.size()));
			m_indexed.push_back(&(m_items.back()));
		}
	}

	/**
	 * Updates the internal heap so that the heap property is met.
	 * If the heap was dirty before, it becomes clean.
	 */
	inline
	void updateAll()
	{
		if(m_indexed.size() != m_items.size()){
			m_indexed.clear();
			m_indexed.reserve(m_items.size());
			for(typename std::vector<HeapElement>::iterator it = m_items.begin(); it != m_items.end(); ++it){
				m_indexed.push_back(&(*it));
			}
		}
		std::make_heap(m_indexed.begin(), m_indexed.end(), m_comp);
		for(std::size_t i = 0; i < m_indexed.size(); ++i){
			m_indexed[i]->m_index = i;
		}
		m_dirty = false;
	}

	/**
	 * Updates a single item.
	 * @precondition The heap is not dirty.
	 */
	inline
	void update(std::size_t index)
	{
		assert(!m_dirty && "The heapscheduler is dirty.");
		n_tools::fix_heap(m_indexed.begin(), m_indexed.end(), m_indexed.begin()+m_items[index].m_index, m_comp, m_upd);
	}

	inline
	typename std::vector<HeapElement>::iterator begin()
	{
		return m_items.begin();
	}

	inline
	typename std::vector<HeapElement>::const_iterator begin() const
	{
		return m_items.begin();
	}

	inline
	typename std::vector<HeapElement>::iterator end()
	{
		return m_items.end();
	}

	inline
	typename std::vector<HeapElement>::const_iterator end() const
	{
		return m_items.end();
	}

	inline
	HeapElement& front()
	{
		return *m_indexed.front();
	}

	inline
	const HeapElement& front() const
	{
		return *m_indexed.front();
	}

	/**
	 * Removes an item from the heap.
	 * @note The heap will become dirty.
	 */
	inline
	void remove(std::size_t index)
	{
		HeapElement item = m_items[index];
		LOG_DEBUG("Removing item ", index, " at heap index ", item.m_index);
		std::swap(m_items[index], m_items.back());
		m_items.pop_back();
		if(m_dirty)
			return;
	        std::swap(*(m_indexed.begin() + item.m_index), m_indexed.back());	//swap with last one and remove from the heap
	        m_indexed.pop_back();
	        m_indexed[item.m_index]->m_index = item.m_index;
	}

	/**
	 * Tests whether te heap condition is met.
	 * Note that this is never the case if the heap is dirty.
	 * @see update
	 * @see updateAll
	 */
	inline
	bool isHeap() const
	{
		return (!m_dirty && std::is_heap(m_indexed.begin(), m_indexed.end(), m_comp));
	}
};
} /* namespace n_tools */

#endif /* SRC_TOOLS_HEAPSCHEDULER_H_ */
