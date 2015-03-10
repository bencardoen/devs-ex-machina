/**
 *  Implementation of class template SyncScheduler.
 *  @author Ben Cardoen
 * */

#ifndef SYNCHRONIZEDSCHEDULER_TPP
#define SYNCHRONIZEDSCHEDULER_TPP

#include <assert.h>

namespace n_tools {

template<typename X, typename R>
void SynchronizedScheduler<X, R>::push_back(const R& item) {
	std::lock_guard<std::mutex> lock(m_lock);
	t_handle handle = m_storage.push(item);
	m_hashtable.insert(std::make_pair(item, handle));
	assert(m_storage.size() == m_hashtable.size() && "Inserting discrepancy.");
}

template<typename X, typename R>
std::size_t SynchronizedScheduler<X, R>::size() const {
	std::lock_guard<std::mutex> lock(m_lock);
	return m_storage.size();
}

template<typename X, typename R>
const R& SynchronizedScheduler<X, R>::top() {
	std::lock_guard<std::mutex> lock(m_lock);
	if (m_storage.empty()) {
		throw std::out_of_range("No elements in scheduler to top.");
	}
	return m_storage.top();
}

template<typename X, typename R>
R SynchronizedScheduler<X, R>::pop() {
	std::lock_guard<std::mutex> lock(m_lock);
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
bool SynchronizedScheduler<X, R>::empty() const {
	std::lock_guard<std::mutex> lock(m_lock);
	return m_storage.empty();
}

template<typename X, typename R>
bool SynchronizedScheduler<X, R>::isLockable() const {
	return true;
}

template<typename X, typename R>
void SynchronizedScheduler<X, R>::clear() {
	std::lock_guard<std::mutex> lock(m_lock);
	m_storage.clear();
	m_hashtable.clear();// TODO Ensure both or none are executed. Not sure if this is possible.
}

template<typename X, typename R>
void SynchronizedScheduler<X, R>::unschedule_until(std::vector<R>& container,
		const R& time) {
	// Two options to implement this : use ordered iterators (NlogN) or pop until(log n)
	// We need to lock the whole heap during the operation to ensure consistent results.
	// TODO test edge cases , empty scheduler , unschedule more than present etc....
	std::lock_guard<std::mutex> lock(m_lock);
	while (not m_storage.empty()) {
		const R element = m_storage.top(); // Note: using const & here gives false threading errors, a copy is required anyway.
		if (element < time)
			return;
		m_storage.pop();
		m_hashtable.erase(element);
		container.push_back(element); // TODO if push_back fails, the element is lost forever (neither in container, nor in scheduler.
	}
}

template<typename X, typename R>
bool SynchronizedScheduler<X, R>::contains(const R& elem) const {
	std::lock_guard<std::mutex> lock(m_lock);
	return (m_hashtable.find(elem) != m_hashtable.end());
}

template<typename X, typename R>
bool SynchronizedScheduler<X, R>::erase(const R& elem) {
	std::lock_guard<std::mutex> lock(m_lock);
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

} // ENamespace
#endif
