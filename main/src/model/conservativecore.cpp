/*
 * conservativecore.cpp
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */

#include <thread>
#include <chrono>
#include "model/conservativecore.h"

namespace n_model {

Conservativecore::Conservativecore(const t_networkptr& n, std::size_t coreid, std::size_t totalCores,
	const t_eotvector& vc, const t_timevector& tc)
	: Core(coreid, totalCores),
	m_network(n),m_eit(t_timestamp(0, 0)), m_distributed_eot(vc),m_distributed_time(tc),m_min_lookahead(0u,0u),m_last_sent_msgtime(t_timestamp::infinity())
{
        /// Make sure our nulltime is set correctly
        m_distributed_time->lockEntry(this->getCoreID());
        m_distributed_time->set(this->getCoreID(), t_timestamp::infinity());
        m_distributed_time->unlockEntry(this->getCoreID());
}

void Conservativecore::getMessages()
{
	bool wasLive = isLive();
        this->setLive(true);
        if(!wasLive)
        	LOG_INFO("MCORE :: ", this->getCoreID(), " switching to live before we check for messages");
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("CCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
	if(messages.size()== 0 && ! wasLive){
		setLive(false);
		LOG_INFO("MCORE :: ", this->getCoreID(), " switching back to not live. No messages from network and we weren't live to begin with.");
	}
	this->sortIncoming(messages);
}

Conservativecore::~Conservativecore()
{
}

void Conservativecore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	for( auto i = messages.begin(); i != messages.end(); i++) {
		t_msgptr message = *i;
                validateUUID(message->getDstUUID());
		this->receiveMessage(message);
	}
}

t_timestamp Conservativecore::getEit()const {return m_eit;}

void
Conservativecore::setEit(const t_timestamp& neweit){
	this->m_eit = neweit;
}


void Conservativecore::updateEOT()
{       
        t_timestamp x_sent = t_timestamp::infinity();
	if(this->getLastMsgSentTime().getTime()==this->getTime().getTime()) {    
		x_sent = this->getTime()+t_timestamp::epsilon();        // Safe because imminent time will have been recorded before.
        }
        
        t_timestamp x_la(this->m_min_lookahead);
        t_timestamp nulltime(this->getNullTime());

        // In a cycle, overwrite lookahead iff we have bypassed the value. Only LA can be bypassed, so reuse the variable.
        // Note that lookahead by default is recalculated if <= nulltime, so only if we don't find a new LA we can increment time.
        if(!isInfinity(nulltime) && x_la.getTime()<= nulltime.getTime()){
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

        t_timestamp y_imminent = t_timestamp::infinity();
        if(!this->m_scheduler->empty()){       
                y_imminent = this->m_scheduler->top().getTime();
        }

        getMessages();
        t_timestamp y_pending = this->getFirstMessageTime();                // Message lock

	t_timestamp neweot(std::min({x_la, x_sent, y_imminent, y_pending}).getTime(),0);
        
#ifdef SAFETY_CHECKS
        if(isInfinity(neweot)){
                LOG_WARNING("CCORE:: ", this->getCoreID(), " time: ", getTime(), " EOT=inf."); // Only allowed if it nevers goes back to real values.
        }
#endif
        const t_timestamp oldeot(getEot());
        
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " updating eot from ", oldeot, " to ", neweot, " min of  x_la = ", x_la);
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " x_sent ", x_sent, " y_pending ", y_pending, " y_imminent ", y_imminent);
        
        if(!isInfinity(oldeot)  && oldeot.getTime() > neweot.getTime()){
                LOG_ERROR("CCORE:: ", this->getCoreID(), " time: ", getTime(), " eot moving backward in time, BUG. ::old= ", oldeot, "new= ",neweot);
                LOG_FLUSH;
                throw std::logic_error("EOT moving back in time.");
        }
        
        if(oldeot != neweot)
                setEot(neweot);
}

void Conservativecore::setEot(t_timestamp ntime){       // Fact that def is here matters not for inlining, TU where this is called is always this class only.
                m_distributed_eot->lockEntry(this->getCoreID());
                m_distributed_eot->set(this->getCoreID(), ntime);
                m_distributed_eot->unlockEntry(this->getCoreID());
}

