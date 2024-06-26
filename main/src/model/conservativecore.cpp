/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp 
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at 
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl 
 *      Author: Stijn Manhaeve, Ben Cardoen, Tim Tuijn
 */

#include <thread>
#include <string>
#include <chrono>
#include "model/conservativecore.h"
#include "scheduler/stlscheduler.h"
#include "network/timestamp.h"

namespace n_model {

Conservativecore::Conservativecore(const t_networkptr& n, std::size_t coreid, std::size_t totalCores,
	const t_eotvector& vc, const t_timevector& tc)
	: Core(coreid, totalCores),
	m_network(n),m_eit(0u), m_distributed_eot(vc),m_distributed_time(tc),m_min_lookahead(0u,0u),m_last_sent_msgtime(t_timestamp::infinity())
{
        if(totalCores==m_distributed_time->size())
                throw std::logic_error("NLTIME not aligned properly. !!");
        m_externalMessages.resize(totalCores);
}

void Conservativecore::getMessages()
{
        // Regardless of what happens, pull messages to avoid others stalling on network empty.
        if(this->m_network->havePendingMessages(this->getCoreID())){
                std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
                LOG_INFO("CCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
                if(isLive())
                        this->sortIncoming(messages);
        }
}

Conservativecore::~Conservativecore()
{
}

void Conservativecore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	for( auto i = messages.begin(); i != messages.end(); i++) {
		t_msgptr message = *i;
#ifdef SAFETY_CHECKS
                validateUUID(uuid(message->getDestinationCore(), message->getDestinationModel()));
#endif
		this->receiveMessage(message);
	}
}

t_timestamp::t_time Conservativecore::getEit()const {return m_eit;}

void
Conservativecore::setEit(const t_timestamp::t_time neweit){
	this->m_eit = neweit;
}


void Conservativecore::updateEOT()
{       
        t_timestamp x_sent = t_timestamp::infinity();
	if(this->getLastMsgSentTime().getTime()==this->getTime().getTime())
		x_sent = this->getTime()+t_timestamp::epsilon();        // Safe because imminent time will have been recorded before.
        
        t_timestamp x_la(this->m_min_lookahead);
        t_timestamp::t_time nulltime(this->getNullTime());

        // In a cycle, overwrite lookahead iff we have bypassed the value. Only LA can be bypassed, so reuse the variable.
        // Note that lookahead by default is recalculated if <= nulltime, so only if we don't find a new LA we can increment time.
        if(!isInfinity(t_timestamp(nulltime,0u)) && x_la.getTime()<= nulltime){
                if(checkNullRelease()){ // We can't always crawl ahead, only if influencing are at least as far, else you risk decreasing eot.
                        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), 
                        " Lookahead <= nulltime, starting CRAWLING mode, x_la== ", x_la, " null + eps ", nulltime +t_timestamp::epsilon());
                        x_la = nulltime+t_timestamp::epsilon();
                }else{
                        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), 
                        " Lookahead <= nulltime, Can't advance EOT since not all cores have advanced equally :: x_la = nulltime ", nulltime);
                        x_la = nulltime;
                }
        }
        t_timestamp y_imminent = getFirstImminentTime();

        getMessages();
        t_timestamp y_pending = this->getFirstMessageTime();                
	t_timestamp neweot(std::min({x_la, x_sent, y_imminent, y_pending}).getTime(),0);
        
#ifdef SAFETY_CHECKS
        if(isInfinity(neweot)){
                LOG_WARNING("CCORE:: ", this->getCoreID(), " time: ", getTime(), " EOT=inf."); // Only allowed if it nevers goes back to real values.
        }
#endif
        const t_timestamp oldeot(getEot());
        
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " updating eot from ", oldeot, " to ", neweot, " min of  x_la = ", x_la);
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " x_sent ", x_sent, " y_pending ", y_pending, " y_imminent ", y_imminent);
        
#ifdef SAFETY_CHECKS
        if(!isInfinity(oldeot)  && oldeot.getTime() > neweot.getTime()){
                LOG_ERROR("CCORE:: ", this->getCoreID(), " time: ", getTime(), " eot moving backward in time, BUG. ::old= ", oldeot, "new= ",neweot);
                LOG_FLUSH;
                throw std::logic_error("EOT moving back in time.");
        }
