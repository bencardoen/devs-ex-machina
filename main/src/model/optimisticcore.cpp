/*
 * Optimisticcore.cpp
 *
 *  Created on: 21 Mar 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */

#include <thread>

#include "model/optimisticcore.h"
#include "tools/objectfactory.h"

using namespace n_model;
using namespace n_network;

Optimisticcore::~Optimisticcore()
{
        // Destructors are run on main(), our pool is live but we can't access it anymore.
        // Do not delete ptrs here.
        for(auto& ptr : m_sent_messages){
                LOG_ERROR("MCORE:: ", this->getCoreID(), " HAVE ", ptr, " in  m_sent_messages @ destruction. This will result in an std::bad_alloc exception.");
                // We're back on main's thread, cannot call our pool.
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
        }
        m_sent_messages.clear();
        // Another edge case, if we quit simulating before getting all messages from the network, we leak memory if 
        // any of these is an antimessage.
        
        if(m_network->havePendingMessages(this->getCoreID())){
                LOG_ERROR("OCORE::", this->getCoreID(), " destructor detected messages in network for us, purging.");
                // Pull them in case another thread is waiting on network idle.
                std::vector<t_msgptr>msgs = m_network->getMessages(this->getCoreID());
        }
}
void Optimisticcore::shutDown()
{
        assert(!isLive() && "The core shouldn't be live when it is shut down!");
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " core shutting down, still need to remove ", m_sent_messages.size(), " messages");
        for(auto& ptr : m_sent_messages){
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " in core shutdown.");
                // We're not yet back on main's thread, can safely call our pool.
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
        }
        for (auto& ptr : m_sent_antimessages) {
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " in core shutdown.");
                // We're not yet back on main's thread, can safely call our pool.
                ptr->releaseMe();
                m_stats.logStat(DELMSG);
        }
        for(const auto& ptr: m_indexed_models) {
                ptr->clearSentMessages();
        }
        m_sent_messages.clear();
        m_sent_antimessages.clear();
}

Optimisticcore::Optimisticcore(const t_networkptr& net, std::size_t coreid, size_t cores)
	: Core(coreid, cores), m_network(net), m_color(MessageColor::WHITE), m_mcount_vector(cores), m_tred(
	        t_timestamp::infinity()),m_tmin(0u), m_removeGVTMessages(false)
{
}

void
Optimisticcore::clearProcessedMessages(std::vector<t_msgptr>& msgs){
#ifdef SAFETY_CHECKS
        if(msgs.size()==0)
                throw std::logic_error("Msgs not empty after processing ?");
#endif
        // In optimistic, delete only local-local messages after processing.
        for(t_msgptr& ptr : msgs){
                ptr->setFlag(Status::PROCESSED);
                if(ptr->getSourceCore()==this->getCoreID() && ptr->getDestinationCore()==this->getCoreID()){
                        m_stats.logStat(DELMSG);
                        LOG_DEBUG("MCORE:: ", this->getCoreID(),"@",this->getTime(), " deleting ", ptr, " = ", ptr->toString());
                        ptr->releaseMe();
                }
        }
        msgs.clear();
}

void Optimisticcore::sortMail(const std::vector<t_msgptr>& messages, std::size_t& msgCount)
{
        for (const auto& message : messages) {
                message->setCausality(++msgCount);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " sorting message ", message->toString());
                if (not this->isMessageLocal(message)) {
                        this->sendMessage(message);	// A noop for single core, multi core handles this.
                } else {
                        this->queueLocalMessage(message);
                }
        }
}

void Optimisticcore::sendMessage(t_msgptr msg)
{
        // We're locked on msglock. Don't change the ordering here.
        m_stats.logStat(MSGSENT);
        this->countMessage(msg);
        this->m_network->acceptMessage(msg);
        this->markMessageStored(msg);
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending message @",msg, " tostring: ", msg->toString() );
}

void Optimisticcore::sendAntiMessage(const t_msgptr& msg)
{
	// We're locked on msglock
        // size_t branch :: skip alloc of amsg entirely.
        m_stats.logStat(AMSGSENT);
	
        // Don't touch the color of the message.
	msg->setAntiMessage(true);
        
	LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending antimessage : ", msg->toString());
        // Skip this, by definition any antimessage is beyond the cut-line of gvt.
        // If you enable this, do so as well @receive side.
	//this->countMessage(amsg);					
	this->m_network->acceptMessage(msg);
	m_sent_antimessages.push_back(msg);
}

