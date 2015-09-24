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
	std::vector<Q> 	m_queue;
public:
	/**
	 * Add element to queue
	 * @threadsafe
	 */
	void
	push(const Q& element){
		std::lock_guard<std::mutex> lock(m_lock);
		m_queue.push_back(element);
	}

	/**
	 * Return contents of queue.
	 * @syncrhonized
	 */
	std::vector<Q>
	purge(){
		std::lock_guard<std::mutex> lock(m_lock);
#ifdef USE_STAT
		m_msgcountstat += m_queue.size();
#endif
		auto contents(std::move(m_queue));
		m_queue.clear();
		return contents;
	}

	/**
	 * Report how many messages are queued.
	 */
	inline std::size_t
	size()const{
		std::lock_guard<std::mutex> lock(m_lock);	// lock because size could change
		return m_queue.size();
	}

//-------------statistics gathering--------------
//#ifdef USE_STAT
private:
	n_tools::t_uintstat m_msgcountstat;
	static std::size_t m_counter;
public:
	Msgqueue(): m_msgcountstat(std::string("_network/messagequeue") + n_tools::toString(++m_counter), "messages")
{
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
