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
        const n_control::t_location_tableptr& ltable, size_t cores, const t_eotvector& vc, const t_timevector& tc)
	: Multicore(n, coreid, ltable, cores), /*Forward entire parampack.*/
	m_eit(t_timestamp(0, 0)), m_distributed_eot(vc),m_distributed_time(tc),m_min_lookahead(t_timestamp::infinity()),m_last_sent_msgtime(t_timestamp::infinity())
{
	;
}

Conservativecore::~Conservativecore()
{
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
* 		y = 	if(sent_message)	eit,1
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
		x=this->getTime() + this->m_min_lookahead;	//  this can still overflow, but then we're dead anyway.
	}

	if(this->getLastMsgSentTime().getTime()==this->getTime().getTime())
		y_sent=this->getTime();
        
        if(!this->m_scheduler->empty())
                y_imminent = this->m_scheduler->top().getTime();
        
        y_pending = this->getFirstMessageTime();                // Message lock

	const t_timestamp neweot(std::min({x, y_sent, y_imminent, y_pending}).getTime(),0);
        
	this->m_distributed_eot->lockEntry(getCoreID());
        const t_timestamp oldeot = this->m_distributed_eot->get(this->getCoreID());
        if(!isInfinity(oldeot)  && oldeot > neweot){
                LOG_ERROR("CCORE:: ", this->getCoreID(), " time: ", getTime(), " eot moving backward in time, BUG.");
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
	this->updateEOT();
	this->updateEIT();

	Core::syncTime();	// Multicore has no syncTime, explicitly invoke top base class.

	// If we don't reset the min lookahead, we'll get in a corrupt state very fast.
	this->resetLookahead();

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

	// Core::setTime is not locked, which is an issue if we run GVT async.
	// vv is a synchronized setter.
	Multicore::setTime(corrected);
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
                this->revert(msgtime);
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

        // For now, make sure GVT keeps working.
	Multicore::registerReceivedMessage(msg);
}

void Conservativecore::init(){
	/// Get first time, offset all models if required
	Core::init();

	/// Make sure we know who we're slaved to
	buildInfluenceeMap();

	/// Get first lookahead.
	for(const auto& model : m_models){
		t_timestamp current_min = this->m_min_lookahead;
		t_timestamp model_la = model.second->lookAhead();
		this->m_min_lookahead = std::min(current_min, model_la);
		LOG_DEBUG("CCORE :: ", this->getCoreID(), " updating lookahead from " , current_min, " to ", this->m_min_lookahead);
	}
}


void Conservativecore::initExistingSimulation(const t_timestamp& loaddate){
	Core::initExistingSimulation(loaddate);
	buildInfluenceeMap();
	for(const auto& model : m_models){
		t_timestamp current_min = this->m_min_lookahead;
		t_timestamp model_la = model.second->lookAhead();
		this->m_min_lookahead = std::min(current_min, model_la);
		LOG_DEBUG("CCORE :: ", this->getCoreID(), " updating lookahead from " , current_min, " to ", this->m_min_lookahead);
	}
}

void Conservativecore::buildInfluenceeMap(){
	LOG_INFO("CCORE :: ", this->getCoreID(), " building influencee map.");
	std::set<std::string>	influencees;
	for(const auto& modelentry: m_models){
		const auto& model = modelentry.second;
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


void Conservativecore::postTransition(const t_atomicmodelptr& model){
	t_timestamp current_min = this->m_min_lookahead;
	t_timestamp model_la = model->lookAhead();
	this->m_min_lookahead = std::min(current_min, model_la);
	LOG_DEBUG("CCORE :: ", this->getCoreID(), " updating lookahead from " , current_min, " to ", this->m_min_lookahead);
}

void Conservativecore::resetLookahead(){
	this->m_min_lookahead = t_timestamp::infinity();
}

bool Conservativecore::timeStalled(){
        return (this->getTime().getTime()==this->getEit().getTime());
}

void Conservativecore::runSmallStep(){
        
        this->lockSimulatorStep();                      //L
        
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

                this->unlockSimulatorStep();            // UL
                
                // RST updates eit, so recheck.
                if(timeStalled())       
                        invokeStallingBehaviour();      // Still stalled (not yet deadlocked), backoff.
        }
        else{   // 3 cases : 2x not stalled (fine), stalled&released, fine
                this->unlockSimulatorStep();            // UL
                
                Core::runSmallStep();   //  L -> UL
        }
}

void Conservativecore::collectOutput(std::set<std::string>& imminents){
        // Two cases, either we have collected output already, in which case we need to remove the entry from the set
        // Or we haven't in which case the entry stays put, and we mark it for the next round.
        std::set<std::string> sortedimminents;
        for(const auto& imminent : imminents){
                auto found = m_generated_output_at.find(imminent);
                if(found != m_generated_output_at.end()){
                        // Have an entry, check timestamps (possible stale entries)
                        if(found->second.getTime()!=this->getTime().getTime()){
                                // Stale entry, need to collect output and mark model at current time.
                                sortedimminents.insert(imminent);
                                m_generated_output_at[imminent]=this->getTime();
                        }else{
                                // Have entry at current time, leave it (and leave this comment, compiler will remove it.)
                                ;
                        }
                }
                else{
                        // No entry, so make one at current time.
                        sortedimminents.insert(imminent);
                        m_generated_output_at[imminent]=this->getTime();
                }
        }
        // Base function handles all the rest (message routing etc..)
        Core::collectOutput(sortedimminents);
        // Next, we're stalled, but can be entering deadlock. Signal out current Time so the tiebreaker can
        // be found and break the lock.
        m_distributed_time->lockEntry(this->getCoreID());
        m_distributed_time->set(this->getCoreID(), this->getTime());
        m_distributed_time->unlockEntry(this->getCoreID());
        LOG_DEBUG("CCORE :: ", this->getCoreID(), " Stalled round output generation, broadcasting time :: ", this->getTime());
}

void Conservativecore::runSmallStepStalled()
{
        std::set<std::string> imminents = this->getImminent();
        collectOutput(imminents); // only collects output once
        for(const auto& imm : imminents){
                const t_atomicmodelptr mdl = this->m_models[imm];
                const t_timestamp last_scheduled = mdl->getTimeLast();
                this->scheduleModel(mdl->getName(), last_scheduled);
        }
        /**
         * If we have no imminents, our EOT value can still have changed
         */
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
        return false;
        
        /** Uncomment implementation after checking the tests.
        t_timestamp::t_time current_time = this->getTime().getTime();
        for(const auto& influencing : this->m_influencees){
                
                this->m_distributed_time->lockEntry(influencing);
                t_timestamp::t_time nulltime = this->m_distributed_time->get(influencing).getTime();
                this->m_distributed_time->unlockEntry(influencing);
                
                if(nulltime < current_time){
                        return false;
                }        
        }
        
        
        this->m_distributed_time->lockEntry(this->getCoreID());
        t_timestamp::t_time own_null = this->m_distributed_time->get(this->getCoreID()).getTime();
        this->m_distributed_time->unlockEntry(this->getCoreID());
        if(own_null != current_time)
                return false;
        
        return true;
        */
}

void
Conservativecore::sendMessage(const t_msgptr& msg){
        // At output collection, timestamp is set (color etc is of no interest to us here (and is not yet set)).
        this->m_last_sent_msgtime = msg->getTimeStamp();
        Multicore::sendMessage(msg);
}


} /* namespace n_model */
