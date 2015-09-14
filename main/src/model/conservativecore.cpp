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

Conservativecore::Conservativecore(const t_networkptr& n, std::size_t coreid,
        const n_control::t_location_tableptr& ltable, const t_eotvector& vc, const t_timevector& tc)
	: Core(coreid),
	m_network(n),m_eit(t_timestamp(0, 0)), m_distributed_eot(vc),m_distributed_time(tc),m_min_lookahead(0u,0u),m_last_sent_msgtime(t_timestamp::infinity()),m_loctable(ltable)
{
        /// Make sure our nulltime is set correctly
        m_distributed_time->lockEntry(this->getCoreID());
        m_distributed_time->set(this->getCoreID(), t_timestamp::infinity());
        m_distributed_time->unlockEntry(this->getCoreID());
}

void Conservativecore::getMessages()
{
	std::vector<t_msgptr> messages = this->m_network->getMessages(this->getCoreID());
	LOG_INFO("CCORE :: ", this->getCoreID(), " received ", messages.size(), " messages. ");
	if(messages.size()!= 0){
		if(this->isIdle()){
			this->setIdle(false);
			LOG_INFO("MCORE :: ", this->getCoreID(), " changing state from idle to non-idle since we have messages to process");
		}
	}
	this->sortIncoming(messages);
}

Conservativecore::~Conservativecore()
{
}

void Conservativecore::sortIncoming(const std::vector<t_msgptr>& messages)
{
	for( auto i = messages.begin(); i != messages.end(); i++) {
		const auto & message = *i;
                validateUUID(message->getDstUUID());
		this->receiveMessage(message);
	}
}

t_timestamp Conservativecore::getEit()const {return m_eit;}

void
Conservativecore::setEit(const t_timestamp& neweit){
	this->m_eit = neweit;
}

/** Step 3 of CNPDEVS
*     This assumes we're at eit, which we won't always be.
*     So for us, use min(time, eit) wherever alg uses eit.
* Pseudocode :
* 	EOT(myid) = std::min(x,y)
* 		x = eit + lookahead_min
* 		y = 	if(sent_message)	eit+eps // have to increment, else we get deadlock!
* 			else			top of scheduler (next event)
*                       // what if : not sent message, time = 50, msg waiting @70, next event = 90
*                       --> y = min(sent_msg, next_imminent, next_pendingmsg);
* 			none of the above : oo
*/
void Conservativecore::updateEOT()
{
        // Lookahead based
	t_timestamp x = t_timestamp::infinity();
        // If we've generated output, eot=time
        t_timestamp y_sent = t_timestamp::infinity();
        // Next possible event
	t_timestamp y_imminent = t_timestamp::infinity();
        t_timestamp y_pending = t_timestamp::infinity();
        
	if(! isInfinity(this->m_min_lookahead)){
		x = this->m_min_lookahead;
	}

        // Replace with nullmsgtime + eps
	if(this->getLastMsgSentTime().getTime()==this->getTime().getTime())
		y_sent = this->getTime()+t_timestamp::epsilon();
        
        if(!this->m_scheduler->empty())
                y_imminent = this->m_scheduler->top().getTime();
        
        y_pending = this->getFirstMessageTime();                // Message lock

	t_timestamp neweot(std::min({x, y_sent, y_imminent, y_pending}).getTime(),0);
        
        if(isInfinity(neweot)){
                // Can only happen if all x,y == inf, meaning we can never receive a message, and have nothing to do.
                // So eot cannot go backward later on from infinity to a real value. Nonetheless, for now log this event.
                LOG_WARNING("CCORE:: ", this->getCoreID(), " time: ", getTime(), " Idle core, setting eot to infinity !!!.");
        }
        
	this->m_distributed_eot->lockEntry(getCoreID());
        const t_timestamp oldeot = this->m_distributed_eot->get(this->getCoreID());
        if(!isInfinity(oldeot)  && oldeot > neweot){
                LOG_ERROR("CCORE:: ", this->getCoreID(), " time: ", getTime(), " eot moving backward in time, BUG.");
                // Don't remove braces, and don't throw unless you unlock first.
        }
	this->m_distributed_eot->set(this->getCoreID(), neweot);
	this->m_distributed_eot->unlockEntry(getCoreID());
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " updating eot from ", oldeot, " to ", neweot, " min of  x = ", x);
        LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " y_sent ", y_sent, " y_pending ", y_pending, " y_imminent ", y_imminent);
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
	/** The algorithm says : advance until next time >= eit
	 *  		-> then calculate EOT/EIT
	 *  We'll advance anyway, but EOT/EIT have to be recalculated more to avoid
	 *  slowing down. Case in point : EIT=oo, EOT=10, Time:10->20, EOT can never be behind current time.
	 *
	 */
        this->calculateMinLookahead();
	this->updateEOT();
	this->updateEIT();

	Core::syncTime();	// Multicore has no syncTime, explicitly invoke top base class.

	// If we don't reset the min lookahead, we'll get in a corrupt state very fast.
	//this->resetLookahead(); // No longer do this, x=inf edge case.

	// If we've terminated, our EOT should be our current time, not what we've calculated.
	// Else a dependent kernel can get hung up, since in Idle() state we'll never get here again.
	if(this->getTime()>=this->getTerminationTime()){        // isIdle is dangerous here.
		this->m_distributed_eot->lockEntry(getCoreID());
                this->m_distributed_eot->set(this->getCoreID(), t_timestamp::infinity());
		//this->m_distributed_eot->set(this->getCoreID(), t_timestamp(this->getTime().getTime(), 0));
		this->m_distributed_eot->unlockEntry(getCoreID());
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
        {
                std::lock_guard<std::mutex> lk(m_timelock);
                Core::setTime(corrected);
        }
}

