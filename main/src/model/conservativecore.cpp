/*
 * conservativecore.cpp
 *
 *  Created on: 4 May 2015
 *      Author: Ben Cardoen -- Tim Tuijn
 */

#include "model/conservativecore.h"

namespace n_model {

Conservativecore::Conservativecore(const t_networkptr& n, std::size_t coreid,
        const n_control::t_location_tableptr& ltable, size_t cores, const t_eotvector& vc)
	: Multicore(n, coreid, ltable, cores), /*Forward entire parampack.*/
	m_eit(t_timestamp(0, 0)), m_distributed_eot(vc),m_min_lookahead(t_timestamp::infinity())
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
                LOG_ERROR("MCORE:: ",this->getCoreID(), " eot moving backward in time, BUG.");
        }
	this->m_distributed_eot->set(this->getCoreID(), neweot);
	this->m_distributed_eot->unlockEntry(getCoreID());
        LOG_DEBUG("CCore:: ", this->getCoreID(), " updating eot from ", oldeot, " to ", neweot, " min of  x = ", x);
        LOG_DEBUG("CCore:: ", this->getCoreID(), " y_sent ", y_sent, " y_pending ", y_pending, " y_imminent ", y_imminent);
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
	LOG_INFO("CCore:: ", this->getCoreID(), " updating EIT:: eit_now = ", this->m_eit);
	t_timestamp min_eot_others = t_timestamp::infinity();
	for(const auto& influence_id : m_influencees){
		this->m_distributed_eot->lockEntry(influence_id);
		const t_timestamp new_eot = this->m_distributed_eot->get(influence_id);
		this->m_distributed_eot->unlockEntry(influence_id);
		min_eot_others = std::min(min_eot_others, new_eot);
	}
        
        LOG_INFO("Core:: ", this->getCoreID(), " setting EIT == ",  min_eot_others, " from ", this->getEit());
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

void Conservativecore::postTransition(const t_atomicmodelptr& model){
	t_timestamp current_min = this->m_min_lookahead;
	t_timestamp model_la = model->lookAhead();
	this->m_min_lookahead = std::min(current_min, model_la);
	LOG_DEBUG("CCORE :: ", this->getCoreID(), " updating lookahead from " , current_min, " to ", this->m_min_lookahead);
}

void Conservativecore::resetLookahead(){
	this->m_min_lookahead = t_timestamp::infinity();
}

void Conservativecore::runSmallStep(){
        
        this->lockSimulatorStep();                      //L
        
        if(this->getEit().getTime() == this->getTime().getTime()){
                LOG_DEBUG("CCORE :: ", this->getCoreID(), " EIT==TIME ");
                m_stats.logStat(STAT_TYPE::STALLEDROUNDS);
    
                this->runSmallStepStalled();
    
                this->unlockSimulatorStep();            // UL
        }
        else{
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


} /* namespace n_model */
