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
#include<g2log.hpp>
#include <vector>
#include <array>

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
	bool m_verbose;
public:

	typedef std::vector<t_msgptr> t_messages;

	Network():m_verbose(true){
		LOG_IF(DEBUG, m_verbose) << "Network constructor with " << cores << " queues.";
	}

	void
	setVerbose(bool v){m_verbose=v;}

	/**
	 * Called by a core pushing a message to the network.
	 * @pre : msg has destination core id set.
	 * @post : msg will be queued for destination.
	 */
	void
	acceptMessage(const t_msgptr& msg){
		CHECK(msg->getDestinationCore()<cores)<< "Core index invalid : " << msg->getDestinationCore()  << " geq than " << cores;
		LOG_IF(DEBUG, m_verbose) << "Network accepting message";
		m_queues[msg->getDestinationCore()].push(msg);
	}

	/**
	 * Called by a core when it is ready to process messages.
	 * @return Any messages queued for calling core. (can be empty)
	 * @attention locked
	 * @pre coreid < cores
	 */
	t_messages
	getMessages(std::size_t coreid){
		CHECK(coreid < cores) << "Core index invalid : " << coreid  << " geq than " << cores;
		LOG_IF(DEBUG, m_verbose) << "Network sending msgs to " << coreid;
		return m_queues[coreid].purge();
	}

	bool
	havePendingMessages(std::size_t coreid)const{
		CHECK(coreid < cores) << "Core index invalid : " << coreid  << " geq than " << cores;
		return m_queues[coreid].size()!=0;
	}
};

}// end namespace



#endif /* SRC_NETWORK_NETWORK_H_ */
