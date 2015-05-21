/*
 * listscheduler.h
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_TOOLS_LISTSCHEDULER_H_
#define SRC_TOOLS_LISTSCHEDULER_H_

#include <list>
#include <unordered_map>
#include "scheduler.h"

namespace n_tools {

// Forward declare friend
template<typename T>
class SchedulerFactory;

/**
 * Port of PyPDevs sorted linked list based scheduler.
 * Complexity:
 * 	push() : O(n logn) (sorting linked list)
 * 	pop()/top() : O(1)
 * 	unschedule_until: O(k) with k items to be unscheduled.
 * 	erase : O(1)
 * 	contains: O(1)
 * @brief Linked list based scheduler.
 */
template<typename T>
class Listscheduler: public Scheduler<T>
{
private:
	std::list<T> m_storage;
	std::unordered_map<T, typename decltype(m_storage)::iterator> m_hashtable;
public:
	Listscheduler()
	{
		;
	}
	virtual ~Listscheduler()
	{
		;
	}

	virtual void push_back(const T& element) override
	{
		if(m_hashtable.find(element) != m_hashtable.end() ){
			throw std::logic_error("Error, scheduler already contains item");
		}
		auto handle = m_storage.insert(m_storage.end(), element);
		m_hashtable.insert(std::make_pair(element, handle));
		m_storage.sort();
		testInvariant();
	}

	virtual size_t size() const override
	{
		return m_hashtable.size();
	}

	virtual bool empty() const override
	{
		return m_hashtable.empty();
	}

	/**
	 * @brief top Retrieve first item in scheduler, do not remove it.
	 * This is not const, if T = pointer, access to const T& can
	 * invalidate the heap.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 */
	virtual const T& top() override
	{
		return m_storage.back();
	}

	/**
	 * @brief pop Remove the first entry in the scheduler.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 * @post size-=1
	 */
	virtual T pop() override
	{
		if (m_storage.empty()) {
			throw std::out_of_range("No elements in scheduler to pop.");
		}
		T top_el = m_storage.back();
		m_storage.pop_back();
		m_hashtable.erase(top_el);
		testInvariant();
		return top_el;
	}

	/**
	 * @brief isLockable : gives an indication if the implementing subclass is synchronized or not. Note that single operation synchronization can be
	 * provided,  multiple operations (eg. size() && pop() are never safe without your own lock.
	 */
	virtual
	bool isLockable() const override
	{
		return false;
	}

	virtual
	void unschedule_until(std::vector<T>& container, const T& time) override
	{
		while (not m_storage.empty()) {
			const T element = m_storage.back();
			if (element < time){
				testInvariant();
				return;
			}
			m_storage.pop_back();
			m_hashtable.erase(element);
			container.push_back(element);
		}
		testInvariant();
	}

	virtual
	void clear() override
	{
		m_storage.clear();
		m_hashtable.clear();
	}

	virtual
	bool contains(const T& elem) const override
	{
		return (m_hashtable.find(elem) != m_hashtable.end());
	}

	virtual
	bool erase(const T& elem) override
	{
		if (this->m_hashtable.find(elem) != m_hashtable.end()) {
			auto handle = m_hashtable[elem];
			m_hashtable.erase(elem);
			m_storage.erase(handle);
			testInvariant();
			return true;
		} else {
			testInvariant();
			return false;
		}
	}

	virtual
	void printScheduler() override
	{
		for (const auto& elem : m_storage) {
			std::cout << elem << std::endl;
		}
	}

	virtual
	void testInvariant() override{
		if(m_storage.size() != m_hashtable.size()){
			std::stringstream ss;
			ss << "Invariant Scheduler failed :: \n";
			ss << "Current size of hashmap == ";
			ss << m_hashtable.size();
			ss << " != current size of heap :: ";
			ss << m_storage.size();
			throw std::logic_error(ss.str());
		}
	}
};

/**
 * @brief synchronized version of Listscheduler.
 */
template<typename T>
class SyncedListscheduler: public Listscheduler<T>
{
private:
	mutable std::mutex m_lock;
public:
	SyncedListscheduler()
	{
		;
	}
	virtual ~SyncedListscheduler()
	{
		;
	}

	virtual void push_back(const T& element) override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		Listscheduler<T>::push_back(element);
	}

	virtual size_t size() const override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::size();
	}

	virtual bool empty() const override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::empty();
	}

	/**
	 * @brief top Retrieve first item in scheduler, do not remove it.
	 * This is not const, if T = pointer, access to const T& can
	 * invalidate the heap.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 */
	virtual const T& top() override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::top();
	}

	/**
	 * @brief pop Remove the first entry in the scheduler.
	 * @pre (not this.empty())
	 * @throws std::out_of_range if empty().
	 * @post size-=1
	 */
	virtual T pop() override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::pop();
	}

	/**
	 * @brief isLockable : gives an indication if the implementing subclass is synchronized or not. Note that single operation synchronization can be
	 * provided,  multiple operations (eg. size() && pop() are never safe without your own lock.
	 */
	virtual
	bool isLockable() const override
	{
		return true;
	}

	/**
	 * @brief unschedule_until Remove all items from the scheduler until their priority (time) exceeds the parameter.
	 * @param container Ordered result of popped items.
	 * @param time max priority/time value (included) of items to be removed from scheduler.
	 */
	virtual
	void unschedule_until(std::vector<T>& container, const T& time) override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		Listscheduler<T>::unschedule_until(container, time);
	}

	/**
	 * Remove all items from the scheduler.
	 */
	virtual
	void clear() override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		Listscheduler<T>::clear();
	}

	/**
	 * Return true if the given element is present in the scheduler.
	 * @note O(1) (hashtable lookup).
	 */
	virtual
	bool contains(const T& elem) const override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::contains(elem);
	}

	/**
	 * Erase the element from the scheduler if present.
	 * @note O(log n)
	 * @return true if element was found and erased, false otherwise.
	 */
	virtual
	bool erase(const T& elem) override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		return Listscheduler<T>::erase(elem);
	}

	virtual
	void printScheduler() override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		Listscheduler<T>::printScheduler();
	}

	virtual
	void testInvariant()override
	{
		std::lock_guard<std::mutex> lock(m_lock);
		Listscheduler<T>::testInvariant();
	}
};

} /* namespace n_model */

#endif /* SRC_TOOLS_LISTSCHEDULER_H_ */