void Optimisticcore::handleAntiMessage(const t_msgptr& msg)
{
        // We're locked on msgs
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " entering handleAntiMessage with message ", msg);
        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " handling antimessage ", msg->toString());

        if(msg->flagIsSet(Status::PROCESSED)) {
                // processed before, must do revert (handled elsewhere)
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " message already processed, marking as KILL ", msg);
                msg->setFlag(Status::KILL);
        } else if(msg->flagIsSet(Status::HEAPED)) {
                // is currently in the scheduler, mark as to erase
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " message only in heap, marking as TOERASE ", msg);
                msg->setFlag(Status::TOERASE);
        } else if(msg->flagIsSet(Status::DELETE)) {
                // we encountered this one before, just kill it.
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " original msg found, marking as KILL ", msg);
                msg->setFlag(Status::KILL);
        } else {
                // we haven't seen this message yet, mark it as delete
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " first time we see this message, marking as DELETE ", msg);
                msg->setFlag(Status::DELETE);
        }
//        if (this->m_received_messages->contains(MessageEntry(msg))) {                   /// QUEUED
//                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " we apparently found msg ", msg,
//                        " in the following scheduler:");
//                m_received_messages->printScheduler();
//                this->m_received_messages->erase(MessageEntry(msg));
//                LOG_DEBUG("MCORE:: ", this->getCoreID(), " original msg found, deleting ", msg);
//                msg->releaseMe();
//
//                m_stats.logStat(DELMSG);
//        } else {                                          /// Not queued, so either never seen it, or allready processed
//
//                if (msg->flagIsSet(Status::PROCESSED)) {           // Processed before, only antimessage ptr in transit.
//                        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " Message is processed :: deleting ", msg);
//                        msg->releaseMe();
//                        m_stats.logStat(DELMSG);
//                        return;
//                }
//                if (!msg->flagIsSet(Status::DELETE)) {    // Possibly never seen, delete both.
//                        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " Special case : first pass:: ", msg);
//                        msg->setFlag(Status::DELETE);
//                } else {                           // Second time, delete.
//                        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " Special case : second pass, deleting.");
//                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", msg);
//                        msg->releaseMe();
//                        m_stats.logStat(DELMSG);
//                }
//        }
}

void Optimisticcore::markMessageStored(const t_msgptr& msg)
{
	LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " storing sent message ", msg, ": ", msg->toString());
#ifdef SAFETY_CHECKS
        if(msg->getSourceCore()!=this->getCoreID()){
                LOG_FLUSH;
                throw std::logic_error("Storing msg not sent from this core.");
        }
#endif
	this->m_sent_messages.push_back(msg);
}

void Optimisticcore::countMessage(const t_msgptr& msg)
{
        /**
         * ALGORITHM 1.4 (or Fujimoto page 121 send algorithm)
         * @pre Msgcolor == this.color
         */
        std::lock_guard<std::mutex> colorlock(m_colorlock);
        msg->paint(m_color);
#ifdef SAFETY_CHECKS
        // With the new lock in this function, only memory corruption or sender changing color of this message could
        // trigger this check. Nonetheless, if it does we have a case of nasal demons, and want to know about it before they're seen.
        if(msg->getColor()!=m_color){
                LOG_ERROR("Message color not equal to core color : ", msg->toString());
                LOG_FLUSH;
                throw std::logic_error("Msg color not equal to core color, race detected.");
        }
#endif
	if (m_color == MessageColor::WHITE) {           // getter is locked
		const size_t j = msg->getDestinationCore();
                {
                        std::lock_guard<std::mutex> lock(this->m_vlock);
                        const int val = ++(this->m_mcount_vector.getVector()[j]);
                        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: incrementing count vector to : ", val);
                }
	} else {
		// Locks this Tred because we might want to use it during the receiving
		// of a control message
		std::lock_guard<std::mutex> lock(this->m_tredlock);
                t_timestamp oldtred = m_tred;
		m_tred = std::min(m_tred, msg->getTimeStamp());
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: tred changed from : ", oldtred, " to ", m_tred, " after sending msg.");
	}
}

