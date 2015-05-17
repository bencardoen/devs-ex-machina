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
	if(m_hashtable.find(item)!=m_hashtable.end()){
		throw std::logic_error("Error, scheduler already contains item");
	}
	t_handle handle = m_storage.push(item);
	m_hashtable.insert(std::make_pair(item, handle));
	testInvariant();
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
	m_hashtable.erase(top_el);
	m_storage.pop();
	testInvariant();
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
	m_hashtable.clear();
	m_storage.clear();
	testInvariant();
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
		if (element < time){
			testInvariant();
			return;
		}
		m_storage.pop();
		m_hashtable.erase(element);
		container.push_back(element);
	}
	testInvariant();
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
void SynchronizedScheduler<X, R>::printScheduler()  {
	std::lock_guard<std::mutex> lock(m_lock);
	auto iter = m_storage.ordered_begin();
	for(;iter != m_storage.ordered_end(); ++iter){
		R stored = *iter;
		std::cout << stored << std::endl;
	}
}

template<typename X, typename R>
void SynchronizedScheduler<X, R>::testInvariant(){
	if(m_storage.size() != m_hashtable.size()){
		throw std::logic_error("Invariant of scheduler violated, sizes of heap and map do not match!!");
	}
}

} // ENamespace
#endif