/**
 * Step 4/5 of CNPDEVS.
 * a) For all messages from the network, for each core, get max timestamp &| eot value
 * b) From those values, get the minimum and set that value to our own EIT.
 * We don't need step a, this is already done by sendMessage/sharedVector, so we only need to collect the maxima
 * and update EIT with the min value.
 * Special cases to consider:
 *      no eot : eit=oo (ok)
 *      1 eot value @ oo, should not happen, still result is ok (eit=oo).
 * The algorithm never has to take into account messagetime, since eot reflects sending messages,
 * we only need to register the min of all maxima, and earliest output time reflects the earliest point in time
 * where the core will generate a new message (disregarding those in transit/pending completely).
 */
void Conservativecore::updateEIT()
{
	LOG_INFO("CCORE:: ", this->getCoreID(), " time: ", getTime(), " updating EIT:: eit_now = ", this->m_eit);
	t_timestamp min_eot_others = t_timestamp::infinity();
	for(const auto& influence_id : m_influencees){
		this->m_distributed_eot->lockEntry(influence_id);
		const t_timestamp new_eot = this->m_distributed_eot->get(influence_id);
		this->m_distributed_eot->unlockEntry(influence_id);
		min_eot_others = std::min(min_eot_others, new_eot);
	}
        const t_timestamp oldeot = this->getEit();
        
        LOG_INFO("Core:: ", this->getCoreID(), " setting EIT == ",  min_eot_others, " from ", oldeot);
	this->setEit(min_eot_others);
}

void Conservativecore::syncTime(){
	
        this->calculateMinLookahead();
	this->updateEOT();                     
	this->updateEIT();
	const t_timestamp nextfired(this->getFirstImminentTime());
        
        getMessages();          // We've been promised by eit/eot that everything <eot is on the net, pull again so we can't miss a msg.
	const t_timestamp firstmessagetime(this->getFirstMessageTime());
        
	t_timestamp newtime = std::min(firstmessagetime, nextfired);
        LOG_DEBUG("\tCORE :: ", this->getCoreID(),"@", this->getTime(), " New time is ", newtime, " =min( ", nextfired, " , ", firstmessagetime , ")");
        
	if (isInfinity(newtime)) {
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
                this->unlockSimulatorStep();
		throw std::runtime_error("Core time going backwards. ");
	}
#endif
	this->setTime(newtime);						
									
	this->resetZombieRounds();

	if (this->getTime() >= this->getTerminationTime()) {
		LOG_DEBUG("\tCORE :: ",this->getCoreID() ," Reached termination time :: now: ", this->getTime(), " >= ", this->getTerminationTime());
                this->m_distributed_eot->lockEntry(getCoreID());
                this->m_distributed_eot->set(this->getCoreID(), t_timestamp::infinity());
		//this->m_distributed_eot->set(this->getCoreID(), t_timestamp(this->getTime().getTime(), 0));
		this->m_distributed_eot->unlockEntry(getCoreID());
		this->setLive(false);
	}
}

void Conservativecore::setTime(const t_timestamp& newtime){
	/** Step 1/2 of algoritm:
	 * 	Advance state until time >= eit.
	 * 	A kernel works in rounds however, so we only enforce here that the
	 * 	kernel time never advances beyond eit.
	 */
        
	LOG_INFO("CCORE :: ", this->getCoreID(), " got request to forward time from ", this->getTime(), " to ", newtime);

	t_timestamp corrected = std::min(this->getEit(), newtime);

	LOG_INFO("CCORE :: ", this->getCoreID(), " corrected time ", corrected , " == min ( Eit = ", this->getEit(), ", ", newtime, " )");
       
        Core::setTime(corrected);
}

void Conservativecore::receiveMessage(t_msgptr msg){
        m_stats.logStat(MSGRCVD);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message \n", msg->toString());
        
#ifdef SAFETY_CHECKS
        if (msg->isAntiMessage()) 
                throw std::logic_error("Antimessage in conservativecore !!");
#endif
        
        this->queuePendingMessage(msg); // do not store antimessage
	
#ifdef SAFETY_CHECKS
        const t_timestamp::t_time currenttime= this->getTime().getTime();       // Avoid x times locked getter
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();

        if (msgtime < currenttime){
                LOG_INFO("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", currenttime,
		        " msg follows: ", msg->toString());
                m_stats.logStat(REVERTS);
                throw std::logic_error("Revert in conservativecore !!");
        }
        // The case == is safe for conservative due to stalling.
        // The case > is normal.
#endif
}

void Conservativecore::init(){
	/// Get first time, offset all models if required
	Core::init();

	/// Make sure we know who we're slaved to
	buildInfluenceeMap();

	/// Get first lookahead.
        this->calculateMinLookahead();
}


