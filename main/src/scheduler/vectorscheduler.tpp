/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef VECTORSCHEDULER_TPP
#define VECTORSCHEDULER_TPP

#include <assert.h>
#include <utility>
#include "tools/globallog.h"

namespace n_scheduler {

template<typename X, typename R>
void VectorScheduler<X, R>::push_back(const R& item) {
#ifdef SAFETY_CHECKS
	if(contains(item)){
		throw std::logic_error("Error, scheduler already contains item");
	}
#endif
        
	const t_handle handle = m_storage.push(item);
	const size_t index = (size_t) item;
        if(index>=m_keys.size()){
                m_keys.resize(index+1);
                // Default construction is valid, nullptr + false pair
        }
        m_keys[index].first=handle;
        m_keys[index].second=true;
}

template<typename X, typename R>
std::size_t VectorScheduler<X, R>::size() const {
	return m_storage.size();
}

template<typename X, typename R>
const R& VectorScheduler<X, R>::top() {
#ifdef SAFETY_CHECKS
        
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to top.");
	}
#endif
        
	return m_storage.top();
}

template<typename X, typename R>
R VectorScheduler<X, R>::pop() {
	const R top_el = m_storage.top();
        m_storage.pop();
        const size_t index(top_el);
	m_keys[index].second=false;
	return top_el;
}

template<typename X, typename R>
bool VectorScheduler<X, R>::empty() const {
	return m_storage.empty();
}

template<typename X, typename R>
bool VectorScheduler<X, R>::isLockable() const {
	return false;
}

template<typename X, typename R>
void VectorScheduler<X, R>::clear() {
	m_keys.clear();
	m_storage.clear();
	testInvariant();
}

template<typename X, typename R>
void VectorScheduler<X, R>::unschedule_until(std::vector<R>& container,const R& elem) 
{
        while(! empty()){
		const R element = top();
		if (element < elem)
                        return;
		pop();
		container.push_back(element);
	}
}

template<typename X, typename R>
bool VectorScheduler<X, R>::contains(const R& elem) const {
        const size_t index(elem);
        if(index>=m_keys.size())
                return false;
        return m_keys[index].second;
}

template<typename X, typename R>
bool VectorScheduler<X, R>::erase(const R& elem) {
        if(!contains(elem))
                return false;
        auto& entry = m_keys[size_t(elem)];
        m_storage.erase(entry.first);
        entry.second=false;
        return true;
}

template<typename X, typename R>
void VectorScheduler<X, R>::printScheduler() const {
#if LOGGING
	LOG_DEBUG("Printing scheduler:");
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		LOG_DEBUG(" ", *iter);
	}
#endif
}

template<typename X, typename R>
void VectorScheduler<X, R>::testInvariant() const {
	size_t count_keys=0;
        for(const auto& entry : m_keys)
                if(entry.second)
                        ++count_keys;
        LOG_DEBUG("count_keys: ", count_keys, " size(): ", size());
        if(count_keys != size())
                throw std::logic_error("Scheduler invariant broken : keys!=heap items");
}

template<typename X, typename R>
void VectorScheduler<X, R>::hintSize(size_t expected){
        m_keys.reserve(expected);
}

template<typename X, typename R>
void VectorScheduler<X, R>::update(const R& item){
        if(!contains(item)){
                this->push_back(item);
        }else{
                const size_t index(item);
                auto& handle = m_keys[index].first;
                if(*handle != item){
                        m_storage.update(handle, item);
                }
        }
}

} // ENamespace
#endif
