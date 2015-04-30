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
	LOG_DEBUG("MCore:: ", this->getCoreID(), " sending message ", msg->toString());
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
	LOG_DEBUG("MCore:: ", this->getCoreID(), " sending antimessage : ", amsg->toString());
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
		LOG_ERROR("MCore:: ", this->getCoreID(), " received antimessage without corresponding message",
		        msg->toString());
	}
}

void Multicore::markMessageStored(const t_msgptr& msg)
{
	LOG_DEBUG("MCore:: ", this->getCoreID(), " storing sent message", msg->toString());
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
	if (msg->getSourceCore() == this->getCoreID()){
		LOG_ERROR("MCore :: ", this->getCoreID(), " got message with Source == my id ?? ", msg->toString());
		return;
	}

	// ALGORITHM 1.5 (or Fujimoto page 121 receive algorithm)
	std::lock_guard<std::mutex> lock(this->m_vlock);
	if (msg->getColor() == MessageColor::WHITE) {
		this->m_mcount_vector.getVector()[this->getCoreID()] -= 1;
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
		if(this->containsModel(message->getDestinationModel())){
			this->receiveMessage(message);
		}else{
			LOG_ERROR("MCORE:: ", this->getCoreID(), " received message for model not in core\n", message->toString());
		}
	}
	this->unlockMessages();
}

void Multicore::waitUntilOK(const t_controlmsg& msg, std::atomic<bool>& rungvt)
{
	// We don't have to get the count from the message each time,
	// because the control message doesn't change, it stays in this core
	// The V vector of this core can change because ordinary message are
	// still being received by another thread in this core, this is why
	// we lock the V vector
	int msgcount = msg->getCountVector()[this->getCoreID()];
	while (true) {
		std::lock_guard<std::mutex> lock(m_vlock);
		if(rungvt == false){
			LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a Core thread, stopping GVT.");
			return;
		}
		int v_value = this->m_mcount_vector.getVector()[this->getCoreID()];
		if (v_value + msgcount <= 0){
			LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt : V + C <=0 ");
			break; // Lock is released, all white messages are received!
		}
	}
}

void Multicore::receiveControl(const t_controlmsg& msg, bool first, std::atomic<bool>& rungvt)
{	// TODO Beautify!!
// ALGORITHM 1.7 (more or less) (or Fujimoto page 121)
// Also see snapshot_gvt.pdf
	if(rungvt==false){
		LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a thread, stopping GVT.");
		return;
	}
	if (this->getCoreID() == 0 && first) {
		LOG_INFO("MCore:: ", this->getCoreID(), " GVT received first control message, starting first round");
		// If this processor is Pinit and is the first to be called in the GVT calculation
		// Might want to put this in a different function?
		this->setColor(MessageColor::RED);	// Locked
		this->m_tredlock.lock();
		this->m_tred = t_timestamp::infinity();
		this->m_tredlock.unlock();

		msg->setTmin(this->getTime());
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
		this->waitUntilOK(msg, rungvt);
		// If all items in count vectors are zero
		if (msg->countIsZero()) {
			LOG_INFO("MCore:: ", this->getCoreID(), " process init received control message, found GVT!");

			// We found GVT!
			t_timestamp GVT_approx = std::min(msg->getTmin(), msg->getTred());
			// Put this info in the message
			msg->setGvtFound(true);
			msg->setGvt(GVT_approx);
			// Stop
			return;
		} else {
			// if 3d round? exit?
			LOG_INFO("MCore:: ", this->getCoreID(),
			        " process init received control message, starting 2nd round");

			// We start a second round
			msg->setTmin(this->getTime());
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
		if (this->getColor() == MessageColor::WHITE) {	// Locked
			// Probably not necessary, because messages can't be white during GVT calculation
			// when red messages are present, better safe than sorry though
			this->m_tredlock.lock();
			this->m_tred = t_timestamp::infinity();	// LOCK
			this->m_tredlock.unlock();
			this->setColor(MessageColor::RED);
		}
		// We wait until we have received all messages
		waitUntilOK(msg, rungvt);

		LOG_INFO("MCore:: ", this->getCoreID(), "process received control message");

		// Equivalent to sending message, controlmessage is passed to next core.
		t_timestamp msg_tmin = msg->getTmin();
		t_timestamp msg_tred = msg->getTred();
		msg->setTmin(std::min(msg_tmin, this->getTime()));
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
		LOG_DEBUG("MCore : ", this->getCoreID(), " storing processed msg", msg->toString());
		this->m_processed_messages.push_back(msg);
	}
}

void Multicore::setGVT(const t_timestamp& newgvt)
{
	Core::setGVT(newgvt);
	if (newgvt < this->getGVT() or newgvt == t_timestamp::infinity()) {
		LOG_WARNING("Core:: ", this->getCoreID(), " cowardly refusing to set gvt to ", newgvt, " vs current : ",
		        this->getGVT());
		return;
	}
	this->lockSimulatorStep();
	// Clear processed messages with time < gvt
	// Message don't need a lock, simulator can't change
	auto iter = m_processed_messages.begin();
	for (; iter != m_processed_messages.end(); ++iter) {
		if ((*iter)->getTimeStamp() > this->getGVT()) {
			break;
		}
	}
	LOG_DEBUG("MCore:: ", this->getCoreID(), "setgvt found ", distance(m_processed_messages.begin(), iter),
	        " processed messages to erase.");
	m_processed_messages.erase(m_processed_messages.begin(), iter);		//erase[......GVT x)
	LOG_DEBUG("MCore:: ", this->getCoreID(), "processed messages now contains :: ", m_processed_messages.size());

	auto senditer = m_sent_messages.begin();
	for (; senditer != m_sent_messages.end(); ++senditer) {
		if ((*senditer)->getTimeStamp() > this->getGVT()) {
			break;
		}
	}

	LOG_DEBUG("MCore:: ", this->getCoreID(), " found ", distance(m_sent_messages.begin(), senditer),
	        " sent messages to erase.");
	m_sent_messages.erase(m_sent_messages.begin(), senditer);
	LOG_DEBUG("MCore:: ", this->getCoreID(), " sent messages now contains :: ", m_sent_messages.size());

	for (const auto& modelentry : this->m_models) {
		modelentry.second->setGVT(newgvt);
	}

	this->setColor(MessageColor::WHITE);
	LOG_INFO("Mcore:: ", this->getCoreID(), " painted core back to white, for next gvt calculation");
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
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "trying to unlock simulator core", this->getCoreID());
	this->m_locallock.unlock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "simulator core unlocked", this->getCoreID());
}

