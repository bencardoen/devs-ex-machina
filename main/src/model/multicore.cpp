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

Multicore::~Multicore()
{
	this->m_loctable.reset();
	this->m_network.reset();
	// this->m_received_messages member of Core.cpp, don't touch.
	this->m_sent_messages.clear();
	this->m_processed_messages.clear();
}

Multicore::Multicore(const t_networkptr& net, std::size_t coreid, const t_location_tableptr& ltable, size_t cores)
	: Core(coreid), m_network(net), m_loctable(ltable), m_color(MessageColor::WHITE), m_mcount_vector(cores), m_tred(
	        t_timestamp::infinity())
{
}

void Multicore::sendMessage(const t_msgptr& msg)
{
	// We're locked on msglock.
	size_t coreid = this->m_loctable->lookupModel(msg->getDestinationModel());
	msg->setDestinationCore(coreid);	// time, color, source are set by collectOutput(). Rest is set by model.
	LOG_DEBUG("MCore:: sending message ", msg->toString());
	this->m_network->acceptMessage(msg);
	this->markMessageStored(msg);
	this->countMessage(msg);					// Make sure Mattern is notified
}

void Multicore::sendAntiMessage(const t_msgptr& msg)
{
	// We're locked on msglock
	t_msgptr amsg = n_tools::createObject<Message>(msg->getDestinationModel(), msg->getTimeStamp(),
	        msg->getDestinationPort(), msg->getSourcePort(), msg->getPayload());// Use explicit copy accessors to void any chance for races.
	// TODO Race : it's still possible a model uses msg->Payload while we copy it. Don't use it ? (it's not in hash).
	this->paintMessage(amsg);		// Antimessage should have same color as core!!
	amsg->setAntiMessage(true);
	amsg->setDestinationCore(msg->getDestinationCore());
	amsg->setSourceCore(msg->getSourceCore());
	LOG_DEBUG("MCore:: sending antimessage : ", amsg->toString());
	this->m_network->acceptMessage(amsg);
	this->countMessage(amsg);					// Make sure Mattern is notified
}

void Multicore::paintMessage(const t_msgptr& msg)
{
	msg->paint(this->getColor());
}

void Multicore::handleAntiMessage(const t_msgptr& msg)
{
	// We're locked on msgs
	LOG_DEBUG("MCore:: handling antimessage ", msg->toString());
	if (this->m_received_messages->contains(MessageEntry(msg))) {
		this->m_received_messages->erase(MessageEntry(msg));
	} else {
		LOG_ERROR("MCore:: received antimessage without corresponding message", msg->toString());
	}
}

void Multicore::markMessageStored(const t_msgptr& msg)
{
	LOG_DEBUG("MCore:: storing sent message", msg->toString());
	this->m_sent_messages.push_back(msg);
}

void Multicore::countMessage(const t_msgptr& msg)
{	// ALGORITHM 1.4 (or Fujimoto page 121 send algorithm)
	std::lock_guard<std::mutex> lock(this->m_vlock);
	if (this->getColor() == MessageColor::WHITE) { // Or use the color from the message (should be the same!)
		size_t j = msg->getDestinationCore();
		this->m_mcount_vector.getVector()[j] += 1;
	} else {
		// Locks this Tred because we might want to use it during the receiving
		// of a control message
		std::lock_guard<std::mutex> lock(this->m_tredlock);
		m_tred = std::min(m_tred, msg->getTimeStamp());
	}
}

void Multicore::receiveMessage(const t_msgptr& msg)
{
	Core::receiveMessage(msg);	// Trigger antimessage handling etc (and possibly revert)

	// If we receive a message from our own, we don't want that to be counted in the V vector
	if (msg->getDestinationCore() == this->getCoreID())
		return;

	// ALGORITHM 1.5 (or Fujimoto page 121 receive algorithm)
	std::lock_guard<std::mutex> lock(this->m_vlock);
	if (msg->getColor() == MessageColor::WHITE) {
		this->m_mcount_vector.getVector()[this->getCoreID()] -= 1; // Fails if core id is not properly set by controller.
	}
}

void Multicore::getMessages()
{
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("MCore id:", this->getCoreID(), " received ", messages.size(), " messages ");
	this->sortIncoming(messages);
}

void Multicore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	this->lockMessages();
	for (const auto & message : messages) {
		assert(message->getDestinationCore() == this->getCoreID());
		this->receiveMessage(message);
	}
	this->unlockMessages();
}