void Optimisticcore::receiveMessage(t_msgptr msg)
{
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();
        bool msgtime_in_past = false;
        if (msgtime < this->getTime().getTime()) {
                msgtime_in_past = true;
        }

        m_stats.logStat(MSGRCVD);

        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message @", msg);

        if (msg->isAntiMessage()) {
                m_stats.logStat(AMSGRCVD);
                LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got antimessage, not queueing.");
                this->handleAntiMessage(msg);
        } else {
                this->queuePendingMessage(msg);
                this->registerReceivedMessage(msg);
        }

        if (msgtime_in_past) {
                LOG_INFO("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", this->getTime());
                m_stats.logStat(REVERTS);
                this->revert(msgtime);
        }
}


void Optimisticcore::registerReceivedMessage(const t_msgptr& msg)
{
        // ALGORITHM 1.5 (or Fujimoto page 121 receive algorithm)
        // msg is not an antimessage here.
	if (msg->getColor() == MessageColor::WHITE) {
                std::lock_guard<std::mutex> lock(this->m_vlock);
                const int value_cnt_vector = --this->m_mcount_vector.getVector()[this->getCoreID()];
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: decrementing count vector to : ", value_cnt_vector);
	}
}

void Optimisticcore::runSmallStep()
{
        // Find out how many sent messages we have with time <= gvt
        if(m_removeGVTMessages) {
                auto senditer = m_sent_messages.begin();
                for (; senditer != m_sent_messages.end(); ++senditer) {
                        if ((*senditer)->getTimeStamp().getTime() >= this->getGVT().getTime()) {    // time value only ?
                                break;
                        }
                        t_msgptr& ptr = *senditer;
                        LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " = ", ptr->toString());
                        ptr->releaseMe();
                        m_stats.logStat(DELMSG);
#ifdef SAFETY_CHECKS
                        ptr = nullptr;
#endif
                }
                for (auto iter2 = senditer; iter2 != m_sent_messages.end();) {
                        t_msgptr ptr = *iter2;
                        if (ptr->flagIsSet(Status::KILL)) {
                                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " = ", ptr->toString());
                                ptr->releaseMe();
                                m_stats.logStat(DELMSG);
                                iter2 = m_sent_messages.erase(iter2);
                        } else {
                                ++iter2;
                        }
                }
                for (auto aiter = m_sent_antimessages.begin(); aiter != m_sent_antimessages.end();) {
                        t_msgptr ptr = *aiter;
                        if (ptr->flagIsSet(Status::KILL)) {
                                LOG_DEBUG("MCORE:: ", this->getCoreID(), " deleting ", ptr, " = ", ptr->toString());
                                ptr->releaseMe();
                                m_stats.logStat(DELMSG);
                                aiter = m_sent_antimessages.erase(aiter);
                        } else {
                                ++aiter;
                        }
                }

                LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " found ",
                        distance(m_sent_messages.begin(), senditer), " sent messages to erase.");

                m_sent_messages.erase(m_sent_messages.begin(), senditer);
                LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " sent messages now contains :: ",
                        m_sent_messages.size());
                m_removeGVTMessages = false;
        }

        Core::runSmallStep();
}

void Optimisticcore::getMessages()
{
	bool wasLive = isLive();
        this->setLive(true);
        if(!wasLive){
        	LOG_INFO("MCORE :: ", this->getCoreID(), " switching to live before we check for messages");
        }
        if(this->m_network->havePendingMessages(this->getCoreID())){
                std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
                LOG_INFO("CCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
                this->sortIncoming(messages);
        }else{
                if(! wasLive){
                        setLive(false);
                        LOG_INFO("MCORE :: ", this->getCoreID(), " switching back to not live. No messages from network and we weren't live to begin with.");
                }
	}
}

void Optimisticcore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	for( auto i = messages.begin(); i != messages.end(); i++) {
		const auto & message = *i;
#ifdef SAFETY_CHECKS
                validateUUID(uuid(message->getDestinationCore(), message->getDestinationModel()));
#endif
		this->receiveMessage(message);
	}
}

void Optimisticcore::waitUntilOK(const t_controlmsg& msg, std::atomic<bool>& rungvt)
{
	// We don't have to get the count from the message each time,
	// because the control message doesn't change, it stays in this core
	// The V vector of this core can change because ordinary message are
	// still being received by another thread in this core, this is why
	// we lock the V vector
	const int msgcount = msg->getCountVector()[this->getCoreID()];
	while (true) {

		if(rungvt == false){
			LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a Core thread, stopping GVT.");
			return;
		}
		int v_value = 0;
		{
			std::lock_guard<std::mutex> lock(m_vlock);
			v_value = this->m_mcount_vector.getVector()[this->getCoreID()];
		}
		if (v_value + msgcount <= 0){
			LOG_DEBUG("MCORE :: ", this->getCoreID(), " rungvt : V + C <=0; v= ", v_value, " C=",msgcount );
			break;
		}
		// We can't use a cvar here, if sim ends before we wake we hang the entire simulation.
		{
			std::chrono::milliseconds ms { 1 };
                        std::this_thread::sleep_for(ms);
		}
	}
}