void n_model::Multicore::lockMessages()
{
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "sim msgs locking ... ", this->getCoreID());
	m_msglock.lock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "sim msgs locked ", this->getCoreID());
}

void n_model::Multicore::unlockMessages()
{
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "sim msg unlocking ...");
	m_msglock.unlock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "sim msg unlocked");
}

void n_model::Multicore::revert(const t_timestamp& totime)
{
	assert(totime >= this->getGVT());
	LOG_DEBUG("MCORE:: ", this->getCoreID(), "reverting from ", this->getTime(), " to ", totime);
	if (this->isIdle()) {
		LOG_DEBUG("MCORE:: ", this->getCoreID(), " Core going from idle to active ");
		this->setIdle(false);
		this->setLive(true);
		this->setTerminated(false);	// May or may not be true, but needs to be reavaluated by simStep.
	}
	// We have the simulator lock
	// DO NOT lock on msgs, we're called by receive message, which is locked !!
	while (!m_processed_messages.empty()) {
		auto msg = m_processed_messages.back();
		if (msg->getTimeStamp() > totime) {
			LOG_DEBUG("MCORE:: ", this->getCoreID(), " repushing processed message ", msg->toString());
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
			LOG_DEBUG("MCORE:: ", this->getCoreID(), " popping sent message ", msg->toString());
			this->sendAntiMessage(msg);
		} else {
			break;
		}
	}
	this->setTime(totime);
	this->rescheduleAll(totime);		// Make sure the scheduler is reloaded with fresh/stale models
	this->revertTracerUntil(totime); 	// Finally, revert trace output
}


bool n_model::Multicore::existTransientMessage(){
	bool b = this->m_network->networkHasMessages();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " network has messages ?=", b);
	return b;
}

void
n_model::Multicore::setColor(MessageColor mc){
	std::lock_guard<std::mutex> lock(m_colorlock);
	this->m_color = mc;
}

MessageColor
n_model::Multicore::getColor(){
	std::lock_guard<std::mutex> lock(m_colorlock);
	return m_color;
}

void
n_model::Multicore::setTime(const t_timestamp& newtime){
	std::lock_guard<std::mutex> lock(m_timelock);
	Core::setTime(newtime);
}

t_timestamp
n_model::Multicore::getTime(){
	std::lock_guard<std::mutex> lock(m_timelock);
	return Core::getTime();
}
