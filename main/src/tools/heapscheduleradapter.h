/*
 * heapscheduleradapter.h
 *
 *  Created on: Oct 19, 2015
 *      Author: lttlemoi
 */

#ifndef SRC_TOOLS_HEAPSCHEDULERADAPTER_H_
#define SRC_TOOLS_HEAPSCHEDULERADAPTER_H_


#include "tools/scheduler.h"
#include "tools/heapscheduler.h"
#include "model/atomicmodel.h"
#include "tools/globallog.h"

namespace n_tools {

// Forward declare friend
template<typename T>
class SchedulerFactory;

template<typename S>
class HeapSchedulerAdapter: public Scheduler<S> {
	static_assert(std::is_same<S, n_model::t_raw_atomic>::value,
		"The heap scheduler is currently only implemented for n_model::t_raw_atomic.");
};

template<>
class HeapSchedulerAdapter<n_model::t_raw_atomic>: public Scheduler<n_model::t_raw_atomic, n_network::t_timestamp, n_model::t_raw_atomic>
{
	friend class SchedulerFactory<n_model::t_raw_atomic> ;
private:
	struct Comparator
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
        n_tools::HeapScheduler<n_model::AtomicModel_impl, Comparator>  m_storage;
        /**
         * Working vector for getting the imminent models
         */
        std::vector<std::size_t> m_imminentIndexes;

public:
        HeapSchedulerAdapter() {
		;
	}

	virtual ~HeapSchedulerAdapter() {
		;
	}

	/**
	 * @brief push Schedule item.
	 * @pre a has a fully ordered implementation of S::operator< or std::less<S>
	 * @post size += 1
	 */
	virtual void push_back(n_model::t_raw_atomic a) override
	{ return m_storage.push_back(a); }

	virtual size_t size() const override
	{ return m_storage.size(); }

	/**
	 * @brief empty
	 * @attention : if you use empty to protect from
	 * popping empty scheduler, make sure both empty() and pop()
	 * are wrapped in a single lock.
	 * @return
	 */
	virtual bool empty() const override
	{ return !m_storage.size(); }

	virtual n_model::t_raw_atomic top() override
	{ return m_storage.front().m_ptr; }

	virtual n_model::t_raw_atomic pop() override
	{
		n_model::t_raw_atomic fr = top();
		m_storage.remove(fr->getLocalID());
		return fr;
	}

	virtual bool isLockable() const override
	{ return false; }

	virtual
	void
	unschedule_until(std::vector<n_model::t_raw_atomic> &container, const n_network::t_timestamp& mark) override
	{
		std::size_t heapsize = m_storage.size();
		m_imminentIndexes.push_back(0);

		while(m_imminentIndexes.size()){
			std::size_t i = m_imminentIndexes.back();
			m_imminentIndexes.pop_back();
			if(i >= heapsize)
				continue;
			const n_model::t_raw_atomic ptr = m_storage.heapAt(i);
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

	void
	clear() override
	{ m_storage.clear(); }

	virtual
	bool
	contains(n_model::t_raw_atomic elem) const override
	{ return (elem->getLocalID() < m_storage.size() && elem == m_storage.at(elem->getLocalID()).m_ptr); }

	virtual
	bool
	erase(n_model::t_raw_atomic elem) override
	{ m_storage.remove(elem->getLocalID()); return true; }

	virtual
	void
	printScheduler()override
	{
	#ifdef LOGGING
		LOG_DEBUG(" Scheduler state:");
		LOG_DEBUG("    heap models size: ", m_storage.size());
		for(std::size_t i = 0; i < m_storage.size(); ++i){
			auto m = m_storage.heapAt(i);
			LOG_DEBUG("    ", i,"\t", m_storage[m->getLocalID()], "\t:  model: ", m->getName(), ", time: ", m->getTimeNext());
		}
	#endif
	}

	virtual
	void
	testInvariant()override
	{ assert(m_storage.isHeap() && "Heap scheduler heap property is violated"); }

	virtual
        void
        update(n_model::t_raw_atomic elem) override
        { m_storage.update(elem->getLocalID()); }

	virtual
        void
        updateAll() override
        { m_storage.updateAll(); }

	virtual
        void
        hintSize(size_t size) override
        { m_storage.reserve(size); }
};

} /* namespace n_tools */



#endif /* SRC_TOOLS_HEAPSCHEDULERADAPTER_H_ */
