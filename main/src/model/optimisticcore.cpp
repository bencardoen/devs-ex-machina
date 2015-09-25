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
	this->m_network.reset();
        for(auto& ptr : m_sent_messages){
                delete ptr;
        }
        m_sent_messages.clear();
}

Optimisticcore::Optimisticcore(const t_networkptr& net, std::size_t coreid, size_t cores)
	: Core(coreid, cores), m_network(net), m_color(MessageColor::WHITE), m_mcount_vector(cores), m_tred(
	        t_timestamp::infinity()),m_zombie_rounds(0)
{
}

void
Optimisticcore::clearProcessedMessages(std::vector<t_msgptr>& msgs){
#ifdef SAFETY_CHECKS
        if(msgs.size()==0)
                throw std::logic_error("Msgs not empty after processing ?");
#endif
        /// Msgs is a vector of processed msgs, stored in m_local_indexed_mail.
        for(auto& ptr : msgs){
                if(ptr->getSourceCore()==this->getCoreID())
                        delete ptr;
#ifdef SAFETY_CHECKS
                ptr = nullptr;
#endif   
        
        }
        msgs.clear();
}

void Optimisticcore::sendMessage(const t_msgptr& msg)
{
	// We're locked on msglock.
	LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending message ", msg->toString());
        paintMessage(msg);
	this->m_network->acceptMessage(msg);
	this->markMessageStored(msg);
	this->countMessage(msg);					// Make sure Mattern is notified
}

void Optimisticcore::sendAntiMessage(const t_msgptr& msg)
{
	// We're locked on msglock
        // size_t branch :: skip alloc of amsg entirely.
        m_stats.logStat(AMSGSENT);
	
	this->paintMessage(msg);		
	msg->setAntiMessage(true);
        
	LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " sending antimessage : ", msg->toString());
        // Skip this, by definition any antimessage is beyond the cut-line of gvt.
        // If you enable this, do so as well @receive side.
	//this->countMessage(amsg);					
	this->m_network->acceptMessage(msg);
}

void Optimisticcore::paintMessage(const t_msgptr& msg)
{
	msg->paint(this->getColor());
}

void Optimisticcore::handleAntiMessage(const t_msgptr& msg, bool msgtimeinpast)
{
	// We're locked on msgs
	LOG_DEBUG("\tMCORE :: ",this->getCoreID()," handling antimessage ", msg->toString());
        
        if(this->m_received_messages->contains(msg)){   // Have received it before, but not processed it.
                LOG_DEBUG("\tMCORE :: ",this->getCoreID()," original found ", msg->toString(), " deleting ");
                this->m_received_messages->erase(MessageEntry(msg));
                delete msg;
        }else{                                          // Not yet received (origin) OR processed
                if(msgtimeinpast){                      // Processed, destroy am
                        LOG_DEBUG("\tMCORE :: ",this->getCoreID()," antimessage is for processed msg, deleting am");
                        delete msg;
                }else{                                  // Not processed, not received, meaning origin an antimessage at same time in network.
                        if(!msg->deleteFlagIsSet()){     // First time we see the pointer, remember.
                                LOG_DEBUG("\tMCORE :: ",this->getCoreID()," Special case : first pass.");
                                msg->setDeleteFlag();
                        }
                        else{                           // Second time, delete.
                                LOG_DEBUG("\tMCORE :: ",this->getCoreID()," Special case : second pass, deleting.");
                                delete msg;             
                        }
                }
        }
}

void Optimisticcore::markMessageStored(const t_msgptr& msg)
{
	LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " storing sent message", msg->toString());
	this->m_sent_messages.push_back(msg);
}

void Optimisticcore::countMessage(const t_msgptr& msg)
{
        /**
         * ALGORITHM 1.4 (or Fujimoto page 121 send algorithm)
         * this->getColor() should be equal to the msg color, however, the message can
         * be made slightly before the GVT code updates the color, resulting in a mismatch,
         * which will then fail the algorithm with negative values. (issue #1)
         * We can either force an update (locked), or just do as the algorithm says and use msgcolor.
         */
	if (msg->getColor() == MessageColor::WHITE) { // Don't use Core->color, alg specifies color = msgcolor here.
		size_t j = msg->getDestinationCore();
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
                LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: tred changed from : ", oldtred, " to ", m_tred, " after receiving msg.");
	}
}

