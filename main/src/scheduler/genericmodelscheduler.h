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
#include <type_traits>

namespace n_tools {


/**
 * A generic atomic model scheduler.
 * The core will communicate with a model scheduler through the interface provided by this class.
 *
 * @tparam Container A subclass of n_tools::Scheduler.
 * 			This container will dictate how items are stored internally, ordered by the local ID of the Atomic Model.
 * @tparam Heap The container needs a heap data structure to order the atomic models by scheduled time.
 * @precondition The Container class is a subclass of n_tools::Scheduler.
 */
template<template<typename...T> class Container = n_tools::VectorScheduler, template<typename...T> class Heap = boost::heap::pairing_heap>
class GenericModelScheduler: public Container<Heap<n_model::ModelEntry>, n_model::ModelEntry>
{
private:
	static_assert(std::is_base_of<n_tools::Scheduler<n_model::ModelEntry>,
					Container<Heap<n_model::ModelEntry>, n_model::ModelEntry>>::value,
			"The container type must be a derived class of n_tools::Scheduler.");

	typedef Container<
			Heap<n_model::ModelEntry>,
			n_model::ModelEntry> t_base;

	std::vector<n_model::t_raw_atomic> m_index;
	std::vector<n_model::ModelEntry> m_imm_ids;
public:
	/**
	 * Default constructor. Constructs an empty model heap scheduler.
	 */
	GenericModelScheduler() = default;

	/**
	 * Constructs a new, empty scheduler but reserves some memory for quick insertions.
	 * @param size The new scheduler will reserve memory so that it can at least schedule this amount of
	 * 		items without having to do a reallocation.
	 * @see reserve
	 * @note Whether or not actual memory is reserves depends on the underlying scheduler type.
	 * 	Typically, node based schedulers, such as a fibonacci heap, won't be able to allocate memory,
	 * 	but array based schedulers, such as a vector scheduler, can.
	 */
	GenericModelScheduler(std::size_t size)
	{
		reserve(size);
	}

	/**
	 * Reserves up to size spaces in the scheduler.
	 * @param size The new amount of space for elements.
	 * @note In some schedulers, this operation will have no effect.
	 * 	Whether or not actual memory is reserves depends on the underlying scheduler type.
	 * 	Typically, node based schedulers, such as a fibonacci heap, won't be able to allocate memory,
	 * 	but array based schedulers, such as a vector scheduler, can.
	 */
	inline
	void reserve(std::size_t size)
	{
		m_index.reserve(size);
		m_imm_ids.reserve(size);
		t_base::hintSize(size);
	}

	/**
	 * Removes all items from the scheduler.
	 */
	void clear()
	{
		m_index.clear();
		m_imm_ids.clear();
		t_base::clear();
	}

        /**
         * @brief Finds all items with a next internal transition time smaller than or equal to some timestamp.
         * @param container The found items will be collected in this container
         * @param mark All items with next internal transition time less than or equal to this timestamp will be collected.
         *
         * Complexity: depends on the underlying scheduler type. at least O(K+k)
         * 		where k is the amount of found items
         * 		and K the complexity of retrieving the first k elements from the scheduler.
         */
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

	/**
	 * @brief prints a textural representation of the internals of the scheduler to the global log file.
	 * @note If logging is disabled, this method is reduced to a no op.
	 * @param vals... Some schedulers may use the value(s) passed as arguments to put them in front of every logged line.
	 * 		  This may be used to differentiate between multiple instances of the scheduler in the same log file.
	 * 		  However, this is not the general case, so they will just be ignored in this case.
	 * 		  An example of a scheduler that does implement this functionality is the ModelHeapScheduler
	 */
	template<typename... T>
	void printScheduler(T...) const
	{
		t_base::printScheduler();
	}

	/*
	 * Adds a new item to the heap. This item may not be present in the heap already.
	 * @see update if you want to reschedule an item instead.
	 */
	void push_back(n_model::t_raw_atomic item)
	{
		LOG_DEBUG("adding item ", item->getName(), " with id ", item->getLocalID(), " and index size ", m_index.size());
		assert(item->getLocalID() == m_index.size() && "Newly added item must have local ID equal to current scheduler size.");
		m_index.push_back(item);
		update(item->getLocalID());
	}

	/**
	 * Removes an item from the heap.
	 */
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

	/**
	 * @return The total amount of items in the scheduler.
	 * @note Some schedulers may not schedule items with a timestamp of infinity.
	 * 	In that case, the return value of size() may be smaller than the return value of this method.
	 * 	The total amount of items depends only on the amount of push_back and remove operations.
	 */
	inline
	std::size_t indexSize() const
	{ return m_index.size(); }

	/**
	 * @return The minimal scheduled time of all items in the scheduler.
	 */
	n_network::t_timestamp topTime()
	{ return t_base::size()? t_base::top().getTime(): n_network::t_timestamp::infinity(); }


	/**
	 * Updates a single item.
	 * @param id The ID of the item that must be updated.
	 *
	 *
	 * Complexity: Depends on the underlying scheduler type.
	 *
	 * @see singleReschedule
	 * @see updateAll
	 */
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


	/**
	 * @brief Signals the size of the next batch of updates.
	 *
	 * Signaling the size of the next batch of updates helps the scheduler in deciding
	 * whether a 1 by 1 update approach is better than rebuilding the entire scheduler.
	 * @param k The amount of updates in the next batch of operations.
	 * @see doSingleUpdate
	 * @see update
	 * @note Some scheduler implementations may use this method to fix the internal state.
	 * 	The correct order of calls to findUntil, update/updateAll and signalUpdateSize is the following:
	 * 		1. findUntil
	 * 		2. signalUpdateSize
	 * 		3. update/updateAll
	 */
	inline
	void signalUpdateSize(std::size_t)
	{ m_imm_ids.clear(); }	//this is called just before all items are updated. We are now certain that the imminents won't need to be rescheduled.

	/**
	 * Updates all items in the scheduler.
	 *
	 * Complexity: Depends on the underlying scheduler type.
	 * 		Sometimes, one call to updateAll may have a lower complexity
	 * 		than a number of subsequent calls to update
	 * @see update
	 * @see updateAll
	 */
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

	/**
	 * Some schedulers may become 'dirty' in certain conditions.
	 * These conditions will always be explicitly mentioned in the documentation of that particular scheduler.
	 * The GenericModelScheduler will never become dirty.
	 * If a scheduler is dirty, this can always be fixed with a call to updateAll or clear.
	 */
	inline
	bool dirty() const
	{ return false; }

	/**
	 * If true, the scheduler has determined that subsequent calls to update
	 * will have a lower complexity than one call to updateAll.
	 * Otherwise, it is advised to use one call to updateAll to reschedule all items.
	 * @see signalUpdateSize
	 * @see update
	 * @see updateAll
	 */
	inline
	bool doSingleUpdate() const
	{ return true; }

	using t_base::testInvariant;
};

} /* namespace n_tools */

#endif /* SRC_SCHEDULER_GENERICMODELSCHEDULER_H_ */
