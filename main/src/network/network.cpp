/*
 * network.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: Ben Cardoen
 */

#include "network.h"
namespace n_network {
Network::Network(size_t cores)
	: m_cores(cores), m_queues(m_cores), m_counting(false)
{
	LOG_DEBUG("NETWORK: Network constructor with ", cores, " queues.");
}

void Network::acceptMessage(const t_msgptr& msg)
{
	m_queues[msg->getDestinationCore()].push(msg);
	m_counting.store(true);
	LOG_DEBUG("NETWORK: Network accepting message");
}

Network::t_messages Network::getMessages(std::size_t coreid)
{
	LOG_DEBUG("NETWORK: Network sending msgs to ", coreid);
	return m_queues[coreid].purge();
}

bool Network::havePendingMessages(std::size_t coreid) const
{
	return m_queues[coreid].size() != 0;
}

bool Network::networkHasMessages(){
	/// First mark that we are checking.
	this->m_counting.store(false);
	/// Find first nonempty queue, if so immmediately return.
	for(size_t i = 0; i<m_cores; ++i){
		if(havePendingMessages(i))
			return true;
	}
	/// Else, if in the mean time accept has been called on a previously
	/// empty queue, check if accept has been called, and return that value.
	return m_counting;
}

}