void Conservativecore::initExistingSimulation(const t_timestamp& loaddate){
	Core::initExistingSimulation(loaddate);
	buildInfluenceeMap();
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
                if(temp[i])                // Dependency on self is implied.
                        this->m_influencees.push_back(i);
	}
	
	for(const auto& coreid : m_influencees){
		LOG_INFO("CCORE :: ", this->getCoreID() , " influenced by " ,  coreid);
	}
}

void Conservativecore::resetLookahead(){
	this->m_min_lookahead = t_timestamp::infinity();
}

bool Conservativecore::timeStalled(){
        return (this->getTime().getTime()==this->getEit().getTime());
}

void Conservativecore::runSmallStep(){
        
        if(timeStalled() ){             // EIT==TIME
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT==TIME ");
                m_stats.logStat(STAT_TYPE::STALLEDROUNDS);
                this->runSmallStepStalled();
                if(checkNullRelease()){                 // If all influencing cores nulltime >= our nulltime, don't waste another round and immediately continue.                
                        Core::runSmallStep();   
                }else{                                  // At least one influencing core < our nulltime, wait, but update EOT/EIT to signal others.
                        updateEOT();            
                        updateEIT();            
                }
        }                               // EIT > TIME
        else{ // !stalled && released = fine, !stalled && !released =fine (limited by eit), stalled && released == fine
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT < TIME ");
                Core::runSmallStep();   
        }
}

void Conservativecore::collectOutput(std::vector<t_raw_atomic>& imminents){
        const t_timestamp::t_time outputtime = m_distributed_time->get(this->getCoreID()).getTime();
        if(outputtime == getTime().getTime()){
                return;         // If we've collected output before (in a stalled round usually), return immediately.
        }
        
                
        // Base function handles all the rest (message routing etc..)
        Core::collectOutput(imminents);
        // Next, we're stalled, but can be entering deadlock. Signal out current Time so the tiebreaker can
        // be found and break the lock.
        
        m_distributed_time->lockEntry(this->getCoreID());
        m_distributed_time->set(this->getCoreID(), getTime());
        m_distributed_time->unlockEntry(this->getCoreID());
        LOG_DEBUG("CCORE :: ", this->getCoreID(), " Null message time set @ :: ", this->getTime());
}

void Conservativecore::runSmallStepStalled()
{
        /**
         * Time == Eit. Generate output (once), then check if we can advance next time.
         * Broadcast null msg time to others to try to break the deadlock.
         */
        const t_timestamp::t_time outputtime = m_distributed_time->get(this->getCoreID()).getTime();
        if(outputtime != getTime().getTime()){
                std::vector<t_raw_atomic> imms;
                this->getImminent(imms);

                collectOutput(imms);            // after all output is sent, mark null msg time in m_distributed.
                for(auto mdl : imms){
                        const t_timestamp last_scheduled = mdl->getTimeLast() + mdl->timeAdvance();                
                        this->scheduleModel(mdl->getLocalID(), last_scheduled);
                }
                
        }
}

bool Conservativecore::checkNullRelease(){
        /**
         * If we find any influencing core with an output time (null msg time) not
         * equal to our own, we can't advance. (ret false)
         * If all nulltimes are equal, but our own isn't we need at least 1 stalled round, 
         * so again return false.
         */
        LOG_DEBUG("Core :: ", this->getCoreID(), " stalled round, checking if all influencing cores have advanced equally far ");
        t_timestamp::t_time current_time = this->getTime().getTime();
        t_timestamp::t_time own_null = this->m_distributed_time->get(this->getCoreID()).getTime();
        if(own_null != current_time ){
                LOG_DEBUG("Core :: ", this->getCoreID(), " stalled round, our own time is not yet set as nulltime, return false.");
                return false;
        }
        
        for(auto influencing : this->m_influencees){
                
                this->m_distributed_time->lockEntry(influencing);
                t_timestamp::t_time nulltime = this->m_distributed_time->get(influencing).getTime();
                this->m_distributed_time->unlockEntry(influencing);
                
                if(nulltime < current_time || isInfinity(t_timestamp(nulltime, 0))){
                        LOG_DEBUG("Core :: ", this->getCoreID(), " Null check failed for id = ", influencing, " at " ,nulltime);
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
Conservativecore::sendMessage(const t_msgptr& msg){
        // At output collection, timestamp is set (color etc is of no interest to us here (and is not yet set)).
        this->m_last_sent_msgtime = msg->getTimeStamp();
	LOG_DEBUG("\tCCORE :: ", this->getCoreID(), " sending message ", msg->toString());
	this->m_network->acceptMessage(msg);
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

} /* namespace n_model */
