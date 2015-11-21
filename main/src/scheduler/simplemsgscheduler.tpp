/**
 *  @author Ben Cardoen
 * */

#ifndef SIMPLEMSGSCHEDULER_TPP
#define SIMPLEMSGSCHEDULER_TPP

#include <assert.h>
#include "tools/globallog.h"

namespace n_scheduler {

template<typename X, typename R>
void SimpleMessageScheduler<X, R>::push_back(const R& item) {
	t_handle handle = m_storage.push(item);
	testInvariant();
}

template<typename X, typename R>
std::size_t SimpleMessageScheduler<X, R>::size() const {
	return m_storage.size();
}

template<typename X, typename R>
const R& SimpleMessageScheduler<X, R>::top() {
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to top.");
	}
	testInvariant();
	return m_storage.top();
}

template<typename X, typename R>
R SimpleMessageScheduler<X, R>::pop() {
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to pop.");
	}
	R top_el = m_storage.top();
	m_storage.pop();
	testInvariant();
	return top_el;
}

template<typename X, typename R>
bool SimpleMessageScheduler<X, R>::empty() const {
	return m_storage.empty();
}

template<typename X, typename R>
bool SimpleMessageScheduler<X, R>::isLockable() const {
	return false;
}

template<typename X, typename R>
void SimpleMessageScheduler<X, R>::clear() {
	m_storage.clear();
	testInvariant();
}

template<typename X, typename R>
void SimpleMessageScheduler<X, R>::unschedule_until(std::vector<R>& container,
		const R& time) {
	while (not m_storage.empty()) {
		const R element = m_storage.top(); // Note: using const & here gives false threading errors, a copy is required anyway.
		if (element < time){
			testInvariant();
			return;
		}
		// Else remove and copy
		m_storage.pop();
		container.push_back(element);
	}
	testInvariant();
}

template<typename X, typename R>
bool SimpleMessageScheduler<X, R>::contains(const R&) const {
	assert(false && "can't call contains on the SimpleMessageScheduler.");
        return false;
}

template<typename X, typename R>
bool SimpleMessageScheduler<X, R>::erase(const R&) {
    assert(false && "can't call erase on the SimpleMessageScheduler.");
    return false;
}

template<typename X, typename R>
void SimpleMessageScheduler<X, R>::printScheduler() const {
#if LOGGING
	LOG_DEBUG("Printing scheduler:");
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		LOG_DEBUG(" ", *iter);
	}
#endif
}

template<typename X, typename R>
void SimpleMessageScheduler<X, R>::testInvariant() const {
        LOG_DEBUG("void invariant : forwarding class.");
        ;
}

} // ENamespace
#endif
