/*
 * genericscheduler.h
 *
 *  Created on: Oct 21, 2015
 *      Author: Devs Ex Machina
 */

#ifndef SRC_SCHEDULER_GENERICMODELSCHEDULER_H_
#define SRC_SCHEDULER_GENERICMODELSCHEDULER_H_

#include "model/atomicmodel.h"
#include "model/modelentry.h"
#include "tools/globallog.h"
#include "tools/msgscheduler.h"
#include "tools/vectorscheduler.h"
#include <boost/heap/pairing_heap.hpp>

namespace n_tools {

template<template<typename...T> class Container = n_tools::VectorScheduler, template<typename...T> class Heap = boost::heap::pairing_heap>
class GenericModelScheduler: public Container<Heap<n_model::ModelEntry>, n_model::ModelEntry>
{
private:
	typedef Container<
			Heap<n_model::ModelEntry>,
			n_model::ModelEntry> t_base;

	std::vector<n_model::t_raw_atomic> m_index;
	std::vector<n_model::ModelEntry> m_imm_ids;
public:
	GenericModelScheduler() = default;
	GenericModelScheduler(std::size_t size)
	{
		reserve(size);
	}

	/**
	 * Reserves up to size spaces in the scheduler.
	 * @param size The new amount of space for elements.
	 * @note In node-based schedulers, this operation will have no effect.
	 */
	inline
	void reserve(std::size_t size)
	{
		m_index.reserve(size);
		m_imm_ids.reserve(size);
		t_base::hintSize(size);
	}

	void clear()
	{
		m_index.clear();
		m_imm_ids.clear();
		t_base::clear();
	}

	void findUntil(std::vector<n_model::t_raw_atomic> &container, const n_network::t_timestamp& mark)
	{
		//note: if m_imm_ids is not empty, this means that findUntil was called twice in a row without rescheduling anything.
		//therefore, everything must be updated at once.
		for(const auto& item: m_imm_ids){
			const std::size_t id = item.getID();
			n_model::t_raw_atomic mdl = m_index[id];
			const n_network::t_timestamp newt = mdl->getTimeLast() + mdl->timeAdvance();
			const n_model::ModelEntry entry(id, newt);
			if (t_base::contains(entry)) {
				LOG_INFO(" scheduleModel Tried to schedule a model that is already scheduled: ", mdl->getName(), ", replacing.");
				t_base::erase(entry);			// Needed for revert, scheduled entry may be wrong.
			}
			if(newt != n_network::t_timestamp::infinity()) {
				LOG_INFO(" refused to reschedule ", mdl->getName(), " at time ", newt);
				t_base::push_back(entry);
			}
		}
		m_imm_ids.clear();	//any waiting items have been removed.
		t_base::unschedule_until(m_imm_ids, n_model::ModelEntry(0, n_network::t_timestamp(mark.getTime(), n_network::t_timestamp::MAXCAUSAL)));
		for (const auto& entry : m_imm_ids) {
			auto model = m_index[entry.getID()];
			model->markInternal();
			container.push_back(model);
		}
	}

	template<typename... T>
	void printScheduler(T...) const
	{
		t_base::printScheduler();
	}

	void push_back(n_model::t_raw_atomic item)
	{
		LOG_DEBUG("adding item ", item->getName(), " with id ", item->getLocalID(), " and index size ", m_index.size());
		assert(item->getLocalID() == m_index.size() && "Newly added item must have local ID equal to current scheduler size.");
		m_index.push_back(item);
		update(item->getLocalID());
	}

	void remove(std::size_t index)
	{
		std::swap(m_index[index], m_index.back());
		m_index.pop_back();
		const n_model::ModelEntry entry(index, n_network::t_timestamp());
		if (t_base::contains(entry)) {
			t_base::erase(entry);
		} else {
			//remove entry from m_imm_ids. It can only be in here if the scheduler doesn't contain this entry already
			auto it = m_imm_ids.begin();
			while(it != m_imm_ids.end()){
				if(it->getID() == index){
					m_imm_ids.erase(it);
					break;
				}
				++it;
			}
		}
	}

	inline
	std::size_t indexSize() const
	{ return m_index.size(); }

	n_network::t_timestamp topTime()
	{ return t_base::size()? t_base::top().getTime(): n_network::t_timestamp::infinity(); }

	void update(std::size_t id)
	{
		n_model::t_raw_atomic model = m_index[id];
		const n_network::t_timestamp newt(model->getTimeNext().getTime(), model->getPriority());
		const n_model::ModelEntry entry(id, newt);
		if (t_base::contains(entry)) {
			LOG_INFO(" scheduleModel Tried to schedule a model that is already scheduled: ", model->getName(), ", replacing.");
			t_base::erase(entry);			// Needed for revert, scheduled entry may be wrong.
		}
		if(newt.getTime() != n_network::t_timestamp::MAXTIME) {
			LOG_INFO(" rescheduled ", model->getName(), " at time ", newt);
			t_base::push_back(entry);
		}
	}



	inline
	void signalUpdateSize(std::size_t)
	{ m_imm_ids.clear(); }	//this is called just before all items are updated. We are now certain that the imminents won't need to be rescheduled.

	inline
	void updateAll()
	{
		m_imm_ids.clear();
		t_base::clear();
		for(n_model::t_raw_atomic mdl: m_index){
			const std::size_t id = mdl->getLocalID();
			const n_network::t_timestamp newt = mdl->getTimeLast() + mdl->timeAdvance();
			const n_model::ModelEntry entry(id, newt);
			assert(!t_base::contains(entry) && "Scheduler was cleared, but somehow still contains an entry.");
			if(newt != n_network::t_timestamp::infinity()) {
				LOG_INFO(" refused to reschedule ", mdl->getName(), " at time ", newt);
				t_base::push_back(entry);
			}
		}
	}

	inline
	bool dirty() const
	{ return false; }

	inline
	bool doSingleUpdate() const
	{ return true; }

	inline
	void recalcKValue() const
	{}

	using t_base::testInvariant;
};

} /* namespace n_tools */

#endif /* SRC_SCHEDULER_GENERICMODELSCHEDULER_H_ */
