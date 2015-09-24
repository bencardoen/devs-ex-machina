/*
 * network.h
 *
 *  Created on: 12 Mar 2015
 *      Author: Ben Cardoen
 */

#ifndef SRC_NETWORK_NETWORK_H_
#define SRC_NETWORK_NETWORK_H_

#include "network/message.h"
#include "network/msgqueue.h"
#include <vector>
#include <atomic>
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
	std::atomic<bool> m_counting;
public:
	typedef std::vector<t_msgptr> t_messages;

	Network() = delete;

	/**
	 * Create a network object with #cores.
	 */
	Network(size_t cores);

	/**
	 * Called by a core pushing a message to the network.
	 * @pre : msg has destination core id set.
	 * @post : msg will be queued for destination.
	 */
	void
	acceptMessage(const t_msgptr& msg);

	/**
	 * Called by a core when it is ready to process messages.
	 * @return Any messages queued for calling core. (can be empty)
	 * @attention locked
	 * @pre coreid < cores
	 */
	t_messages
	getMessages(std::size_t coreid);

	/**
	 * Check if the network has any pending messages.
	 * Use this if you don't want to pop pending messages, only check for them.
	 */
	bool
	havePendingMessages(std::size_t coreid)const;

	/**
	 * Report if there are still transient messages in the network.
	 * @attention : expensive check.
	 * This function has to walk the (changing) queues and request non-zero size.
	 * If we are checking queue #4, and then a message comes in at #3 (empty when we checked),
	 * our result could be wrong. We therefore check if a trigger in accept is set.
	 * @return true if there are transient message from time of call to time of return.
	 */
	bool networkHasMessages();

//-------------statistics gathering--------------
	void printStats(std::ostream& out = std::cout) const
	{
#ifdef USE_STAT
		for(const auto& i:m_queues)
			i.printStats(out);
#endif
	}
};


typedef std::shared_ptr<Network> t_networkptr;

}// end namespace



#endif /* SRC_NETWORK_NETWORK_H_ */
