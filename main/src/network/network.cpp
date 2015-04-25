/*
 * network.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: Ben Cardoen
 */

#include "network.h"
namespace n_network {
Network::Network(size_t cores)
	: m_cores(cores), m_queues(m_cores)
{
	LOG_DEBUG("NETWORK: Network constructor with ", cores, " queues.");
}

void Network::acceptMessage(const t_msgptr& msg)
{
	m_queues[msg->getDestinationCore()].push(msg);
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

}
