/*
 * conservativecore.cpp
 *
 *  Created on: 4 May 2015
 *      Author: ben
 */

#include <conservativecore.h>

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
	t_timestamp myeot = m_distributed_eot->get(getCoreID());
	/// Step 4 in PDF, but do this @ caller side.
	if (myeot < msgtime) {
		LOG_DEBUG("Updating EOT for ", this->getCoreID(), " from ", myeot, " to ", msgtime);
		m_distributed_eot->set(getCoreID(), msgtime);
	}
	Multicore::sendMessage(msg);
}

void Conservativecore::updateEOT()
{
	/** Step 3 of CNPDEVS
	 * Pseudocode :
	 * 	EOT(myid) = std::min(x,y)
	 * 		x = eit + lookahead_min
	 * 		y = 	if(sent_message)	eit,1
	 * 			else			top of scheduler (next event)
	 */
	t_timestamp x;
	if(m_eit == t_timestamp::infinity() || this->m_min_lookahead == t_timestamp::infinity())
		x = t_timestamp::infinity();
	else{
		x=t_timestamp(m_eit.getTime() + this->m_min_lookahead.getTime(), 0);
	}
	t_timestamp y = t_timestamp::infinity();
	if(this->m_sent_message){
		this->m_sent_message = false;		// Reset sent flag, else we won't have a clue.
		y = t_timestamp(this->getTime().getTime(), 1);
	}else{
		if(!this->m_scheduler->empty()){
			y = this->m_scheduler->top().getTime();
		}
	}
	t_timestamp neweot = std::min(x,y);
	LOG_INFO("CCore:: ", this->getCoreID(), " updating eot to ", neweot, " x = ", x, " y = ", y);
	this->m_distributed_eot->set(this->getCoreID(), neweot);
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
	/**
	 * Pseudocode: our EIT = min of all distributed EOT's, IFF those carry models that
	 * influence us.
	 */
	LOG_INFO("CCore:: ", this->getCoreID(), " updating EIT eit_now = ", this->m_eit);
	t_timestamp min_eot_others = t_timestamp::infinity();
	for(const auto& influence_id : m_influencees){
		const t_timestamp new_eot = this->m_distributed_eot->get(influence_id);
		min_eot_others = std::min(min_eot_others, new_eot);
	}
	this->setEit(min_eot_others);
	LOG_INFO("Core:: ", this->getCoreID(), " new EIT == ", min_eot_others);
}

void Conservativecore::syncTime(){
	this->updateEOT();
	this->updateEIT();
	Core::syncTime();	// Multicore has no syncTime.
	this->resetLookahead();
}

void Conservativecore::setTime(const t_timestamp& newtime){
	LOG_INFO("CCORE :: ", this->getCoreID(), " got request to forward time from ", this->getTime(), " to ", newtime);
	t_timestamp corrected = std::min( this->getEit(), newtime);
	LOG_INFO("CCORE :: ", this->getCoreID(), " corrected time ", corrected , " == min (", this->getEit(), ", ", newtime);
	Multicore::setTime(corrected);
}

void Conservativecore::init(){
	Core::init();
	buildInfluenceeMap();
}


void Conservativecore::initExistingSimulation(t_timestamp loaddate){
	Core::initExistingSimulation(loaddate);
	buildInfluenceeMap();
}

void Conservativecore::buildInfluenceeMap(){
	LOG_INFO("CCORE :: ", this->getCoreID(), " building influencee map");
	std::set<std::string>	influencees;
	for(const auto& modelentry: m_models){
		const auto& model = modelentry.second;
		model->addInfluencees(influencees);
	}
	for(const auto& modelname : influencees){
		std::size_t influencee_core = this->m_loctable->lookupModel(modelname);
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

} /* namespace n_model */
