/*
 * msgqueue.h
 *
 *  Created on: 12 Mar 2015
 *      Author: ben
 */

#ifndef SRC_NETWORK_MSGQUEUE_H_
#define SRC_NETWORK_MSGQUEUE_H_

#include <deque>
#include <memory>

namespace n_network {

template<typename Q>
class Msgqueue
{
private:
	std::mutex	m_lock;
	std::vector<Q> 	m_queue;
public:
	void
	push(const Q& element){
		std::lock_guard<std::mutex> lock(m_lock);
		m_queue.push_back(element);
	}

	std::vector<Q>
	purge(){
		std::lock_guard<std::mutex> lock(m_lock);
		auto contents(std::move(m_queue));
		m_queue.clear();
		return contents;
	}
};

} /* namespace n_network */

#endif /* SRC_NETWORK_MSGQUEUE_H_ */
