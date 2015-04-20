/**
 *  @author Ben Cardoen
 * */

#ifndef UNSYNCHRONIZEDSCHEDULER_TPP
#define UNSYNCHRONIZEDSCHEDULER_TPP

#include <assert.h>

namespace n_tools {

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::push_back(const R& item) {
	t_handle handle = m_storage.push(item);
	m_hashtable.insert(std::make_pair(item, handle));
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
	return m_storage.top();
}

template<typename X, typename R>
R UnSynchronizedScheduler<X, R>::pop() {
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to pop.");
	}
	R top_el = m_storage.top();
	size_t erased = m_hashtable.erase(top_el);
	assert(erased == 1 && "Hashtable && heap out of sync.");
	m_storage.pop();
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
	m_storage.clear();
	m_hashtable.clear();// TODO Ensure both or none are executed. Not sure if this is possible.
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::unschedule_until(std::vector<R>& container,
		const R& time) {
	while (not m_storage.empty()) {
		const R element = m_storage.top(); // Note: using const & here gives false threading errors, a copy is required anyway.
		if (element < time)
			return;
		m_storage.pop();
		m_hashtable.erase(element);
		container.push_back(element);// TODO if push_back fails, the element is lost forever (neither in container, nor in scheduler.
	}
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
		m_storage.erase(handle);// TODO These two calls need to be executed both or not at all.
		m_hashtable.erase(elem);
		return true;
	} else {
		return false;
	}
}

template<typename X, typename R>
void UnSynchronizedScheduler<X, R>::printScheduler()  {
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		R stored = *iter;
		std::cout << stored << std::endl;
	}
}

} // ENamespace
#endif