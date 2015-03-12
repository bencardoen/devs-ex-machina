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
#include <boost/log/trivial.hpp>

namespace n_network{

/**
 * Abstraction of Network (Cores communicating via network).
 * Receives (push) messages from cores, messages are pulled from destination core.
 * @attention : individual queues are synchronized.
 */
template<size_t cores=1>
class Network{
private:
	std::array<Msgqueue<t_msgptr>, cores> m_queues;
public:
	typedef std::vector<t_msgptr> t_messages;

	/**
	 * Called by a core pushing a message to the network.
	 * @pre : msg has destination core id set.
	 * @post : msg will be queued for destination.
	 */
	void
	acceptMessage(const t_msgptr& msg){
		//BOOST_LOG_TRIVIAL(trace) << "Receiving msg for " << msg->getDestinationCore();
		m_queues[msg->getDestinationCore()].push(msg);
	}

	/**
	 * Called by a core when it is ready to process messages.
	 * @return Any messages queued for calling core. (can be empty)
	 * @attention locked
	 */
	t_messages
	getMessages(std::size_t coreid){
		//BOOST_LOG_TRIVIAL(trace) << "Returning msgs to "<<coreid;
		return m_queues[coreid].purge();
	}
};

}// end namespace



#endif /* SRC_NETWORK_NETWORK_H_ */
