/*
 * HeapScheduler.h
 *
 *  Created on: Oct 4, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_TOOLS_HEAPSCHEDULER_H_
#define SRC_TOOLS_HEAPSCHEDULER_H_

#include "tools/heap.h"

namespace n_tools {

template<typename Item, typename Comp>
class HeapScheduler
{
private:
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

	struct HeapComparator: public Comp
	{
		bool operator()(HeapElement* a, HeapElement* b) const
		{
			return Comp::operator()(a->m_ptr, b->m_ptr);
		}
	} m_comp;
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

public:

	inline
	void reserve(std::size_t size)
	{
		m_items.reserve(size);
		m_indexed.reserve(size);
	}

	inline
	std::size_t size() const
	{
		return m_items.size();
	}

	inline
	HeapElement& operator[](std::size_t i)
	{
		return m_items[i];
	}

	inline
	const HeapElement& operator[](std::size_t i) const
	{
		return m_items[i];
	}

	inline
	HeapElement& at(std::size_t i)
	{
		return m_items.at(i);
	}

	inline
	const HeapElement& at(std::size_t i) const
	{
		return m_items.at(i);
	}

	inline
	void clear()
	{
		m_items.clear();
		m_indexed.clear();
	}

	inline
	void push_back(Item* item)
	{
		m_items.push_back(HeapElement(item, m_items.size()));
		m_indexed.push_back(&(m_items.back()));
	}

	inline
	void updateAll()
	{
		std::make_heap(m_indexed.begin(), m_indexed.end(), m_comp);
		for(std::size_t i = 0; i < m_indexed.size(); ++i){
			m_indexed[i]->m_index = i;
		}
	}

	inline
	void update(std::size_t index)
	{
		n_tools::fix_heap(m_indexed.begin(), m_indexed.end(), m_indexed.begin()+index, m_comp, m_upd);
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

	inline
	void remove(std::size_t index)
	{
		HeapElement* item = &(*(m_items.begin()+index));
		std::swap(m_items[index], m_items.back());
		m_items.pop_back();
	        auto heapIter = std::find(m_indexed.begin(), m_indexed.end(), item);
	        std::swap(*heapIter, m_indexed.back());	//swap with last one and remove from the heap
	        m_indexed.pop_back();
	}

	inline
	bool isHeap() const
	{
		return std::is_heap(m_indexed.begin(), m_indexed.end(), m_comp);
	}
};
} /* namespace n_tools */

#endif /* SRC_TOOLS_HEAPSCHEDULER_H_ */
