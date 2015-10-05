/*
 * network.cpp
 *
 *  Created on: 25 Apr 2015
 *      Author: Ben Cardoen
 */

#include "network/network.h"
namespace n_network {
Network::Network(size_t cores)
	: m_cores(cores), m_queues(m_cores),m_count(0)
{
	LOG_DEBUG("NETWORK: Network constructor with ", cores, " queues.");
}

void Network::acceptMessage(const t_msgptr& msg)
{
        ++m_count;
	m_queues[msg->getDestinationCore()].push(msg);
	LOG_DEBUG("\tNETWORK: Network accepting message");
}

Network::t_messages Network::getMessages(std::size_t coreid)
{
	LOG_DEBUG("\tNETWORK: Network sending msgs to ", coreid);
        auto val = m_queues[coreid].purge();
        m_count-=val.size();
#ifdef SAFETY_CHECKS
        if(m_count.load()<0){
                LOG_DEBUG("Access to getMessages() with param ", coreid, " caused underflow in count");
                LOG_FLUSH;
                throw std::logic_error("Network count less than zero ");
        }
#endif
        return val;
}

bool Network::havePendingMessages(std::size_t coreid) const
{
	return m_queues[coreid].size() != 0;
}

bool Network::empty()const{
        return (m_count==0);
}

}
