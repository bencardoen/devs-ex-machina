/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#include "network/network.h"
#include <assert.h>

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

void Network::giveMessages(size_t coreID, const std::vector<t_msgptr>& msgs)
{
#ifdef SAFETY_CHECKS
	assert(coreID < m_cores && "The coreID is invalid.");
	for(t_msgptr msg: msgs)
		assert(coreID == msg->getDestinationCore() && "All messages must have the same destination core.");
#endif
	m_count += msgs.size();
	m_queues[coreID].insert(msgs.begin(), msgs.end());
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