void Multicore::waitUntilOK(const t_controlmsg& msg)
{
	// We don't have to get the count from the message each time,
	// because the control message doesn't change, it stays in this core
	// The V vector of this core can change because ordinary message are
	// still being received by another thread in this core, this is why
	// we lock the V vector
	int msgcount = msg->getCountVector()[this->getCoreID()];
	while (true) {
		std::lock_guard<std::mutex> lock(m_vlock);
		if (this->m_mcount_vector.getVector()[this->getCoreID()] + msgcount <= 0)
			break; // Lock is released, all white messages are received!
	}
}

void Multicore::receiveControl(const t_controlmsg& msg, bool first)
{	// TODO Beautify!!
// ALGORITHM 1.7 (more or less) (or Fujimoto page 121)
// Also see snapshot_gvt.pdf

	if (this->getCoreID() == 0 && first) {
		LOG_INFO("MCore:: received first control message, starting first round");

		// If this processor is Pinit and is the first to be called in the GVT calculation
		// Might want to put this in a different function?
		this->m_color = MessageColor::RED;
		this->m_tredlock.lock();
		this->m_tred = t_timestamp::infinity();
		this->m_tredlock.unlock();

		msg->setTmin(this->getFirstMessageTime());
		msg->setTred(t_timestamp::infinity());

		t_count& count = msg->getCountVector();
		// We want to make sure our count vector starts with 0
		std::fill(count.begin(), count.end(), 0);
		// Lock because we will change our V vector
		std::lock_guard<std::mutex> lock(this->m_vlock);
		for (size_t i = 0; i < count.size(); i++) {
			count[i] += this->m_mcount_vector.getVector()[i];
			this->m_mcount_vector.getVector()[i] = 0;
		}

		// Send message to next process in ring
		return;

	} else if (this->getCoreID() == 0) {
		// Else if Pinit gets the control message back after a complete first round
		// Pinit must be red! Pinit will start a second round if necessary (if count != 0)
		// Note that Msg_count and Msg_tred must be accumulated over both rounds, whereas
		// Msg_tmin is calculated individually for each round. If initiator gets back control
		// message after second round, Msg_count is guaranteed to be zero vector and GVT
		// approximation is found.

		// Wait until we have received all messages
		this->waitUntilOK(msg);
		t_count& count = msg->getCountVector();
		// If all items in count vectors are zero
		if (msg->countIsZero()) {
			LOG_INFO("MCore:: process init received control message, found GVT!");

			// We found GVT!
			t_timestamp GVT_approx = std::min(msg->getTmin(), msg->getTred());
			// Put this info in the message
			msg->setGvtFound(true);
			msg->setGvt(GVT_approx);
			// Stop
			return;
		} else {
			// if 3d round? exit?
			LOG_INFO("MCore:: process init received control message, starting 2nd round");

			// We start a second round
			msg->setTmin(this->getFirstMessageTime());
			this->m_tredlock.lock();
			msg->setTred(std::min(msg->getTred(), this->m_tred));
			this->m_tredlock.unlock();
			t_count& count = msg->getCountVector();
			// Lock because we will change our V vector
			std::lock_guard<std::mutex> lock(this->m_vlock);
			for (size_t i = 0; i < count.size(); ++i) {
				count[i] += this->m_mcount_vector.getVector()[i];
				this->m_mcount_vector.getVector()[i] = 0;
			}

			// Send message to next process in ring
			return;
		}

	} else {
		// ALGORITHM 1.6 (or Fujimoto page 121 control message receive algorithm)
		if (this->getColor() == MessageColor::WHITE) {
			// Probably not necessary, because messages can't be white during GVT calculation
			// when red messages are present, better safe than sorry though
			this->m_tredlock.lock();
			this->m_tred = t_timestamp::infinity();	// LOCK
			this->m_tredlock.unlock();
			this->m_color = MessageColor::RED;	// LOCK
		}
		// We wait until we have received all messages
		waitUntilOK(msg);

		LOG_INFO("MCore:: process received control message");

		// Equivalent to sending message, controlmessage is passed to next core.
		t_timestamp msg_tmin = msg->getTmin();
		t_timestamp msg_tred = msg->getTred();
		msg->setTmin(std::min(msg_tmin, this->getFirstMessageTime()));
		this->m_tredlock.lock();
		msg->setTred(std::min(msg_tred, this->m_tred));
		this->m_tredlock.unlock();
		t_count& Count = msg->getCountVector();
		// Lock because we will change our V vector
		std::lock_guard<std::mutex> lock(this->m_vlock);
		for (size_t i = 0; i < Count.size(); ++i) {
			Count[i] += this->m_mcount_vector.getVector()[i];
			this->m_mcount_vector.getVector()[i] = 0;
		}

		// Send message to next process in ring
		return;
	}
}

