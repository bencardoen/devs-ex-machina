/**
 *  @author Ben Cardoen
 * */

#ifndef STLSCHEDULER_TPP
#define STLSCHEDULER_TPP

#include <assert.h>
#include <utility>
#include <algorithm>

namespace n_scheduler {

template<typename R>
void STLScheduler<R>::push_back(const R& item) {
	m_storage.push_front(item);
        std::make_heap(m_storage.begin(),m_storage.end());
}

template<typename R>
std::size_t STLScheduler<R>::size() const {
	return m_storage.size();
}

template<typename R>
const R& STLScheduler<R>::top() {
        assert(m_storage.size()!=0);
        return m_storage.front();
}

template<typename R>
R STLScheduler<R>::pop() {
        R val = this->top();
        m_storage.pop_front();
        std::make_heap(m_storage.begin(), m_storage.end());
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
        m_storage.clear();
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
bool STLScheduler<R>::contains(const R& elem) const {
        const size_t key(elem);
        auto cmp = [key](const R& val)->bool{
                return (key==size_t(val));
        };
        const auto& found = std::find_if(m_storage.begin(), m_storage.end(), cmp);
        return found!=m_storage.end();
}

template<typename R>
bool STLScheduler<R>::erase(const R& elem) {
        const size_t key(elem);
        auto cmp = [key](const R& val)->bool{
                return (key==size_t(val));
        };
        auto found = std::find_if(m_storage.begin(), m_storage.end(), cmp);
        if(found==m_storage.end())
                return false;
        m_storage.erase(found);
        return true;
}

template<typename R>
void STLScheduler<R>::printScheduler() const {
#if LOGGING
	LOG_DEBUG("Printing scheduler:");
	for(auto iter=m_storage.rbegin(); iter!=m_storage.rend(); ++iter){
		LOG_DEBUG(" ", *iter);
	}
#endif
}

template<typename R>
void STLScheduler<R>::testInvariant() const {
	
}



} // ENamespace
#endif