void Optimisticcore::receiveMessage(const t_msgptr& msg)
{
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();
        bool msgtime_in_past = false;
        if(msgtime <= this->getTime().getTime()){       // <=, else you risk the edge case of extern/intern double transition instead of conf.
                msgtime_in_past=true;
        }
        
	m_stats.logStat(MSGRCVD);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message \n", msg->toString());
        
        if (msg->isAntiMessage()) {
                m_stats.logStat(AMSGRCVD);
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got antimessage, not queueing.");
		this->handleAntiMessage(msg, msgtime_in_past);
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
		{	// Raii, so that notified thread will not have to wait on lock.
			std::lock_guard<std::mutex> lock(this->m_vlock);
                        const int value_cnt_vector = --this->m_mcount_vector.getVector()[this->getCoreID()];
                        LOG_DEBUG("\tMCORE :: ", this->getCoreID(), " GVT :: decrementing count vector to : ", value_cnt_vector);
		}
		this->m_wake_on_msg.notify_all();
	}
}


void Optimisticcore::getMessages()
{
	// TODO : in principle, we could reduce threading issues by detecting live here (earlier) iso
	// after simulation.
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("MCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
	if(messages.size()!= 0){
		if(this->isIdle()){
			this->setIdle(false);
			LOG_INFO("MCORE :: ", this->getCoreID(), " changing state from idle to non-idle since we have messages to process");
		}
	}
	this->sortIncoming(messages);
}

void Optimisticcore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	// Locking could be done inside the for loop, but would make dissecting logs much more difficult.
	this->lockMessages();
	for( auto i = messages.begin(); i != messages.end(); i++) {
		const auto & message = *i;
                validateUUID(message->getDstUUID());
		this->receiveMessage(message);
	}
	this->unlockMessages();
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
		// Sleep in cvar, else we starve the sim thread.
		{
			std::unique_lock<std::mutex> cvunique(m_cvarlock);
			this->m_wake_on_msg.wait(cvunique);
		}
	}
}

void Optimisticcore::receiveControl(const t_controlmsg& msg, int round, std::atomic<bool>& rungvt)
{
// ALGORITHM 1.7 (more or less) (or Fujimoto page 121)
// Also see snapshot_gvt.pdf
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
                        msg->setTmin(this->getTime());
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
                        // We still can only get here iff V+C <= 0 for all cores (including this one)
                        // Either the algorithm failed with 2 colors, a race occurred, or a Core terminated.
                        // The benign cases (terminated && race with C<0 can be solved by enabling this workaroud.)
                        if(/*msg->countLeQZero()*/ false){
                                t_timestamp GVT_approx = std::min(msg->getTmin(), msg->getTred());
                                LOG_DEBUG("MCORE :: ", this->getCoreID(), " GVT approximation = min( ", msg->getTmin(), ",", msg->getTred(), ")");
                                // Put this info in the message
                                msg->setGvtFound(true);
                                msg->setGvt(GVT_approx);
                                LOG_DEBUG(" GVT Found with non-zero vector ");
                                msg->logMessageState(); 
                        }
                        else{   // Algorithm failure. Controller will log message state for us.
                                LOG_DEBUG("MCORE :: ", this->getCoreID(), " 2nd round , P0 still has non-zero count vectors, quitting algorithm");
                        }
                }
                /// There is no cleanup to be done, startGVT initializes core/msg.
        }
}