#endif        
        if(oldeot != neweot)
                setEot(neweot);
}

void Conservativecore::setEot(t_timestamp ntime){       // Fact that def is here matters not for inlining, TU where this is called is always this class only.
                m_distributed_eot->set(this->getCoreID(), ntime.getTime());
}


t_timestamp Conservativecore::getFirstMessageTime()
{
        constexpr t_timestamp mintime = t_timestamp::infinity();
        if(!m_received_messages->empty()){
                const MessageEntry& first = m_received_messages->top();
                return first.getMessage()->getTimeStamp();
        }
        LOG_DEBUG("\tCORE :: ", this->getCoreID(), " first message time == ", mintime);
        return mintime;
}


void Conservativecore::updateEIT()
{
	LOG_INFO("CCORE:: ", this->getCoreID(), " time: ", getTime(), " updating EIT:: eit_now = ", this->getEit());
	t_timestamp::t_time min_eot_others = t_timestamp::MAXTIME;
	for(size_t influence_id : m_influencees)
		min_eot_others = std::min(min_eot_others, this->m_distributed_eot->get(influence_id));
        
        LOG_INFO("Core:: ", this->getCoreID(), " setting EIT == ",  min_eot_others, " from ", this->getEit());
	this->setEit(min_eot_others);
}

void Conservativecore::syncTime(){
        // Lookahead is recalculated iff # transitions > 0
	this->updateEOT();                     
	this->updateEIT();
        
	const t_timestamp nextfired(this->getFirstImminentTime());
        getMessages();          // We've been promised by eit/eot that everything <eot is on the net, pull again so we can't miss a msg.
	const t_timestamp firstmessagetime(this->getFirstMessageTime());  
	t_timestamp newtime = std::min(firstmessagetime, nextfired);
        LOG_DEBUG("\tCORE :: ", this->getCoreID(),"@", this->getTime(), " New time is ", newtime, " =min( ", nextfired, " , ", firstmessagetime , ")");
        
	if (isInfinity(newtime)) {                      // No next event, try eot (crawling nulltime || la), which are equally valid in this case.
                const t_timestamp eot(this->getEot());
                if(isInfinity(eot)){
                        LOG_WARNING("\tCORE :: ", this->getCoreID(), " Core no imms, no msgs, eot=inf, time stuck == zombie.");
                        incrementZombieRounds();
                        return;
                }else{
                        LOG_WARNING("\tCORE :: ", this->getCoreID(), "Newtime == inf, using non-inf eot : ", eot); // Don't use in the general case!
                        newtime = eot;
                }
	}
#ifdef SAFETY_CHECKS
	if (this->getTime().getTime() > newtime.getTime()) {
		LOG_ERROR("\tCORE :: ", this->getCoreID() ," Synctime is setting time backward ?? now:", this->getTime(), " new time :", newtime);
		throw std::runtime_error("Core time going backwards. ");
	}
#endif
	this->setTime(newtime);										
	this->resetZombieRounds();

	if (this->getTime().getTime() >= this->getTerminationTime()) {
		LOG_DEBUG("\tCORE :: ",this->getCoreID() ," Reached termination time :: now: ", this->getTime(), " >= ", this->getTerminationTime());
                setEot(t_timestamp::MAXTIME);
                setNullTime((this->getTime()+t_timestamp::epsilon()).getTime());
		setLive(false);
	}
        
        this->updateDGVT();
        if(m_sent_messages.size()>GC_COLLECT_THRESHOLD)
                gcCollect();
}

void Conservativecore::setTime(const t_timestamp& newtime){
	LOG_INFO("CCORE :: ", this->getCoreID(), " got request to forward time from ", this->getTime(), " to ", newtime);

	t_timestamp corrected = std::min(t_timestamp(this->getEit(),0u), newtime);

	LOG_INFO("CCORE :: ", this->getCoreID(), " corrected time ", corrected , " == min ( Eit = ", this->getEit(), ", ", newtime, " )");
       
        Core::setTime(corrected);
}

