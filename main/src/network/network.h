/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
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
        std::atomic<int_fast64_t> m_count;
	
public:
	typedef std::vector<t_msgptr> t_messages;

	Network() = delete;

	/**
	 * Create a network object with #cores.
	 */
	Network(size_t cores);
        
        /**
         */
        ~Network() = default;

	/**
	 * Called by a core pushing a message to the network.
	 * @pre : msg has destination core id set.
	 * @post : msg will be queued for destination.
	 */
	void
	acceptMessage(const t_msgptr& msg);

	/**
	 * Adds an entire range of messages to the network.
	 * These messages must all have the same core as destination.
	 * @param coreID The id of the destination core
	 * @param msgs A container of messages.
	 * @pre: coreID is a valid core id.
	 * @pre: All messages in msgs have coreID as destination.
	 * @post: all messages will be queued for the destination core.
	 */
	void
	giveMessages(size_t coreID, const std::vector<t_msgptr>& msgs);

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
        
        bool
        empty()const;

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
