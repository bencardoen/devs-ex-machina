/*
 * Multicore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen
 */

#include <multicore.h>

using namespace n_model;
using n_control::t_location_tableptr;

Multicore::Multicore(const t_networkptr& net, std::size_t coreid, const t_location_tableptr& ltable)
:Core(coreid), m_network(net) , m_loctable(ltable)
{
}

void
Multicore::sendMessage(const t_msgptr& msg){
	this->m_network->acceptMessage(msg);
}

void
Multicore::processMessage(const t_msgptr& msg){
	t_timestamp messagetime = msg->getTimeStamp();
	if(messagetime < this->getTime()){
		// Trigger revert.
		LOG_ERROR("Got message from the past, my time : ", this->getTime(), " message time ", messagetime);
	}
	if(messagetime > this->getTime()){
		// Update a floating max
		if(messagetime > this->m_future_max){
			this->m_future_max = messagetime;
		}
	}
}

void
Multicore::getMessages(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag)
{
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("MCore id:" , this->getCoreID(), " received " , messages.size(), " messages ");
	this->sortIncoming(mailbag, messages);
}

void
Multicore::sortIncoming(std::unordered_map<std::string, std::vector<t_msgptr>>& mailbag, const std::vector<t_msgptr>& messages)
{
	for (const auto & message : messages) {
		assert(message->getDestinationCore() == this->getCoreID());
		const std::string& destname = message->getDestinationModel();
		auto found = mailbag.find(destname);
		if (found == mailbag.end()) {
			mailbag[destname] = std::vector<t_msgptr>();
		}
		mailbag[destname].push_back(message);
	}
}


bool
Multicore::isMessageLocal(const t_msgptr& msg)const{
	std::string destname = msg->getDestinationModel();
	const bool local = this->containsModel(destname);
	if (local) {
		msg->setDestinationCore(this->getCoreID());
		return true;
	} else {
		const size_t destid = m_loctable->lookupModel(destname);
		msg->setDestinationCore(destid);
		return false;
	}
}

void
Multicore::adjustTime(){
	// TODO expand
	t_timestamp scheduled_next = this->getTime();
	if(scheduled_next < this->m_future_max){
		this->setTime(this->m_future_max);
	}
}
