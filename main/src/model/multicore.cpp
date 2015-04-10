/*
 * Multicore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen
 */

#include <multicore.h>

using namespace n_model;
using n_control::t_location_tableptr;
using namespace n_network;

Multicore::~Multicore(){
	this->m_loctable.reset();
	this->m_network.reset();
}

Multicore::Multicore(const t_networkptr& net, std::size_t coreid, const t_location_tableptr& ltable, std::mutex& lock)
:Core(coreid), m_network(net) , m_loctable(ltable),m_color(MessageColor::WHITE), m_vlock(lock),m_tmin(t_timestamp::infinity())
{
}

void
Multicore::sendMessage(const t_msgptr& msg){
	msg->setSourceCore(this->getCoreID());
	size_t coreid = this->m_loctable->lookupModel(msg->getDestinationModel());
	msg->setDestinationCore(coreid);
	this->m_network->acceptMessage(msg);
}


void
Multicore::getMessages()
{
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("MCore id:" , this->getCoreID(), " received " , messages.size(), " messages ");
	this->sortIncoming(messages);
}

void
Multicore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	for (const auto & message : messages) {
		assert(message->getDestinationCore() == this->getCoreID());
		this->receiveMessage(message);
	}
}