void Optimisticcore::receiveControl(const t_controlmsg& msg, int round, std::atomic<bool>& rungvt)
{
// ALGORITHM 1.7 (more or less) (or Fujimoto page 121)
// Also see snapshot_gvt.pdf
        // Race check : id = const, read only.
	if(rungvt==false){
		LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a thread, stopping GVT.");
		return;
	}
	if (this->getCoreID() == 0 && round==0) {
		startGVTProcess(msg, round, rungvt);
	} else if (this->getCoreID() == 0) {            // Round 1,2,3...
                finalizeGVTRound(msg, round, rungvt);
	} else {
		receiveControlWorker(msg, round, rungvt);
	}
}

void
Optimisticcore::finalizeGVTRound(const t_controlmsg& msg, int round, std::atomic<bool>& rungvt){
        LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT : process init received control message, waiting for pending messages.");
        this->waitUntilOK(msg, rungvt);
        LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT : All messages received, checking vectors. ");
        if (msg->countIsZero()) {  /// Case found GVT
                LOG_INFO("MCORE :: ", this->getCoreID(), " found GVT!");
                t_timestamp GVT_approx = std::min(msg->getTmin(), msg->getTred());
                LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT approximation = min( ", msg->getTmin(), ",", msg->getTred(), ")");
                msg->setGvtFound(true);
                msg->setGvt(GVT_approx);
        } else {                /// C vector is != 0
                if(round == 1){ 
                        LOG_DEBUG("MCORE :: ", this->getCoreID(), " process init received control message, starting 2nd round");
                        msg->setTmin(this->getTMin());
                        msg->setTred(std::min(msg->getTred(), this->getTred()));
                        LOG_DEBUG("MCORE :: ", this->getCoreID(), " Starting 2nd round with tmin ", msg->getTmin(), " tred ", msg->getTred(), ")");
                        t_count& count = msg->getCountVector();
                        std::lock_guard<std::mutex> lock(this->m_vlock);
                        for (size_t i = 0; i < count.size(); ++i) {
                                count[i] += this->m_mcount_vector.getVector()[i];
                                this->m_mcount_vector.getVector()[i] = 0;
                        }
                }
                else{
                        LOG_DEBUG("MCORE :: ", this->getCoreID(), " 2nd round , P0 still has non-zero count vectors, quitting algorithm");
                }
                /// There is no cleanup to be done, startGVT initializes core/msg.
        }
}

void
Optimisticcore::receiveControlWorker(const t_controlmsg& msg, int /*round*/, std::atomic<bool>& rungvt)
{
        // ALGORITHM 1.6 (or Fujimoto page 121 control message receive algorithm)
        if (this->getColor() == MessageColor::WHITE) {
                this->setTred(t_timestamp::infinity());
                this->setColor(MessageColor::RED);
        }
        // We wait until we have received all messages
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " process received control message");
        waitUntilOK(msg, rungvt);

        // Equivalent to sending message, controlmessage is passed to next core.
        t_timestamp msg_tmin = msg->getTmin();
        t_timestamp msg_tred = msg->getTred();
        msg->setTmin(std::min(msg_tmin, this->getTMin()));
        msg->setTred(std::min(msg_tred, this->getTred()));
        LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " Updating tmin to ", msg->getTmin(), " tred = ", msg->getTred());
        t_count& Count = msg->getCountVector();
        std::lock_guard<std::mutex> lock(this->m_vlock);
        for (size_t i = 0; i < Count.size(); ++i) {
                Count[i] += this->m_mcount_vector.getVector()[i];
                this->m_mcount_vector.getVector()[i] = 0;
        }

        // Send message to next process in ring
        return;
}

void
Optimisticcore::startGVTProcess(const t_controlmsg& msg, int /*round*/, std::atomic<bool>& rungvt)
{
        if(rungvt==false){
		LOG_INFO("MCORE :: ", this->getCoreID(), " rungvt set to false by a thread, stopping GVT.");
		return;
	}
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " GVT received first control message, starting first round");
        this->setColor(MessageColor::RED);
        msg->setTmin(this->getTMin());
        msg->setTred(t_timestamp::infinity());
        LOG_INFO("MCORE :: ", this->getCoreID(), " Starting GVT Calculating with tred value of ", msg->getTred(), " tmin = ", msg->getTmin());
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
}