void
Optimisticcore::receiveControlWorker(const t_controlmsg& msg, int /*round*/, std::atomic<bool>& rungvt)
{
        // ALGORITHM 1.6 (or Fujimoto page 121 control message receive algorithm)
        if (this->getColor() == MessageColor::WHITE) {	// Locked
                // Probably not necessary, because messages can't be white during GVT calculation
                // when red messages are present, better safe than sorry though
                this->setTred(t_timestamp::infinity());
                this->setColor(MessageColor::RED);
        }
        // We wait until we have received all messages
        LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " process received control message");
        waitUntilOK(msg, rungvt);

        // Equivalent to sending message, controlmessage is passed to next core.
        t_timestamp msg_tmin = msg->getTmin();
        t_timestamp msg_tred = msg->getTred();
        msg->setTmin(std::min(msg_tmin, this->getTime()));
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
        msg->setTmin(this->getTime());
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
	Core::setGVT(newgvt);
	if (newgvt < this->getGVT() || isInfinity(newgvt)) {
		LOG_WARNING("Core:: ", this->getCoreID(), " cowardly refusing to set gvt to ", newgvt, " vs current : ",
		        this->getGVT());
		return;
	}
	// Have to erase msgs older than gvt, inform models, require Msglock, SimLock (model != synced)
	// Simlock => msglock.
	this->lockSimulatorStep();
	// Find out how many sent messages we have with time <= gvt
	auto senditer = m_sent_messages.begin();
	for (; senditer != m_sent_messages.end(); ++senditer) {      
		if ((*senditer)->getTimeStamp() > this->getGVT()) {
			break;
		}
                t_msgptr& ptr = *senditer;
                delete ptr;
#ifdef SAFETY_CHECKS
                ptr = nullptr;
#endif          
	}
	// Erase them.
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " found ", distance(m_sent_messages.begin(), senditer),
	        " sent messages to erase.");
        /// TODO delete sent messages from begin() to senditer (not incl)
	m_sent_messages.erase(m_sent_messages.begin(), senditer);
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " sent messages now contains :: ", m_sent_messages.size());
	// Update models
	for (const auto& model : this->m_indexed_models) {
		model->setGVT(newgvt);
	}
	// Reset state (note V-vector is reset by Mattern code.
	this->setColor(MessageColor::WHITE);
	LOG_INFO("MCORE:: ", this->getCoreID(), " time: ", getTime(), " painted core back to white, for next gvt calculation");
	this->unlockSimulatorStep();
}

void n_model::Optimisticcore::lockSimulatorStep()
{
	LOG_DEBUG("MCORE :: ", this->getCoreID(), " trying to lock simulator core");
	this->m_locallock.lock();
	LOG_DEBUG("MCORE :: ", this->getCoreID(), "simulator core locked");
}

void n_model::Optimisticcore::unlockSimulatorStep()
{
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " trying to unlock simulator core.");
	this->m_locallock.unlock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " simulator core unlocked.");
}

void n_model::Optimisticcore::lockMessages()
{
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " msgs locking ... ");
	m_msglock.lock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " msgs locked ");
}

void n_model::Optimisticcore::unlockMessages()
{
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " msg unlocking ...");
	m_msglock.unlock();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " msg unlocked");
}

void n_model::Optimisticcore::revert(const t_timestamp& totime)
{
        /// Ownership semantics : receiver is always responsible for delete
	assert(totime.getTime() >= this->getGVT().getTime());
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " reverting from ", this->getTime(), " to ", totime);
	if (this->isIdle()) {
		LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " Core going from idle to active ");
		this->setIdle(false);
		this->setLive(true);
		this->setTerminatedByFunctor(false);
	}
	//		  vv Simlock		  vv Messagelock
	// Call chain :: singleStep->getMessages->sortIncoming -> receiveMessage() -> revert()

	while (!m_sent_messages.empty()) {		// For each message > totime, send antimessage
		auto msg = m_sent_messages.back();
		if (msg->getTimeStamp() >= totime) {
			m_sent_messages.pop_back();
			LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " revert : sent message > time , antimessagging. \n ", msg->toString() );
			this->sendAntiMessage(msg);
		} else {
			break;
		}
	}
	this->setTime(totime);
	this->rescheduleAllRevert(totime);		// Make sure the scheduler is reloaded with fresh/stale models
	this->revertTracerUntil(totime); 	// Finally, revert trace output
}


bool n_model::Optimisticcore::existTransientMessage(){
	bool b = this->m_network->networkHasMessages();
	LOG_DEBUG("MCORE:: ", this->getCoreID(), " time: ", getTime(), " network has messages ?=", b);
	return b;
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

void
n_model::Optimisticcore::setTime(const t_timestamp& newtime){
	std::lock_guard<std::mutex> lock(m_timelock);
	Core::setTime(newtime);
}

t_timestamp
n_model::Optimisticcore::getTime(){
	std::lock_guard<std::mutex> lock(m_timelock);
	return Core::getTime();
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

void n_model::Optimisticcore::setTerminationTime(t_timestamp endtime)
{
	std::lock_guard<std::mutex> lock(m_timelock);
	Core::setTerminationTime(endtime);
}

n_network::t_timestamp n_model::Optimisticcore::getTerminationTime()
{
	std::lock_guard<std::mutex> lock(m_timelock);
	return Core::getTerminationTime();
}