void Conservativecore::receiveMessage(t_msgptr msg){
        m_stats.logStat(MSGRCVD);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message \n", msg->toString());        
        this->queuePendingMessage(msg);
	
#ifdef SAFETY_CHECKS
        const t_timestamp::t_time currenttime= this->getTime().getTime();
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();

        if (msgtime < currenttime){
                LOG_ERROR("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", currenttime,
		        " msg follows: ", msg->toString());
                m_stats.logStat(REVERTS);
                LOG_FLUSH;
                throw std::logic_error("Revert in conservativecore !!");
        }
        // The case == is safe for conservative due to stalling.
        // The case > is normal.
#endif
}

void Conservativecore::queuePendingMessage(t_msgptr msg){
        m_received_messages->push_back(MessageEntry(msg));
}

void Conservativecore::init(){
	/// Get first time, offset all models if required
	Core::init();

	/// Make sure we know who we're slaved to
	buildInfluenceeMap();

	/// Get first lookahead.
        this->calculateMinLookahead();
}

void Conservativecore::buildInfluenceeMap(){
	LOG_INFO("CCORE :: ", this->getCoreID(), " building influencee map.");
	std::vector<std::size_t> temp(m_cores, 0);

	for(const auto& model: m_indexed_models){
		model->addInfluencees(temp);
	}
	for(std::size_t i = 0; i < m_cores; ++i){
		LOG_INFO("CCORE :: ", getCoreID(), "temp vector[", i, "] = ", temp[i]);
		if(i == getCoreID())
			continue;
                if(temp[i])                
                        this->m_influencees.push_back(i);
	}
#if (LOG_LEVEL != 0)	
	for(const auto& coreid : m_influencees){
		LOG_INFO("CCORE :: ", this->getCoreID() , " influenced by " ,  coreid);
	}
#endif
}

bool Conservativecore::timeStalled(){
        return (this->getTime().getTime()==this->getEit());
}

void Conservativecore::signalTransition()
{
        calculateMinLookahead();
}

void Conservativecore::runSmallStep(){
        // Note : spinning on null release is ~20-30% more expensive than infrequently checking. @see git reverts in branch conservative
        if(timeStalled() ){             // EIT==TIME
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT==TIME ");
                m_stats.logStat(STAT_TYPE::STALLEDROUNDS);
                this->runSmallStepStalled();
                if(checkNullRelease()){                 // If all influencing cores nulltime >= our nulltime, don't waste another round and immediately continue.                
                        Core::runSmallStep();   
                }else{                                  // At least one influencing core < our nulltime, wait, but update EOT/EIT to signal others.
                        updateEOT();            
                        updateEIT();
                        // Don't yield, this is nearly always slower.
                }
        }                               // EIT > TIME
        else{ 
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT < TIME ");
                Core::runSmallStep();   
        }
}

void Conservativecore::collectOutput(std::vector<t_raw_atomic>& imminents){
        if(getNullTime() == getTime().getTime()){
                return;         // faster than remembering state. Don't collect output twice.
        }
                      
        Core::collectOutput(imminents);	//collects all output and sorts it locally/globally
        for(std::size_t i = 0; i < m_cores; ++i){
        	if(i == getCoreID()) continue;
        	std::vector<t_msgptr>& msgvec = m_externalMessages[i];
        	if(msgvec.size()){
        		//there's no point in requesting a lock on the network if we won't send anything
        		LOG_DEBUG("\tCCORE :: ", this->getCoreID(), " delivering ", msgvec.size(), " messages to core ", i);
        		m_network->giveMessages(i, msgvec);
        		msgvec.clear();
        	}
        }
        setNullTime(getTime().getTime());
        LOG_DEBUG("CCORE :: ", this->getCoreID(), " Null message time set @ :: ", this->getTime());
}