void Multicore::markProcessed(const std::vector<t_msgptr>& messages)
{
	for (const auto& msg : messages) {
		LOG_DEBUG("MCore : storing processed msg", msg->toString());
		this->m_processed_messages.push_back(msg);
	}
}

void Multicore::setGVT(const t_timestamp& newgvt)
{
	Core::setGVT(newgvt);
	this->lockSimulatorStep();
	// Clear processed messages with time < gvt
	this->lockMessages();
	auto iter = m_processed_messages.begin();
	for (; iter != m_processed_messages.end(); ++iter) {
		if ((*iter)->getTimeStamp() > this->getGVT()) {
			break;
		}
	}
	LOG_DEBUG("MCore:: setgvt found ", distance(m_processed_messages.begin(), iter),
	        " processed messages to erase.");
	m_processed_messages.erase(m_processed_messages.begin(), iter);		//erase[......GVT x)
	LOG_DEBUG("MCore:: processed messages now contains :: ", m_processed_messages.size());

	auto senditer = m_sent_messages.begin();
	for (; senditer != m_sent_messages.end(); ++senditer) {
		if ((*senditer)->getTimeStamp() > this->getGVT()) {
			break;
		}
	}

	LOG_DEBUG("MCore:: found ", distance(m_sent_messages.begin(), senditer), " sent messages to erase.");
	m_sent_messages.erase(m_sent_messages.begin(), senditer);
	LOG_DEBUG("MCore:: processed sent messages now contains :: ", m_sent_messages.size());

	for(const auto& modelentry : this->m_models){
		modelentry.second->setGVT(newgvt);
	}

	this->m_color = MessageColor::WHITE;
	LOG_INFO("Mcore:: painted core back to white, for next gvt calculation");
	this->unlockMessages();
	this->unlockSimulatorStep();
}

void n_model::Multicore::lockSimulatorStep()
{
	LOG_DEBUG("MCORE:: trying to lock simulator core");
	this->m_locallock.lock();
	LOG_DEBUG("MCORE:: simulator core locked");
}

void n_model::Multicore::unlockSimulatorStep()
{
	LOG_DEBUG("MCORE:: trying to unlock simulator core", this->getCoreID());
	this->m_locallock.unlock();
	LOG_DEBUG("MCORE:: simulator core unlocked", this->getCoreID());
}

void n_model::Multicore::lockMessages()
{
	LOG_DEBUG("MCORE:: sim msgs locking ... ", this->getCoreID());
	m_msglock.lock();
	LOG_DEBUG("MCORE:: sim msgs locked ", this->getCoreID());
}

void n_model::Multicore::unlockMessages()
{
	LOG_DEBUG("MCORE:: sim msg unlocking ...");
	m_msglock.unlock();
	LOG_DEBUG("MCORE:: sim msg unlocked");
}

void n_model::Multicore::revert(const t_timestamp& totime)
{
	assert(totime >= this->getGVT());
	LOG_DEBUG("MCORE:: reverting from ", this->getTime(), " to ", totime);
	// We have the simulator lock
	// DO NOT lock on msgs, we're called by receive message, which is locked !!
	while (!m_processed_messages.empty()) {
		auto msg = m_processed_messages.back();
		if (msg->getTimeStamp() > totime) {
			LOG_DEBUG("MCORE:: repushing processed message ", msg->toString());
			m_processed_messages.pop_back();
			this->queuePendingMessage(msg);
		} else {
			break;
		}
	}
	// All send messages time < totime, delete (antimessage).
	while (!m_sent_messages.empty()) {
		auto msg = m_sent_messages.back();
		if (msg->getTimeStamp() > totime) {
			m_sent_messages.pop_back();
			LOG_DEBUG("MCORE:: popping sent message ", msg->toString());
			this->sendAntiMessage(msg);
		} else {
			break;
		}
	}
	this->setTime(totime);
	this->rescheduleAll(totime);		// Make sure the scheduler is reloaded with fresh/stale models
	this->revertTracerUntil(totime); 	// Finally, revert trace output
}

void n_model::calculateGVT(/* access to cores,*/std::size_t ms, std::atomic<bool>& run)
{
	while (run.load() == true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		/** Make control message
		 *  Get Core 0, call receiveControlMessage(msg);
		 *  Need to iterate twice over all cores, from within controller !!
		 */
	}
}
