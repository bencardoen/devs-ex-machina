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
	if(neweit > m_eit)
		m_eit = neweit;
	else{
		LOG_ERROR("Core :: ", this->getCoreID() , " trying to update EIT with a lower ", neweit, " value than current :", this->getEit());
	}
}

void Conservativecore::sendMessage(const t_msgptr& msg)
{
	m_sent_message = true;
	t_timestamp msgtime = msg->getTimeStamp();	// Is same as getTime(), but be explicit (and getTime is locked)
	t_timestamp myeot = m_distributed_eot->get(getCoreID());
	if (myeot < msgtime) {
		LOG_DEBUG("Updating EOT for ", this->getCoreID(), " from ", myeot, " to ", msgtime);
		m_distributed_eot->set(getCoreID(), msgtime);
	}
	Multicore::sendMessage(msg);
}

/**
 * Step 3 of algorithm CNPDEVS
 * 	// Use but don't change eit
 * 	If output sent:
 * 		val = (eit,1);
 * 	Else
 * 		val = eit + lookahead // no causality
 * 	candidate = std::min(val, nextscheduled);	// aka next 'autonomous event'
 *	eot = std::max(eot, candidate)
 *	m_distributed_eot.set(this->getCoreID(), eot);
 *
 */
void Conservativecore::updateEOT()
{
	// TODO rewrite with lookahead.
	t_timestamp val = this->m_eit;
	if (m_sent_message) {
		val.increaseCausality(1);
		m_sent_message = false;		// Make sure we reset this flag.
	} else {
		t_timestamp lookahead_min = t_timestamp::infinity();
		for (const auto& entry : m_models) {
			t_timestamp la = entry.second->lookAhead();
			if (la.getTime() != 0) {	// TODO Tim check if edge case is correct.
				lookahead_min = std::min(lookahead_min, la);
			} else {
				lookahead_min = std::min(lookahead_min, la);
				LOG_WARNING("CCore:: ", this->getCoreID(), " model ", entry.first,
				        " gave 0 as lookahead, ignoring.");
			}
		}
		// 2 cases : infinity() (no models or all inf), or a real non zero value.
		if (lookahead_min != t_timestamp::infinity()) {
			val = val + lookahead_min;
		} else {
			LOG_WARNING("CCore:: ", this->getCoreID(), " got only infinity/0 as lookaheads ?");
		}
	}
	// Look at first scheduled transition, compare with lookahead + eit.
	t_timestamp candidate = val;
	if (not this->m_scheduler->empty()) {
		t_timestamp firsttransition = this->m_scheduler->top().getTime();
		candidate = std::min(val, firsttransition);
	}
	//Finally, update EOT value
	t_timestamp eot = this->m_distributed_eot->get(this->getCoreID());
	eot = std::max(eot, candidate);
	LOG_INFO("Updating EOT for Core ", this->getCoreID(), " to ", eot);
	//Replaces 'sending' EOT message.
	this->m_distributed_eot->set(this->getCoreID(), eot);
}

/**
 * Step 4/5 of CNPDEVS.
 * a) For all messages from the network, for each core, get max timestamp.
 * b) From those values, get the minimum and set that value to our own EIT.
 * We don't need step a, this is allready done by sendMessage/sharedVector, so we only need to collect the maxima
 * and update EIT with the min value.
 */
void Conservativecore::updateEIT()
{
	LOG_INFO("CCore:: ", this->getCoreID(), " updating EIT eit_now = ", this->m_eit);
	t_timestamp min_eot_others = t_timestamp::infinity();
	for (size_t i = 0; i < this->m_cores; ++i) {
		if (i == this->getCoreID())
			continue;	// We don't want our own EOT (which is not an eot for us).
		min_eot_others = std::min(this->m_distributed_eot->get(i), min_eot_others);
	}
	LOG_INFO("CCore:: ", this->getCoreID(), " lowest remote EOT ==", min_eot_others);
	if (min_eot_others != t_timestamp::infinity()) {
		LOG_INFO("CCore:: ", this->getCoreID(), " updating eit to ", min_eot_others);
		this->m_eit = min_eot_others;
	} else {
		LOG_WARNING("CCore:: ",this->getCoreID(), " all eots == infinity ");
	}
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