void Conservativecore::sortMail(const std::vector<t_msgptr>& messages)
{
        for(const auto& message : messages){
                message->setCausality(m_msgCurrentCount);
                m_msgCurrentCount = m_msgCurrentCount==m_msgEndCount? m_msgStartCount: (m_msgCurrentCount+1);

		LOG_DEBUG("\tCCORE :: ", this->getCoreID(), " sorting message ", message->toString());
		if (not this->isMessageLocal(message)) {
                        m_stats.logStat(MSGSENT);
			m_externalMessages[message->getDestinationCore()].push_back(message);
                        m_sent_messages.push_back(message);
			// At output collection, timestamp is set (the rest is of no interest to us here).
			this->m_last_sent_msgtime = message->getTimeStamp();
			LOG_DEBUG("\tCCORE :: ", this->getCoreID(), " queued message ", message->toString());
		} else {
			this->queueLocalMessage(message);
		}
	}
}

void Conservativecore::clearProcessedMessages(std::vector<t_msgptr>& msgs)
{
#ifdef SAFETY_CHECKS
        if(msgs.size()==0)
                throw std::logic_error("Msgs empty after processing ?");
#endif
        /// Msgs is a vector of processed msgs, stored in m_local_indexed_mail.
        for(t_msgptr ptr : msgs){
                if(ptr->getSourceCore()==this->getCoreID() && ptr->getDestinationCore()==this->getCoreID()){ 
                        LOG_DEBUG("CORE:: ", this->getCoreID(), " deleting ", ptr);
                        m_stats.logStat(DELMSG);
                        ptr->releaseMe();
                }
        }
        
        msgs.clear();
}


void Conservativecore::runSmallStepStalled()
{
        /**
         * Time == Eit. Generate output (once), then check if we can advance next time.
         * Broadcast null msg time to others to try to break the deadlock.
         */
        if(getNullTime() != getTime().getTime()){
                std::vector<t_raw_atomic> imms;
                this->getImminent(imms);

                collectOutput(imms);            // after all output is sent, mark null msg time in m_distributed.

                // Don't use same logic as in rstep. This function can be called several
                // times in succession, if you clear a flag here you irretrievably lose information.
                // Optionally, use run_once.
        }
}

bool Conservativecore::checkNullRelease(){
        LOG_DEBUG("Core :: ", this->getCoreID(), " stalled round, checking if all influencing cores have advanced equally far ");
        t_timestamp::t_time current_time = this->getTime().getTime();
        if(getNullTime() != current_time ){
                LOG_DEBUG("Core :: ", this->getCoreID(), " stalled round, our own time is not yet set as nulltime, return false.");
                return false;
        }
        
        for(auto influencing : this->m_influencees){
                t_timestamp::t_time influencing_nulltime = this->m_distributed_time->get(influencing);
                if(influencing_nulltime < current_time || isInfinity(t_timestamp(influencing_nulltime, 0))){
                        LOG_DEBUG("Core :: ", this->getCoreID(), " Null check failed for id = ", influencing, " at " ,influencing_nulltime);
                        return false;
                }        
        }
        LOG_DEBUG("Core :: ", this->getCoreID(), " Null check passed, all influencing cores are at time >= ourselves.");
        return true;       
}

bool n_model::Conservativecore::existTransientMessage(){
        return !m_network->empty();
}

void
Conservativecore::calculateMinLookahead(){
        /**
         * Determine (if needed) a new minimum lookahead value.
         * We need to ask all models for this, not only transitioned:
         * E : 0->70, 70->75, 75->120
         * D : 0->80, 80->90
         * Min LA = 70, 75, 80, 90, 120 (without all checked 80,90 would have been missed
         */
        if(this->m_min_lookahead.getTime() <= getTime().getTime() 
                && !isInfinity(this->m_min_lookahead)){
                m_min_lookahead = t_timestamp::infinity();
                for(const auto& model : m_indexed_models){
                        const t_timestamp la = model->lookAhead();
                        //LOG_DEBUG("Core :: ", this->getCoreID()," Model :: ", model->getName(), " gave LA = ", la);
#ifdef SAFETY_CHECKS
                        if(isZero(la))
                                throw std::logic_error("Lookahead can't be zero");
#endif                        
                        if(isInfinity(la))
                                continue;
                        
                        const t_timestamp last = model->getTimeLast();
                        m_min_lookahead = std::min(m_min_lookahead, (last+la));
                }
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " Lookahead updated to ", m_min_lookahead);
        }else{
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " Lookahead < time , skipping calculation. : ", m_min_lookahead);
        }
}

