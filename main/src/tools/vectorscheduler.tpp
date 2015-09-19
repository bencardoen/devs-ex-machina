/**
 *  @author Ben Cardoen
 * */

#ifndef VECTORSCHEDULER_TPP
#define VECTORSCHEDULER_TPP

#include <assert.h>
#include <utility>

namespace n_tools {

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
		const R element = top(); // Note: using const & here gives false threading errors, a copy is required anyway.
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
void VectorScheduler<X, R>::printScheduler()  {
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		std::cout << *iter << std::endl;
	}
}

template<typename X, typename R>
void VectorScheduler<X, R>::testInvariant() {
	size_t count_keys=0;
        for(const auto& entry : m_keys)
                if(entry.second)
                        ++count_keys;
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
                m_storage.update(handle, item);
        }
}

} // ENamespace
#endif