void Conservativecore::receiveMessage(const t_msgptr& msg){
        m_stats.logStat(MSGRCVD);
	LOG_DEBUG("\tCORE :: ", this->getCoreID(), " receiving message \n", msg->toString());
        
        if (msg->isAntiMessage()) {
                m_stats.logStat(AMSGRCVD);
		LOG_DEBUG("\tCORE :: ", this->getCoreID(), " got antimessage, not queueing.");
		this->handleAntiMessage(msg);	// wipes message if it exists in pending, timestamp is checked later.
	} else {
		this->queuePendingMessage(msg); // do not store antimessage
	}
	
        const t_timestamp::t_time currenttime= this->getTime().getTime();       // Avoid x times locked getter
        const t_timestamp::t_time msgtime = msg->getTimeStamp().getTime();
        const t_timestamp::t_time eittime = this->getEit().getTime();

        if (msgtime < currenttime){
                LOG_INFO("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", currenttime,
		        " msg follows: ", msg->toString());
                m_stats.logStat(REVERTS);
                throw std::logic_error("Revert in conservativecore !!");
        }else{
                if (msgtime == currenttime && currenttime != eittime) {
                                LOG_INFO("\tCORE :: ", this->getCoreID(), " received message time <= than now : ", currenttime,
                                        " msg follows: ", msg->toString());
                                m_stats.logStat(REVERTS);
                                this->revert(msgtime);
                }
                // Stalled round && equal time is ok (have not yet transitioned)
                // msgtime > currenttime is always fine.
        }

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
	std::set<std::string>	influencees;
	for(const auto& model: m_indexed_models){
		model->addInfluencees(influencees);
	}
	for(const auto& modelname : influencees){
		std::size_t influencee_core = this->m_loctable->lookupModel(modelname);
                if(influencee_core != this->getCoreID())                // Dependency on self is implied.
                        this->m_influencees.insert(influencee_core);
	}
	
	for(const auto& coreid : m_influencees){
		LOG_INFO("CCORE :: ", this->getCoreID() , " influenced by " ,  coreid);
	}
}

void Conservativecore::invokeStallingBehaviour()
{
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
}

void Conservativecore::resetLookahead(){
	this->m_min_lookahead = t_timestamp::infinity();
}

bool Conservativecore::timeStalled(){
        return (this->getTime().getTime()==this->getEit().getTime());
}

void Conservativecore::runSmallStep(){
        
        /**
         * EIT == TIME : stall
         *      if this is the first time this happens
         *              generate output & mark nulltime as current ( iow runSmallStepStalled)
         *      else 
         *              // possible (but not guaranteed) deadlock
         *              if(all null message time >= our time)// we're safe to simulate further 
         *                      runSmallStep()
         *              else 
         *                      wait until the above becomes true : runSmallStepStalled (else eot /eit is not updated
         * TIME < EIT : normal round
         */
        if(timeStalled() && !checkNullRelease()){
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT==TIME ");
                m_stats.logStat(STAT_TYPE::STALLEDROUNDS);
                this->runSmallStepStalled();
                
                // RST updates eit, so recheck.
                if(timeStalled())       
                        invokeStallingBehaviour();      // Still stalled (not yet deadlocked), backoff.
        }
        else{   // 3 cases : 2x not stalled (fine), stalled&released, fine
                
                Core::runSmallStep();   //  L -> UL
        }
}