void
Conservativecore::getPendingMail()
{
         if(m_received_messages->empty()){
                return;
        }
	const t_timestamp nowtime = makeLatest(m_time);
	std::vector<MessageEntry> messages;
	m_token.getMessage()->setTimeStamp(nowtime);

	this->m_received_messages->unschedule_until(messages, m_token);
	
	for (const auto& entry : messages){
	        auto msg = entry.getMessage();
                queueLocalMessage(msg);
        }
}

void
Conservativecore::updateDGVT()
{       
        // Nullmessage time = time of output generation, not (yet) transition.
        // gvt = x-eps.
        // Values to ignore are : 0 (0-eps = underflow), oo (not yet active core).
        if(!this->getCoreID()){
                t_timestamp::t_time last = getDGVT();
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " DGVT calc: old = ", last);
                t_timestamp::t_time current_min = std::numeric_limits<t_timestamp::t_time>::max();              
                for(size_t i = 0;i<m_distributed_time->size()-1; ++i){
                        t_timestamp::t_time inulltime = m_distributed_time->get(i);
                        if(inulltime<= t_timestamp::epsilon() || (std::numeric_limits<t_timestamp::t_time>::max()==inulltime) ){
                                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " GVT calc: nulltime <= eps or oo for  ", i , " not going any further.");
                                return;
                        }else{
                                current_min = std::min(inulltime, current_min);
                        }        
                }
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " DGVT calc: new ==  ", current_min - t_timestamp::epsilon().getTime());
                setDGVT(current_min-t_timestamp::epsilon().getTime());
        }
}

void 
Conservativecore::gcCollect()
{
        /** GVT is point in time that all cores have passed. 
         *  So messages with timestamp <= gvt are safe to delete.
         */
        if(m_sent_messages.empty())
                return;
        t_timestamp::t_time dgvt = getDGVT();
        if(std::numeric_limits<t_timestamp::t_time>::max()==dgvt){
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " DGVT  ==  inf, skipping gccollect. ");
                return;
        }else
        {
                std::deque<t_msgptr>::iterator iter = m_sent_messages.begin();
                for(; iter != m_sent_messages.end(); ++iter){
                        t_msgptr mptr = *iter;// debugging
                        if(mptr->getTimeStamp().getTime()>= dgvt){
                                break;
                        }
                        else{
                                LOG_DEBUG("CORE:: ", this->getCoreID(), " deleting ", mptr);
                                m_stats.logStat(DELMSG);
                                mptr->releaseMe();
                        }
                }
                // Erase [begin, found)
                if(std::distance(m_sent_messages.begin(), iter)){
                        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " gccollect erasing ::  ", std::distance(m_sent_messages.begin(),iter));
                        m_sent_messages.erase(m_sent_messages.begin(), iter);
                }
        }
}



void
Conservativecore::shutDown()
{
        Core::shutDown(); // old tracing msgs.
        // @pre : for_each ptr : destination has processed the message. 
        for(t_msgptr ptr : m_sent_messages)
        {
                LOG_DEBUG("CCORE:: ", this->getCoreID(), " clearState:: deleting ", ptr);
                m_stats.logStat(DELMSG);
                // In future, move this into a ifdef safety checks.
                size_t destid = ptr->getDestinationCore();
                t_timestamp::t_time recv_time = m_distributed_time->get(destid);        
                if(recv_time <= ptr->getTimeStamp().getTime() && recv_time!=this->getTerminationTime().getTime()){
                        LOG_ERROR("Pointer : " , ptr, " with receiver id ", destid, " nulltime = ", recv_time, " msgtime = ", ptr->getTimeStamp());
                        LOG_FLUSH;
                }else{
                        ptr->releaseMe();
                }
        }
        m_sent_messages.clear(); // Should this function ever be called twice, make sure we don't actually delete things twice.
}

} /* namespace n_model */
