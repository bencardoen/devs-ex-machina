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
	m_eit(t_timestamp(0, 0)), m_distributed_eot(vc), m_sent_message(false),m_min_lookahead(t_timestamp::infinity())
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

void Conservativecore::sendMessage(const t_msgptr& msg)
{
	m_sent_message = true;
	t_timestamp msgtime = msg->getTimeStamp();	// Is same as getTime(), but be explicit (and getTime is locked)
	m_distributed_eot->lockEntry(getCoreID());
	t_timestamp myeot = m_distributed_eot->get(getCoreID());
	/// Step 4 in PDF, but do this @ caller side.
	if (myeot < msgtime) {
		LOG_DEBUG("Updating EOT for ", this->getCoreID(), " from ", myeot, " to ", msgtime);
		m_distributed_eot->set(getCoreID(), msgtime);
	}
	m_distributed_eot->unlockEntry(getCoreID());
	Multicore::sendMessage(msg);
}

/** Step 3 of CNPDEVS
	 * Pseudocode :
	 * 	EOT(myid) = std::min(x,y)
	 * 		x = eit + lookahead_min
	 * 		y = 	if(sent_message)	eit,1
	 * 			else			top of scheduler (next event)
	 * 			none of the above : oo
	 */
void Conservativecore::updateEOT()
{
	t_timestamp x;
	/// Edge case : EIT=oo (non-dependent kernel), DO NOT add lookahead to inf, but use time().
	if(isInfinity(this->m_eit) || isInfinity(this->m_min_lookahead)){
		x = t_timestamp::infinity();
	}else{
		x=t_timestamp(m_eit.getTime() + this->m_min_lookahead.getTime(), 0);	//  this can still overflow, but then we're dead anyway.
	}

	t_timestamp y = t_timestamp::infinity();
	if(this->m_sent_message){
		this->m_sent_message = false;		// Reset sent flag, else we won't have a clue.
		if(this->m_eit.getTime()==std::numeric_limits<t_timestamp::t_time>::max()){	// Be very careful here, eit=oo does not imply
			y = t_timestamp(this->getTime().getTime(), 1);				// that we're sending with timestamp oo, a detail
		}else{										// that's missing from the pdf.
			y = t_timestamp(this->m_eit.getTime(), 1);
		}
	}else{
		if(!this->m_scheduler->empty()){
			y = this->m_scheduler->top().getTime();
		}
	}

	t_timestamp neweot = std::min(x,y);
	LOG_INFO("CCore:: ", this->getCoreID(), " updating eot to ", neweot, " x = ", x, " y = ", y);
	this->m_distributed_eot->lockEntry(getCoreID());
	this->m_distributed_eot->set(this->getCoreID(), neweot);
	this->m_distributed_eot->unlockEntry(getCoreID());
}

/**
 * Step 4/5 of CNPDEVS.
 * a) For all messages from the network, for each core, get max timestamp.
 * b) From those values, get the minimum and set that value to our own EIT.
 * We don't need step a, this is already done by sendMessage/sharedVector, so we only need to collect the maxima
 * and update EIT with the min value.
 */
void Conservativecore::updateEIT()
{
	/**Implementation alg:
	 * 	-> for all kernels we depend on, get their max EOT
	 * 	-> from that set, get the min, that's our new EIT
	 */
	LOG_INFO("CCore:: ", this->getCoreID(), " updating EIT:: eit_now = ", this->m_eit);
	t_timestamp min_eot_others = t_timestamp::infinity();
	for(const auto& influence_id : m_influencees){
		this->m_distributed_eot->lockEntry(influence_id);
		const t_timestamp new_eot = this->m_distributed_eot->get(influence_id);
		this->m_distributed_eot->unlockEntry(influence_id);
		min_eot_others = std::min(min_eot_others, new_eot);
	}
        // TODO use getMessages() and take that timestamp into account as well.
	this->setEit(min_eot_others);
	LOG_INFO("Core:: ", this->getCoreID(), " new EIT == ", min_eot_others);
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
	if(this->isIdle()){
		this->m_distributed_eot->lockEntry(getCoreID());
		this->m_distributed_eot->set(this->getCoreID(), t_timestamp(this->getTime().getTime(), 0));
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

        t_timestamp adjustedtime = this->getTime();
        if(newtime.getTime() > this->getEit().getTime()){
                LOG_INFO("CCORE :: ", this->getCoreID(), " newtime >= eit , refusing advance");
                // adjustedtime stays the same.
        }
        else{
                // newtime <= eit, use newtime
                adjustedtime = std::min(newtime, this->getEit());
                LOG_INFO("CCORE :: ", this->getCoreID(), " newtime < eit , advacing to ", adjustedtime );
        }
	// Core::setTime is not locked, which is an issue if we run GVT async.
	// vv is a synchronized setter.
	Multicore::setTime(adjustedtime);
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
	LOG_INFO("CCORE :: ", this->getCoreID(), " influencee map == ");
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

t_timestamp Conservativecore::getEit(){
	return m_eit;
}

} /* namespace n_model */
