/*
 * Multicore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen
 */

#include <multicore.h>
#include "objectfactory.h"


using namespace n_model;
using n_control::t_location_tableptr;
using namespace n_network;

Multicore::~Multicore(){
	this->m_loctable.reset();
	this->m_network.reset();
}

Multicore::Multicore(const t_networkptr& net, std::size_t coreid, const t_location_tableptr& ltable, std::mutex& lock, size_t cores)
:Core(coreid), m_network(net) , m_loctable(ltable),m_color(MessageColor::WHITE), m_vlock(lock),m_tmin(t_timestamp::infinity())
{
	m_mcount_vector = n_tools::createObject<V>(cores);
}

void
Multicore::sendMessage(const t_msgptr& msg){
	msg->setSourceCore(this->getCoreID());
	size_t coreid = this->m_loctable->lookupModel(msg->getDestinationModel());
	msg->setDestinationCore(coreid);
	msg->paint(this->getColor());
	LOG_DEBUG("MCore:: sending message", msg->toString());
	this->m_network->acceptMessage(msg);
	LOG_DEBUG("MCore:: storing sent message", msg->toString());
	this->m_sent_messages.push_back(msg);
}

void
Multicore::countMessage(const t_msgptr& msg){
	//TODO Tim 1.4
	std::lock_guard<std::mutex> lock(this->m_vlock);
	if(this->getColor() == MessageColor::WHITE){
		size_t j = msg->getDestinationCore();
		this->m_mcount_vector->getVector()[j]+=1;
	}else{
		m_tmin = std::min(m_tmin, msg->getTimeStamp());
	}
}

void
Multicore::receiveMessage(const t_msgptr& msg){// TODO Tim 1.5
	// first let superclass do its thing
	Core::receiveMessage(msg);
	// then do 1.5
	std::lock_guard<std::mutex> lock(this->m_vlock);
	if(msg->getColor()==MessageColor::WHITE){
		this->m_mcount_vector->getVector()[this->getCoreID()] -= 1;		// Fails if core id is not properly set by controller.
	}
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

void
Multicore::receiveControl(const t_controlmsg& msg){	// TODO Tim 1.7/6
	if(this->getCoreID()==0){
		// same as below, @see 1.7 in pdf.
		// 1.7
	}else{
		this->m_tmin = t_timestamp::infinity();	// LOCK
		this->m_color = MessageColor::RED;	// LOCK
		while(true){// TODO replace with condition variables
			std::lock_guard<std::mutex> lock(m_vlock);
			int count = this->m_mcount_vector->getVector()[this->getCoreID()];
			count += msg->getCountVector()[this->getCoreID()];
			if(count <= 0)
				break;	// Lock is released, but that's ok, we're no longer getting white messages.
		}
		// Equivalent to sending message, controlmessage is passed to next core.
		t_timestamp tclock = msg->getClock();
		t_timestamp tsend = msg->getSend();
		msg->setClock(std::min(tclock, this->getTime()));
		msg->setSend(std::min(this->m_tmin, tsend));
		t_count& Count = msg->getCountVector();
		for(size_t i =0; i< Count.size(); ++i){
			Count[i] += this->m_mcount_vector->getVector()[i];
			this->m_mcount_vector->getVector()[i] = 0;
		}
	}
}


void
Multicore::markProcessed(const std::vector<t_msgptr>& messages) {
	for(const auto& msg : messages){
		LOG_DEBUG("MCore : storing processed msg", msg->toString());
		std::lock_guard<std::mutex> lock(m_locallock);
		this->m_processed_messages.push_back(msg);
	}
}

void
Multicore::setGVT(const t_timestamp& newgvt){
	Core::setGVT(newgvt);
	this->lockSimulatorStep();
	// Clear processed messages with time < gvt
	auto iter = m_processed_messages.begin();
	for(; iter != m_processed_messages.end(); ++iter){
		if( (*iter)->getTimeStamp() > this->getGVT() ){
			break;
		}
	}
	m_processed_messages.erase(m_processed_messages.begin(), iter);		//erase[......GVT x)
	auto senditer = m_sent_messages.begin();
	for(; senditer != m_sent_messages.end(); ++senditer){
		if(  (*senditer)->getTimeStamp() > this->getGVT()  ){
			break;
		}
	}
	m_sent_messages.erase(m_sent_messages.begin(), senditer);
	this->unlockSimulatorStep();
}

void
n_model::Multicore::lockSimulatorStep(){
	LOG_DEBUG("MCORE:: trying to lock simulator core");
	this->m_locallock.lock();
	LOG_DEBUG("MCORE:: simulator core locked");
}

void
n_model::Multicore::unlockSimulatorStep(){
	LOG_DEBUG("MCORE:: trying to unlock simulator core");
	this->m_locallock.unlock();
	LOG_DEBUG("MCORE:: simulator core unlocked");
}

void
n_model::calculateGVT(/* access to cores,*/ std::size_t ms, std::atomic<bool>& run){
	while(run.load()==true){
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		/** Make control message
		 *  Get Core 0, call receiveControlMessage(msg);
		 *  Need to iterate twice over all cores, from within controller !!
		 */
	}
}


