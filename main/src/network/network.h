/*
 * network.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_NETWORK_H_
#define SRC_NETWORK_NETWORK_H_

#include "message.h"
#include "msgqueue.h"
#include <vector>
#include <array>
#include "tools/globallog.h"

namespace n_network{

/**
 * Abstraction of Network (Cores communicating via network).
 * Receives (push) messages from cores, messages are pulled from destination core.
 * @attention : individual queues are synchronized.
 */
class Network{
private:
	size_t	m_cores;
	std::vector<Msgqueue<t_msgptr>> m_queues;
public:
	typedef std::vector<t_msgptr> t_messages;

	Network() = delete;

	Network(size_t cores):m_cores(cores), m_queues(m_cores){
		LOG_DEBUG("Network constructor with ", cores, " queues.");
	}

	/**
	 * Called by a core pushing a message to the network.
	 * @pre : msg has destination core id set.
	 * @post : msg will be queued for destination.
	 */
	void
	acceptMessage(const t_msgptr& msg){
		m_queues[msg->getDestinationCore()].push(msg);
		LOG_DEBUG("Network accepting message");
	}

	/**
	 * Called by a core when it is ready to process messages.
	 * @return Any messages queued for calling core. (can be empty)
	 * @attention locked
	 * @pre coreid < cores
	 */
	t_messages
	getMessages(std::size_t coreid){
		LOG_DEBUG("Network sending msgs to ", coreid);
		return m_queues[coreid].purge();
	}

	bool
	havePendingMessages(std::size_t coreid)const{
		return m_queues[coreid].size()!=0;
	}
};


typedef std::shared_ptr<Network> t_networkptr;

}// end namespace



#endif /* SRC_NETWORK_NETWORK_H_ */
