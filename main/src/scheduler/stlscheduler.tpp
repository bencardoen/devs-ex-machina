/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#ifndef STLSCHEDULER_TPP
#define STLSCHEDULER_TPP

#include <assert.h>
#include <utility>
#include <algorithm>


namespace n_scheduler {

template<typename R>
void STLScheduler<R>::push_back(const R& item) {
	m_storage.push(item);
}

template<typename R>
std::size_t STLScheduler<R>::size() const {
	return m_storage.size();
}

template<typename R>
const R& STLScheduler<R>::top() {
        assert(m_storage.size()!=0);
        return m_storage.top();
}

template<typename R>
R STLScheduler<R>::pop() {
        R val = this->top();
        m_storage.pop();
        return val;
}

template<typename R>
bool STLScheduler<R>::empty() const {
	return m_storage.empty();
}

template<typename R>
bool STLScheduler<R>::isLockable() const {
	return false;
}

template<typename R>
void STLScheduler<R>::clear() {
        while(!m_storage.empty())
                m_storage.pop();
}

template<typename R>
void STLScheduler<R>::unschedule_until(std::vector<R>& container,const R& elem) 
{
        while(!empty()){
                R value = top();
                if(value < elem)
                        return;
                pop();
                container.push_back(value);
        }
}

template<typename R>
bool STLScheduler<R>::contains(const R& /*elem*/) const {
        throw std::logic_error("Not supported");
        return false;
        
}

template<typename R>
bool STLScheduler<R>::erase(const R& /*elem*/) {
        throw std::logic_error("Not supported");
        return false;
}

template<typename R>
void STLScheduler<R>::printScheduler() const {
#if LOGGING
        // Can't iterate over pqueue, todo.
        assert(false);
#endif
}

template<typename R>
void STLScheduler<R>::testInvariant() const {
	
}



} // ENamespace
#endif
