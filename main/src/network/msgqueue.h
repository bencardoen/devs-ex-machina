/*
 * msgqueue.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_MSGQUEUE_H_
#define SRC_NETWORK_MSGQUEUE_H_

#include <deque>
#include <memory>
#include <vector>
#include "tools/statistic.h"

namespace n_network {

/**
 * Synchronized Message Queue.
 */
template<typename Q>
class Msgqueue
{
private:
	mutable std::mutex	m_lock;
        std::atomic<size_t>     m_size;
	std::vector<Q> 	m_queue;
public:

	/**
	 * Add element to queue
	 * @threadsafe
	 */
	void
	push(const Q& element){
		std::lock_guard<std::mutex> lock(m_lock);
                ++m_size;
		m_queue.push_back(element);
	}

	/**
	 * Insert a range of items to the queue
	 * @threadsafe
	 */
	template<typename T>
	void
	insert(T begin, T end){
		static_assert(std::is_same<typename std::iterator_traits<T>::value_type, Q>::value, "Can't insert a different type of items.");
		std::lock_guard<std::mutex> lock(m_lock);
		m_queue.insert(m_queue.end(), begin, end);
		m_size += std::distance(begin, end);
	}

	/**
	 * Return contents of queue.
	 * @syncrhonized
	 */
	std::vector<Q>
	purge(){
		std::lock_guard<std::mutex> lock(m_lock);
#ifdef USE_STAT
		m_msgcountstat += m_queue.size();       // This is fast, but only reports send messages, not all if there are remaining.
#endif
                std::vector<Q> contents;
                m_size -= m_queue.size();
                contents.swap(m_queue);
		m_queue.clear();        // Should not be necessary
		return contents;
	}

	/**
	 * Report how many messages are queued.
	 */
	inline std::size_t
	size()const{
		//std::lock_guard<std::mutex> lock(m_lock);	// lock because size could change
		return m_size;
	}

//-------------statistics gathering--------------
//#ifdef USE_STAT
private:
	n_tools::t_uintstat m_msgcountstat;
	static std::size_t m_counter;
public:
	Msgqueue():m_size(0), m_msgcountstat(std::string("_network/messagequeue") + n_tools::toString(m_counter++), "messages")
{
}
        
        ~Msgqueue(){
#ifdef SAFETY_CHECKS
                std::lock_guard<std::mutex> lock(m_lock); // This should not be necessary, but why risk it.
                if(m_queue.size()!=0){
                        LOG_DEBUG("QUEUE NETWORK not empty :: pointers follow:");
                        for(const auto& msg : m_queue){
                                if(std::is_pointer<Q>::value){
                                        LOG_DEBUG(msg);
                                }
                        }
                }
#endif
        }
	/**
	 * @brief Prints some basic stats.
	 * @param out The output will be printed to this stream.
	 */
	void printStats(std::ostream& out = std::cout) const
	{
		out << m_msgcountstat;
	}
//#endif
};

//#ifdef USE_STAT
template<typename Q>
std::size_t Msgqueue<Q>::m_counter = 0;
//#endif


} /* namespace n_network */

#endif /* SRC_NETWORK_MSGQUEUE_H_ */