void Optimisticcore::setGVT(const t_timestamp& candidate)
{
        
	t_timestamp newgvt = t_timestamp(candidate.getTime(), 0);
#ifdef SAFETY_CHECKS
	if (newgvt.getTime() < this->getGVT().getTime() || isInfinity(newgvt) ) {          
		LOG_WARNING("Core:: ", this->getCoreID(), " cowardly refusing to set gvt to ", newgvt, " vs current : ",
		        this->getGVT());
                LOG_FLUSH;
		throw std::logic_error("Invalid GVT found");
	}
#endif
	
        this->lockSimulatorStep();              // implies msglock
        Core::setGVT(newgvt);           
	m_removeGVTMessages = true;

        for (const auto& model : this->m_indexed_models)
                model->setGVT(newgvt);
	// Reset state (note V-vector is reset by Mattern code.
        
	this->setColor(MessageColor::WHITE);
	LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " painted core back to white, for next gvt calculation");
        
	this->unlockSimulatorStep();
}

void n_model::Optimisticcore::lockSimulatorStep()
{
#ifdef LOG_LOCK
	LOG_DEBUG("MCORE :: ", this->getCoreID(), " trying to lock simulator core");
#endif
	this->m_locallock.lock();
#ifdef LOG_LOCK
	LOG_DEBUG("MCORE :: ", this->getCoreID(), "simulator core locked");
#endif
}

void n_model::Optimisticcore::unlockSimulatorStep()
{
#ifdef LOG_LOCK
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " trying to unlock simulator core.");
#endif
	this->m_locallock.unlock();
#ifdef LOG_LOCK
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " simulator core unlocked.");
#endif
}

void n_model::Optimisticcore::revert(const t_timestamp& totime)
{
	assert(totime.getTime() >= this->getGVT().getTime());
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting from ", this->getTime(), " to ", totime);
	if (!this->isLive()) {
		LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " Core going from idle to active ");
		this->setLive(true);
		this->setTerminatedByFunctor(false);
	}
	//		  vv Simlock		  vv Messagelock
	// Call chain :: singleStep->getMessages->sortIncoming -> receiveMessage() -> revert()

	LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting ", m_sent_messages.size(), " sent messages");
	while (!m_sent_messages.empty()) {		// For each message > totime, send antimessage
		auto msg = m_sent_messages.back();
		LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting message ", msg);
		if (msg->getTimeStamp() >= totime) {
			m_sent_messages.pop_back();
			LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " revert : sent message > time , antimessagging. \n ", msg->toString() );
			this->sendAntiMessage(msg);
		} else {
			LOG_DEBUG("MCORE:: ", this->getCoreID(), " message time < current time ", msg);
			break;
		}
	}
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " Done with reverting messages.");

	this->setTime(totime);
	this->rescheduleAllRevert(totime);		// Make sure the scheduler is reloaded with fresh/stale models
	this->revertTracerUntil(totime); 	// Finally, revert trace output
}


bool n_model::Optimisticcore::existTransientMessage(){
        bool b = this->m_network->empty();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " network is empty = ", b);
	return !b;
}

void
n_model::Optimisticcore::setColor(MessageColor mc){
	std::lock_guard<std::mutex> lock(m_colorlock);
	this->m_color = mc;
}

MessageColor
n_model::Optimisticcore::getColor(){
	std::lock_guard<std::mutex> lock(m_colorlock);
	return m_color;
}

/**
 * Locking strategy explained :
 *      get/set time has a very high call frequency
 *      sim thread : write/read on time
 *      gvt thread : read on time.
 *      So if we share tmin between them (which is what gvt uses time for), and update (locked)
 *      that value sim t updates time, we avoid the unnecessary lock on reading our own writes.
 */
void
n_model::Optimisticcore::setTime(const t_timestamp& newtime){
        this->setTMin(newtime);
	Core::setTime(newtime);
}

t_timestamp
n_model::Optimisticcore::getTred(){
	std::lock_guard<std::mutex> lock(m_tredlock);
	return this->m_tred;
}

void
n_model::Optimisticcore::setTred(t_timestamp val){
	std::lock_guard<std::mutex> lock(m_tredlock);
	this->m_tred = val;
}
