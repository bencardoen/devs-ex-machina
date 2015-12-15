/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef UNSYNCHRONIZEDSCHEDULER_TPP
#define UNSYNCHRONIZEDSCHEDULER_TPP

#include <assert.h>

namespace n_scheduler {

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::push_back(const R& item) {
	if(m_hashtable.find(item)!=m_hashtable.end()){
		throw std::logic_error("Error, scheduler already contains item");
	}
	t_handle handle = m_storage.push(item);
	m_hashtable.insert(std::make_pair(item, handle));
	testInvariant();
	assert(m_storage.size() == m_hashtable.size() && "Inserting discrepancy.");
}

template<typename X, typename R>
std::size_t UnSynchronizedScheduler<X, R>::size() const {
	return m_storage.size();
}

template<typename X, typename R>
const R& UnSynchronizedScheduler<X, R>::top() {
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to top.");
	}
	testInvariant();
	return m_storage.top();
}

template<typename X, typename R>
R UnSynchronizedScheduler<X, R>::pop() {
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to pop.");
	}
	R top_el = m_storage.top();
	m_hashtable.erase(top_el);
	m_storage.pop();
	testInvariant();
	return top_el;
}

template<typename X, typename R>
bool UnSynchronizedScheduler<X, R>::empty() const {
	return m_storage.empty();
}

template<typename X, typename R>
bool UnSynchronizedScheduler<X, R>::isLockable() const {
	return false;
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::clear() {
	m_hashtable.clear();
	m_storage.clear();
	testInvariant();
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::unschedule_until(std::vector<R>& container,
		const R& time) {
	while (not m_storage.empty()) {
		const R element = m_storage.top(); // Note: using const & here gives false threading errors, a copy is required anyway.
		if (element < time){
			testInvariant();
			return;
		}
		// Else remove and copy
		m_storage.pop();
		m_hashtable.erase(element);
		container.push_back(element);
	}
	testInvariant();
}

template<typename X, typename R>
bool UnSynchronizedScheduler<X, R>::contains(const R& elem) const {
	return (m_hashtable.find(elem) != m_hashtable.end());
}

template<typename X, typename R>
bool UnSynchronizedScheduler<X, R>::erase(const R& elem) {
	auto found = m_hashtable.find(elem);
	if (found != m_hashtable.end()) {
		auto handle = found->second;// The actual type of the handle is a typedef listed in the header file.
		m_storage.erase(handle);
		m_hashtable.erase(elem);
		testInvariant();
		return true;
	} else {
		testInvariant();
		return false;
	}
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::printScheduler() const {
#if LOGGING
	LOG_DEBUG("Printing scheduler:");
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		R stored = *iter;
		LOG_DEBUG("  ", stored);
	}
#endif
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::testInvariant() const {
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

} // ENamespace
#endif
