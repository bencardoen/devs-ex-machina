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
	bool m_dirty;

public:
	HeapScheduler():
		m_dirty(false)
	{ }

	HeapScheduler(std::size_t size):
		m_dirty(false)
	{
		reserve(size);
	}

	inline
	void reserve(std::size_t size)
	{
		m_items.reserve(size);
		m_indexed.reserve(size);
	}

	inline
	std::size_t size() const
	{
		LOG_DEBUG("Getting size of scheduler: items size = ", m_items.size(), ", index size = ", m_indexed.size());
		assert(m_items.size() == m_indexed.size() && "heapscheduler sizes not the same.");
		return m_items.size();
	}

	inline
	bool dirty() const
	{
		return m_dirty;
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
	Item* heapAt(std::size_t i) const
	{
		return m_indexed[i]->m_ptr;
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
		if(m_items.size() == m_items.capacity()){
			m_items.push_back(HeapElement(item, m_items.size()));
			m_indexed.clear();
			m_dirty = true;
		}else{
			m_items.push_back(HeapElement(item, m_items.size()));
			m_indexed.push_back(&(m_items.back()));
		}
	}

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

	inline
	void remove(std::size_t index)
	{
		HeapElement item = m_items[index];
		LOG_DEBUG("Removing item ", index, " at heap index ", item.m_index);
		std::swap(m_items[index], m_items.back());
		m_items.pop_back();
	        std::swap(*(m_indexed.begin() + item.m_index), m_indexed.back());	//swap with last one and remove from the heap
	        m_indexed.pop_back();
	        m_indexed[item.m_index]->m_index = item.m_index;
	        m_dirty = true;
	}

	inline
	bool isHeap() const
	{
		return (!m_dirty && std::is_heap(m_indexed.begin(), m_indexed.end(), m_comp));
	}
};
} /* namespace n_tools */

#endif /* SRC_TOOLS_HEAPSCHEDULER_H_ */