void Conservativecore::collectOutput(std::vector<t_atomicmodelptr>& imminents){
        // Two cases, either we have collected output already, in which case we need to remove the entry from the set
        // Or we haven't in which case the entry stays put, and we mark it for the next round.
        std::vector<t_atomicmodelptr> sortedimminents;
        for(const auto& imminent : imminents){
                const std::string& name = imminent->getName();
                auto found = m_generated_output_at.find(name);
                if(found != m_generated_output_at.end()){
                        // Have an entry, check timestamps (possible stale entries)
                        if(found->second.getTime()!=this->getTime().getTime()){
                                // Stale entry, need to collect output and mark model at current time.
                                sortedimminents.push_back(imminent);
                                m_generated_output_at[name]=this->getTime();
                        }else{
                                // Have entry at current time, leave it (and leave this comment, compiler will remove it.)
                                ;
                        }
                }
                else{
                        // No entry, so make one at current time.
                        sortedimminents.push_back(imminent);
                        m_generated_output_at[name]=this->getTime();
                }
        }
        // Base function handles all the rest (message routing etc..)
        Core::collectOutput(sortedimminents);
        // Next, we're stalled, but can be entering deadlock. Signal out current Time so the tiebreaker can
        // be found and break the lock.
        m_distributed_time->lockEntry(this->getCoreID());
        m_distributed_time->set(this->getCoreID(), this->getTime());
        m_distributed_time->unlockEntry(this->getCoreID());
        LOG_DEBUG("CCORE :: ", this->getCoreID(), " Null message time set @ :: ", this->getTime());
}

void Conservativecore::runSmallStepStalled()
{
        std::vector<t_atomicmodelptr> imms;
        this->getImminent(imms);
        
        collectOutput(imms); // only collects output once
        for(const auto& mdl : imms){
                const t_timestamp last_scheduled = mdl->getTimeLast() + mdl->timeAdvance();                
                this->scheduleModel(mdl->getLocalID(), last_scheduled);
        }
        //We have no new states since our last lookahead calculation, so
        //la values are unchanged.
        //Eot does require an update if we have sent ^^^ output.
        //Without updating eit, we'll never get out of a stalled round.
        //this->calculateMinLookahead(); // DO NOT ENABLE, unless debugging.
        this->updateEOT();
        this->updateEIT();
}

bool Conservativecore::checkNullRelease(){
        /**
         * If we find any influencing core with an output time (null msg time) not
         * equal to our own, we can't advance. (ret false)
         * If all nulltimes are equal, but our own isn't we need at least 1 stalled round, 
         * so again return false.
         */
        
        t_timestamp::t_time current_time = this->getTime().getTime();
        for(const auto& influencing : this->m_influencees){
                
                this->m_distributed_time->lockEntry(influencing);
                t_timestamp::t_time nulltime = this->m_distributed_time->get(influencing).getTime();
                this->m_distributed_time->unlockEntry(influencing);
                
                if(nulltime < current_time || isInfinity(t_timestamp(nulltime, 0))){
                        return false;
                }        
        }
        
        
        this->m_distributed_time->lockEntry(this->getCoreID());
        t_timestamp::t_time own_null = this->m_distributed_time->get(this->getCoreID()).getTime();
        this->m_distributed_time->unlockEntry(this->getCoreID());
        if(own_null != current_time)
                return false;
        
        return true;
        
}

bool n_model::Conservativecore::existTransientMessage(){
	bool b = this->m_network->networkHasMessages();
	LOG_DEBUG("CCORE:: ", this->getCoreID(), " time: ", getTime(), " network has messages ?=", b);
	return b;
}

void
Conservativecore::sendMessage(const t_msgptr& msg){
        // At output collection, timestamp is set (color etc is of no interest to us here (and is not yet set)).
        this->m_last_sent_msgtime = msg->getTimeStamp();
	LOG_DEBUG("\tCCORE :: ", this->getCoreID(), " sending message ", msg->toString());
	this->m_network->acceptMessage(msg);
}

t_timestamp
Conservativecore::getTime(){
        std::lock_guard<std::mutex> lk(m_timelock);
        return Core::getTime();
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
        if(this->m_min_lookahead.getTime() <= this->getTime().getTime() 
                && !isInfinity(this->m_min_lookahead)){
                m_min_lookahead = t_timestamp::infinity();
                for(const auto& model : m_indexed_models){
                        const t_timestamp la = model->lookAhead();
                        
                        if(isZero(la))
                                throw std::logic_error("Lookahead can't be zero");
                        
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
